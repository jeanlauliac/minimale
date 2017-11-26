#include "../gen/src/lib/parsing.bs.hpp"
#include "../gen/src/lib/lang_variants.json.h"
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

extern FILE* yyin;

namespace minimale {

typedef std::unique_ptr<FILE, decltype(&fclose)> unique_file;

lang::unit read_unit(const std::string file_path) {
  unique_file file(fopen(file_path.c_str(), "r"), fclose);
  if (!file) {
    throw std::runtime_error("cannot open file: " + file_path);
  }
  yyin = file.get();

  lang::unit unit;
  yy::parser pr(unit);
  //pr.set_debug_level(true);
  if (pr.parse() != 0) {
    throw std::runtime_error("parse failed");
  }
  return unit;
}

struct mut_inst {
  const lang::expr* target;
  const lang::expr* val;
};

struct comp_mutator {
  std::string name;
  const std::vector<lang::func_arg>* args;
  std::vector<mut_inst> insts;
};

/**
 * Use pointers to avoid doing copies. Require the syntax tree to be kept
 * alive while this is used.
 */
struct component_structure {
  std::map<std::string, const lang::type_annot*> state;
  std::map<std::string, comp_mutator> mutators;
  const lang::expr* render;
};

struct invalid_field_name_error {
  invalid_field_name_error(const std::string& field_name): field_name(field_name) {}
  std::string field_name;
};

bool is_state(const lang::expr& xp) {
  if (!xp.is_member_access()) return false;
  const auto& ma = xp.as_member_access();
  if (!ma.expr.is_ref()) return false;
  if (ma.member != "state") return false;
  return ma.expr.as_ref().ident == "this";
}

void get_assignment_insts(
  const lang::expr& xp,
  const std::string& state_member,
  const lang::expr& val,
  std::vector<mut_inst>& insts,
  std::unordered_set<const lang::expr*>& member_tags
) {
  if (xp.is_member_access()) {
    const auto& ma = xp.as_member_access();
    if (!is_state(ma.expr))
      throw std::runtime_error("cannot handle other than `this.state`");
    if (state_member == ma.member) {
      insts.push_back({ &xp, &val });
      member_tags.insert(&xp);
    }
    return;
  }
  if (!xp.is_xml_tag()) return;
  const auto& tag = xp.as_xml_tag();
  for (const auto& frag: tag.frags) {
    if (!frag.is_xml_interp()) continue;
    get_assignment_insts(frag.as_xml_interp().expr, state_member, val, insts, member_tags);
  }
}

component_structure get_structure(
  const lang::comp& cp,
  std::unordered_set<const lang::expr*>& member_tags
) {
  component_structure result;
  std::vector<const lang::comp_func*> mutators;
  for (const auto& s: cp.stmts) {
    if (s.is_comp_field()) {
      const auto& field = s.as_comp_field();
      if (field.name == "state") {
        if (!field.type.is_obj_type_annot())
          throw std::runtime_error("`state` must be an object");
        const auto& ot = field.type.as_obj_type_annot();
        for (const auto& otf: ot.fields) {
          result.state[otf.name] = &otf.type;
        }
        continue;
      }
      throw invalid_field_name_error(field.name);
    }
    if (s.is_comp_func()) {
      const auto& fn = s.as_comp_func();
      if (fn.name == "render") {
        const auto& fs = fn.stmts[0];
        if (!fs.is_return_stmt()) {
          throw std::runtime_error("statement not supported");
        }
        result.render = &fs.as_return_stmt().expr;
        if (fn.stmts.size() > 1) {
          throw std::runtime_error("too many statements");
        }
        continue;
      }
      mutators.push_back(&fn);
      continue;
    }
    throw std::runtime_error("invalid type");
  }
  member_tags.insert(result.render);
  for (auto mutator: mutators) {
    comp_mutator mt;
    mt.name = mutator->name;
    mt.args = &mutator->args;
    for (const auto& stmt: mutator->stmts) {
      if (!stmt.is_assignment()) {
        throw std::runtime_error("statement not supported");
      }
      const auto& asg = stmt.as_assignment();
      if (!asg.assigned.is_member_access()) throw std::runtime_error("expr not supported");
      const auto& ma = asg.assigned.as_member_access();
      if (!is_state(ma.expr))
        throw std::runtime_error("cannot handle other than `this.state`");
      if (result.state.count(ma.member) == 0)
        throw std::runtime_error("unknown state member");
      get_assignment_insts(*result.render, ma.member, asg.val, mt.insts, member_tags);
    }
    result.mutators.insert({ mutator->name, mt });
  }
  return result;
}

void write_state_type(
  const std::string& state_type_name,
  const component_structure& cs,
  std::ostream& os
) {
  os << "export type " << state_type_name << " = {" << std::endl;
  for (const auto& pair: cs.state) {
    if (!pair.second->is_ltr_type_annot()) {
      throw std::runtime_error("cannot handle non-literal type annotations");
    }
    const auto& tt = pair.second->as_ltr_type_annot();
    os << "  " << pair.first << ": " << tt.ident << ',' << std::endl;
  }
  os << "}" << std::endl << std::endl;
}

bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t';
}

std::string trim_xml_whitespace(
  const std::string& s,
  bool keep_front_ws,
  bool keep_back_ws
) {
  std::ostringstream os;
  size_t i = 0;
  if (!keep_front_ws) {
    while (i < s.size() && is_whitespace(s[i])) ++i;
  }
  bool need_space = false;
  while (i < s.size()) {
    char c = s[i];
    if (is_whitespace(c)) {
      need_space = true;
    } else {
      if (need_space) os << ' ';
      os << c;
      need_space = false;
    }
    ++i;
  }
  if (keep_back_ws && need_space) {
    os << ' ';
  }
  return os.str();
}

void write_text_node_creator(
  const std::string& var_name,
  const std::string& indent,
  const std::string& text,
  bool keep_front_ws,
  bool keep_back_ws,
  std::ostream& os
) {
  auto str = trim_xml_whitespace(text, keep_front_ws, keep_back_ws);
  if (str.empty()) return;
  os
    << indent << var_name
    << ".appendChild(d.createTextNode('"
    << str
    << "'));" << std::endl;
}

namespace ts {

std::ostream& write_ltr_string(const std::string val, std::ostream& os) {
  os << "'";
  for (char c: val) {
    if (c == '\'') {
      os << "\\'";
    } else {
      os << c;
    }
  }
  return os << "'";
}

}

size_t get_expr_id(
  std::unordered_map<const lang::expr*, size_t>& ids,
  const lang::expr& xp
) {
  const auto idi = ids.find(&xp);
  if (idi != ids.cend()) return idi->second;
  size_t new_id = ids.size() + 1;
  ids.insert({ &xp, new_id });
  return new_id;
}

void write_node_creator(
  const lang::expr& xp,
  const component_structure& cs,
  const std::string& indent,
  const std::string& root_var_name,
  const std::unordered_set<const lang::expr*> member_tags,
  std::unordered_map<const lang::expr*, size_t>& ids,
  std::ostream& os
) {
  const auto is_member = member_tags.count(&xp) > 0;
  auto id = get_expr_id(ids, xp);
  const auto var_name =
    std::string(is_member ? "this." : "") + "e" + std::to_string(id);
  if (xp.is_ref()) {
    throw std::runtime_error("cannot handle references");
  }
  if (xp.is_member_access()) {
    const auto& ma = xp.as_member_access();
    if (!is_state(ma.expr)) {
      throw std::runtime_error("cannot handle other than `this.state`");
    }
    if (cs.state.find(ma.member) == cs.state.end()) {
      throw std::runtime_error("prop `" + ma.member + "` unknown");
    }
    if (is_member) {
      os
        << indent << var_name << " = d.createTextNode("
        << "initialState." << ma.member
        << ");" << std::endl
        << indent << root_var_name << ".appendChild(" << var_name << ");"
        << std::endl;
      return;
    }
    os
      << indent << root_var_name
      << ".appendChild(d.createTextNode("
      << "initialState." << ma.member
      << "));" << std::endl;
    return;
  }
  if (xp.is_ltr_string()) {
    if (is_member) {
      os
        << indent << var_name
        << " = d.createTextNode(";
      ts::write_ltr_string(xp.as_ltr_string().val, os)
        << ");" << std::endl;
      os
        << indent << root_var_name << ".appendChild(" << var_name << ");"
        << std::endl;
    } else {
      os
        << indent << root_var_name
        << ".appendChild(d.createTextNode(";
      ts::write_ltr_string(xp.as_ltr_string().val, os)
        << "));" << std::endl;
    }
    return;
  }
  if (!xp.is_xml_tag()) throw std::runtime_error("unexpected expression");
  const auto& tag = xp.as_xml_tag();
  os << indent;
  if (!is_member) os << "const ";
  os
    << var_name << " = d.createElement('"
      << tag.name << "');" << std::endl;
  for (const auto& attr_pair: tag.attrs) {
    if (!attr_pair.second.is_ltr_string())
      throw std::runtime_error("unsupported attr value");
    os << indent << var_name << "." << attr_pair.first << " = ";
    ts::write_ltr_string(attr_pair.second.as_ltr_string().val, os)
      << ";" << std::endl;
  }
  std::ostringstream text_acc;
  bool keep_front_ws = false;
  for (const auto& frg: tag.frags) {
    if (frg.is_xml_text()) {
      text_acc << frg.as_xml_text().val;
      continue;
    }
    if (!text_acc.str().empty()) {
      write_text_node_creator(var_name, indent, text_acc.str(), keep_front_ws, true, os);
      text_acc.str("");
      text_acc.clear();
    }
    if (!frg.is_xml_interp()) {
      throw std::runtime_error("invalid fragment");
    }
    const auto& ixp = frg.as_xml_interp();
    write_node_creator(ixp.expr, cs, indent, var_name, member_tags, ids, os);
    keep_front_ws = true;
  }
  if (!text_acc.str().empty()) {
    write_text_node_creator(var_name, indent, text_acc.str(), keep_front_ws, false, os);
  }
  os
    << indent << root_var_name << ".appendChild(" << var_name << ");"
    << std::endl;
}

void write_unmount_function(
  const lang::expr& xp,
  const component_structure& cs,
  const std::string& indent,
  std::unordered_map<const lang::expr*, size_t>& ids,
  const std::unordered_set<const lang::expr*> member_tags,
  std::ostream& os
) {
  auto id = std::to_string(get_expr_id(ids, xp));
  os
    << indent << "this.root.removeChild(this.e" << id
    << ");" << std::endl;
  for (auto expr: member_tags) {
    os
      << indent << "this.e" << std::to_string(get_expr_id(ids, *expr))
      << " = null;" << std::endl;
  }
  os
    << indent << "this.root = null;" << std::endl;
}

void write_mutator(
  const component_structure& cs,
  const std::string& indent,
  const comp_mutator& mut,
  std::unordered_map<const lang::expr*, size_t>& ids,
  std::ostream& os
) {
  os << indent << mut.name << "(";
  for (const auto& arg: *mut.args) {
    os << arg.name << ": ";
    if (!arg.type.is_ltr_type_annot()) {
      throw std::runtime_error("cannot handle non-literal type annotations");
    }
    os << arg.type.as_ltr_type_annot().ident << ", ";
  }
  os << ") {" << std::endl;
  for (const auto& inst: mut.insts) {
    auto id = std::to_string(get_expr_id(ids, *inst.target));
    os << indent << "  this.e" << id << ".nodeValue = ";
    if (inst.val->is_ref()) {
      os << inst.val->as_ref().ident;
    } else if (inst.val->is_ltr_string()) {
      ts::write_ltr_string(inst.val->as_ltr_string().val, os);
    } else {
      throw std::runtime_error("cannot handle this mutation instruction expr");
    }
    os << ";" << std::endl;
  }
  os << indent << "}" << std::endl;
}

void write_mutators(
  const component_structure& cs,
  const std::string& indent,
  std::unordered_map<const lang::expr*, size_t>& ids,
  std::ostream& os
) {
  for (const auto& mt_pair: cs.mutators) {
    os << std::endl;
    write_mutator(cs, indent, mt_pair.second, ids, os);
  }
}

void write_typescript(const lang::unit& ut, std::ostream& os) {
  os
    << "/* Generated with the `minimale` tool. */" << std::endl
    << std::endl;
  for (auto st: ut.stmts) {
    if (!st.is_comp()) {
      continue;
    }
    const auto& cp = st.as_comp();
    std::unordered_set<const lang::expr*> member_tags = {};
    std::unordered_map<const lang::expr*, size_t> ids = {};
    auto cs = get_structure(cp, member_tags);
    auto state_type_name = cp.name + "State";
    write_state_type(state_type_name, cs, os);
    os
      << "export default class " << cp.name << " {" << std::endl
      << "  private root: HTMLElement;" << std::endl;
    for (auto mt: member_tags) {
      os
        << "  private e" << std::to_string(get_expr_id(ids, *mt)) << ": ";
      if (mt->is_ltr_string() || mt->is_ref() || mt->is_member_access()) {
        os << "Node";
      } else {
        os << "HTMLElement";
      }
      os << ";" << std::endl;
    }
    os
      << std::endl
      << "  constructor(root: HTMLElement, initialState: "
        << state_type_name << ") {" << std::endl
      << "    this.root = root;" << std::endl
      << "    const d = root.ownerDocument;" << std::endl;
    write_node_creator(*cs.render, cs, "    ", "root", member_tags, ids, os);
    os
      << "  }" << std::endl << std::endl
      << "  unmount(): void {" << std::endl;
    write_unmount_function(*cs.render, cs, "    ", ids, member_tags, os);
    os << "  }" << std::endl;
    write_mutators(cs, "  ", ids, os);
    os << "}" << std::endl;
  }
}

int run(int argc, char *argv[]) {
  if (argc < 5) {
    throw std::runtime_error("input file must be specified");
  }
  lang::unit ut = read_unit(argv[1]);
  std::ofstream of;
  of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  of.open(argv[2]);
  try {
    write_typescript(ut, of);
  } catch (invalid_field_name_error er) {
    std::cerr
      << "fatal: invalid field name `"
      << er.field_name << "`" << std::endl;
    return 1;
  }
  of.close();
  of.open(argv[3]);
  of << argv[2] << ": " << argv[4] << std::endl;
  return 0;
}

}

int main(int argc, char *argv[]) {
  return minimale::run(argc, argv);
}
