#include "../.build_files/src/lib/parsing.bs.hpp"
#include "lib/unit.h"
#include <iostream>
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

void write_javascript(const store& st, std::ostream& os) {
  os << "/* Generated with the `minimale` tool. */" << std::endl;
  os <<"'use strict';" << std::endl << std::endl;
  for (auto st_id: st.unit.statements) {
    if (st_id.type == statement_type::component) {
      component cp = st.components[st_id.value];
      auto modelTypeName = cp.name + "Model";
      os
        << "export type " << modelTypeName << " = {" << std::endl
        << "}" << std::endl << std::endl
        << "export default class " << cp.name << " {" << std::endl
        << "  _root: HTMLElement" << std::endl << std::endl
        << "  static mount(root: HTMLElement, model: "
          << modelTypeName << "): void {" << std::endl
        << "    this._root = root;" << std::endl
        << "    this._model = model;" << std::endl
        << "  }" << std::endl << std::endl
        << "  unmount(): void {" << std::endl
        << "  }" << std::endl
        << "}" << std::endl;
    }
  }
}

int run(int argc, char *argv[]) {
  if (argc < 2) {
    throw std::runtime_error("input file must be specified");
  }
  store st = read_store(argv[1]);
  write_javascript(st, std::cout);
  return 0;
}

}

int main(int argc, char *argv[]) {
  return minimale::run(argc, argv);
}
