#include "../.build_files/src/lib/parsing.bs.hpp"
#include "lib/unit.h"
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

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
  std::map<std::string, type_annotation_id> props;
};

struct invalid_field_name_error {
  invalid_field_name_error(const std::string& field_name): field_name(field_name) {}
  std::string field_name;
};

component_structure get_structure(const store& st, size_t comp_id) {
  component_structure result;
  const auto& cp = st.components[comp_id];
  for (const auto& s_id: cp.statements) {
    if (s_id.type != component_statement_type::field) continue;
    const auto& field = st.fields[s_id.value];
    if (field.name == "props") {
      if (field.type_annotation.type != type_annotation_type::object)
        throw std::runtime_error("`props` must be an object");
      const auto& ot = st.object_type_annotations[field.type_annotation.value];
      for (const auto& otf: ot.fields) {
        result.props[otf.name] = otf.type_annotation;
      }
      continue;
    }
    throw invalid_field_name_error(field.name);
  }
  return result;
}

void write_props_type(
  const std::string& props_type_name,
  const store& st,
  const component_structure& cs,
  std::ostream& os
) {
  os << "export type " << props_type_name << " = {" << std::endl;
  for (const auto& pair: cs.props) {
    if (pair.second.type != type_annotation_type::literal) {
      throw std::runtime_error("cannot handle non-literal type annotations");
    }
    const auto& tt = st.literal_type_annotations[pair.second.value];
    os << "  " << pair.first << ": " << tt.identifier << ',' << std::endl;
  }
  os << "}" << std::endl << std::endl;
}

void write_mount(
  const store& st,
  const component_structure& cs,
  const std::string& indent,
  std::ostream& os
) {
  
}

void write_typescript(const store& st, std::ostream& os) {
  os
    << "/* Generated with the `minimale` tool. */" << std::endl
    << std::endl;
  for (auto st_id: st.unit.statements) {
    if (st_id.type == statement_type::component) {
      const auto& cp = st.components[st_id.value];
      auto cs = get_structure(st, st_id.value);
      auto props_type_name = cp.name + "Props";
      write_props_type(props_type_name, st, cs, os);
      os
        << "export default class " << cp.name << " {" << std::endl
        << "  private root: HTMLElement;" << std::endl << std::endl
        << "  constructor(root: HTMLElement, initialProps: "
          << props_type_name << ") {" << std::endl
        << "    this.root = root;" << std::endl;
      write_mount(st, cs, "   ", os);
      os
        << "  }" << std::endl << std::endl
        << "  unmount(): void {" << std::endl
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
