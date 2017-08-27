#include "../.build_files/src/lib/parsing.bs.hpp"
#include "../.build_files/src/lib/lang_variants.json.h"
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

struct component_structure {
  std::map<std::string, lang::type_annot> state;
  std::vector<lang::comp_func> mutators;
  lang::expr render;
};

struct invalid_field_name_error {
  invalid_field_name_error(const std::string& field_name): field_name(field_name) {}
  std::string field_name;
};

component_structure get_structure(const lang::comp& cp) {
  component_structure result;
  for (const auto& s: cp.stmts) {
    if (s.is_comp_field()) {
      const auto& field = s.as_comp_field();
      if (field.name == "state") {
        if (!field.type.is_obj_type_annot())
          throw std::runtime_error("`state` must be an object");
        const auto& ot = field.type.as_obj_type_annot();
        for (const auto& otf: ot.fields) {
          result.state[otf.name] = otf.type;
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
        result.render = fs.as_return_stmt().expr;
        if (fn.stmts.size() > 1) {
          throw std::runtime_error("too many statements");
        }
        continue;
      }
      result.mutators.push_back(fn);
      continue;
    }
    throw std::runtime_error("invalid type");
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
    if (!pair.second.is_ltr_type_annot()) {
      throw std::runtime_error("cannot handle non-literal type annotations");
    }
    const auto& tt = pair.second.as_ltr_type_annot();
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

bool is_state(const lang::expr& xp) {
  if (!xp.is_member_access()) return false;
  const auto& ma = xp.as_member_access();
  if (!ma.expr.is_ref()) return false;
  if (ma.member != "state") return false;
  return ma.expr.as_ref().ident == "this";
}

void write_node_creator(
  const lang::expr& xp,
  const component_structure& cs,
  const std::string& indent,
  const std::string& root_var_name,
  const std::unordered_set<const lang::expr*> member_tags,
  std::unordered_map<const lang::expr*, size_t> ids,
  size_t& id_count,
  std::ostream& os
) {
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
    os
      << indent << root_var_name
      << ".appendChild(d.createTextNode("
      << "initialState." << ma.member
      << "));" << std::endl;
    return;
  }
  if (xp.is_ltr_string()) {
    os
      << indent << root_var_name
      << ".appendChild(d.createTextNode('"
      << xp.as_ltr_string().val
      << "'));" << std::endl;
    return;
  }
  const auto is_member = member_tags.count(&xp) > 0;
  const auto idi = ids.find(&xp);
  size_t id;
  if (idi == ids.cend()) {
    ids.insert({ &xp, id_count++ });
    id = id_count;
  } else {
    id = idi->second;
  }
  const auto var_name =
    std::string(is_member ? "this." : "") + "e" + std::to_string(id);
  const auto& tag = xp.as_xml_tag();
  os << indent;
  if (!is_member) os << "const ";
  os
    << var_name << " = d.createElement('"
      << tag.name << "');" << std::endl;
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
    write_node_creator(ixp.expr, cs, indent, var_name, member_tags, ids, id_count, os);
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
  size_t xp_id,
  const component_structure& cs,
  const std::string& indent,
  std::ostream& os
) {
  auto id = std::to_string(xp_id);
  os
    << indent << "if (this.root == null) return;" << std::endl
    << indent << "this.root.removeChild(this.e" << id
    << ");" << std::endl
    << indent << "this.e" << id << " = null;" << std::endl
    << indent << "this.root = null;" << std::endl;
}

void write_mutator(
  const component_structure& cs,
  const std::string& indent,
  const lang::comp_func& fn,
  std::ostream& os
) {
  os
    << indent << fn.name << "(" << ") {" << std::endl
    << indent << "  ;" << std::endl
    << indent << "}" << std::endl;
}

void write_mutators(
  const component_structure& cs,
  const std::string& indent,
  std::ostream& os
) {
  for (const auto& mutator: cs.mutators) {
    os << std::endl;
    write_mutator(cs, indent, mutator, os);
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
    auto cs = get_structure(cp);
    auto state_type_name = cp.name + "State";
    write_state_type(state_type_name, cs, os);
    std::unordered_map<const lang::expr*, size_t> ids = { { &cs.render, 1 } };
    std::unordered_set<const lang::expr*> member_tags = { &cs.render };
    size_t id_count = 1;
    os
      << "export default class " << cp.name << " {" << std::endl
      << "  private root: HTMLElement;" << std::endl;
    for (auto mt: member_tags) {
      os
        << "  private e" << std::to_string(ids.at(mt))
        << ": HTMLElement;" << std::endl;
    }
    os
      << std::endl
      << "  constructor(root: HTMLElement, initialState: "
        << state_type_name << ") {" << std::endl
      << "    this.root = root;" << std::endl
      << "    const d = root.ownerDocument;" << std::endl;
    write_node_creator(cs.render, cs, "    ", "root", member_tags, ids, id_count, os);
    os
      << "  }" << std::endl << std::endl
      << "  unmount(): void {" << std::endl;
    write_unmount_function(1, cs, "    ", os);
    os << "  }" << std::endl;
    write_mutators(cs, "  ", os);
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
