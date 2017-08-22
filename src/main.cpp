#include "../.build_files/src/lib/parsing.bs.hpp"
#include "lib/unit.h"
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>

extern FILE* yyin;

namespace minimale {

typedef std::unique_ptr<FILE, decltype(&fclose)> unique_file;

store read_store(const std::string file_path) {
  unique_file file(fopen(file_path.c_str(), "r"), fclose);
  if (!file) {
    throw std::runtime_error("cannot open file: " + file_path);
  }
  yyin = file.get();

  store st;
  yy::parser pr(st);
  //pr.set_debug_level(true);
  if (pr.parse() != 0) {
    throw std::runtime_error("parse failed");
  }
  return st;
}

struct component_structure {
  std::map<std::string, type_annotation_id> state;
  expression_id render;
};

struct invalid_field_name_error {
  invalid_field_name_error(const std::string& field_name): field_name(field_name) {}
  std::string field_name;
};

component_structure get_structure(const store& st, const component& cp) {
  component_structure result;
  for (const auto& s_id: cp.statements) {
    if (s_id.type == component_statement_type::field) {
      const auto& field = st.fields[s_id.value];
      if (field.name == "state") {
        if (field.type_annotation.type != type_annotation_type::object)
          throw std::runtime_error("`state` must be an object");
        const auto& ot = st.object_type_annotations[field.type_annotation.value];
        for (const auto& otf: ot.fields) {
          result.state[otf.name] = otf.type_annotation;
        }
        continue;
      }
      throw invalid_field_name_error(field.name);
    }
    if (s_id.type == component_statement_type::function) {
      const auto& fn = st.functions[s_id.value];
      if (fn.name == "render") {
        const auto& st_id = fn.statements[0];
        if (st_id.type != function_statement_type::return_) {
          throw std::runtime_error("statement not supported");
        }
        result.render = st.return_statements[st_id.value].expression;
        if (fn.statements.size() > 1) {
          throw std::runtime_error("too many statements");
        }
      }
      continue;
    }
    throw std::runtime_error("invalid type");
  }
  return result;
}

void write_state_type(
  const std::string& state_type_name,
  const store& st,
  const component_structure& cs,
  std::ostream& os
) {
  os << "export type " << state_type_name << " = {" << std::endl;
  for (const auto& pair: cs.state) {
    if (pair.second.type != type_annotation_type::literal) {
      throw std::runtime_error("cannot handle non-literal type annotations");
    }
    const auto& tt = st.literal_type_annotations[pair.second.value];
    os << "  " << pair.first << ": " << tt.identifier << ',' << std::endl;
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

bool is_state(const store& st, const expression_id& xp) {
  if (xp.type != expression_type::member_access) return false;
  const auto& ma = st.member_accesses[xp.value];
  if (ma.expression.type != expression_type::reference) return false;
  if (ma.identifier != "state") return false;
  const auto& ref = st.references[ma.expression.value];
  return ref == "this";
}

void write_node_creator(
  const store& st,
  const expression_id& xp,
  const component_structure& cs,
  const std::string& indent,
  const std::string& root_var_name,
  const std::unordered_set<size_t> member_tags,
  std::ostream& os
) {
  if (xp.type == expression_type::reference) {
    throw std::runtime_error("cannot handle references");
  }
  if (xp.type == expression_type::member_access) {
    const auto& ma = st.member_accesses[xp.value];
    if (!is_state(st, ma.expression)) {
      throw std::runtime_error("cannot handle other than `this.state`");
    }
    if (cs.state.find(ma.identifier) == cs.state.end()) {
      throw std::runtime_error("prop `" + ma.identifier + "` unknown");
    }
    os
      << indent << root_var_name
      << ".appendChild(d.createTextNode("
      << "initialState." << ma.identifier
      << "));" << std::endl;
    return;
  }
  if (xp.type == expression_type::string) {
    const auto& str = st.strings[xp.value];
    os
      << indent << root_var_name
      << ".appendChild(d.createTextNode('"
      << str
      << "'));" << std::endl;
    return;
  }
  const auto is_member = member_tags.count(xp.value) > 0;
  const auto var_name =
    std::string(is_member ? "this." : "") + "e" + std::to_string(xp.value);
  const auto& tag = st.xml_tags[xp.value];
  os << indent;
  if (!is_member) os << "const ";
  os
    << var_name << " = d.createElement('"
      << tag.tag_name << "');" << std::endl
    << indent << root_var_name << ".appendChild(" << var_name << ");"
      << std::endl;
  std::ostringstream text_acc;
  bool keep_front_ws = false;
  for (const auto& frg: tag.fragments) {
    if (frg.type == xml_fragment_type::text) {
      text_acc << st.xml_text_fragments[frg.value];
      continue;
    }
    if (!text_acc.str().empty()) {
      write_text_node_creator(var_name, indent, text_acc.str(), keep_front_ws, true, os);
      text_acc.str("");
      text_acc.clear();
    }
    if (frg.type != xml_fragment_type::interpolation) {
      throw std::runtime_error("invalid fragment");
    }
    const auto& ixp = st.xml_interpolations[frg.value];
    write_node_creator(st, ixp, cs, indent, var_name, member_tags, os);
    keep_front_ws = true;
  }
  if (!text_acc.str().empty()) {
    write_text_node_creator(var_name, indent, text_acc.str(), keep_front_ws, false, os);
  }
}

void write_unmount_function(
  const store& st,
  const expression_id& xp,
  const component_structure& cs,
  const std::string& indent,
  std::ostream& os
) {
  auto id = std::to_string(xp.value);
  os
    << indent << "if (this.root == null) return;" << std::endl
    << indent << "this.root.removeChild(this.e" << id
    << ");" << std::endl
    << indent << "this.e" << id << " = null;" << std::endl
    << indent << "this.root = null;" << std::endl;
}

void write_typescript(const store& st, std::ostream& os) {
  os
    << "/* Generated with the `minimale` tool. */" << std::endl
    << std::endl;
  for (auto st_id: st.unit.statements) {
    if (st_id.type == statement_type::component) {
      const auto& cp = st.components[st_id.value];
      auto cs = get_structure(st, cp);
      auto state_type_name = cp.name + "State";
      write_state_type(state_type_name, st, cs, os);
      std::unordered_set<size_t> member_tags = {cs.render.value};
      os
        << "export default class " << cp.name << " {" << std::endl
        << "  private root: HTMLElement;" << std::endl;
      for (auto mt: member_tags) {
        os
          << "  private e" << std::to_string(mt)
          << ": HTMLElement;" << std::endl;
      }
      os
        << std::endl
        << "  constructor(root: HTMLElement, initialState: "
          << state_type_name << ") {" << std::endl
        << "    this.root = root;" << std::endl
        << "    const d = root.ownerDocument;" << std::endl;
      write_node_creator(st, cs.render, cs, "    ", "root", member_tags, os);
      os
        << "  }" << std::endl << std::endl
        << "  unmount(): void {" << std::endl;
      write_unmount_function(st, cs.render, cs, "    ", os);
      os
        << "  }" << std::endl
        << "}" << std::endl;
    }
  }
}

int run(int argc, char *argv[]) {
  if (argc < 5) {
    throw std::runtime_error("input file must be specified");
  }
  store st = read_store(argv[1]);
  std::ofstream of;
  of.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  of.open(argv[2]);
  try {
    write_typescript(st, of);
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
