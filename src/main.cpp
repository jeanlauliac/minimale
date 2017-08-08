#include "../.build_files/src/lib/parsing.bs.hpp"
#include <iostream>
#include <memory>

extern FILE* yyin;

namespace minimale {

typedef std::unique_ptr<FILE, decltype(&fclose)> unique_file;

int run(int argc, char *argv[]) {
  unique_file file(fopen(argv[1], "r"), fclose);
  if (!file) {
    throw std::runtime_error("cannot open file");
  }
  yyin = file.get();

  yy::parser pr;
  pr.set_debug_level(true);
  return pr.parse();
}

}

int main(int argc, char *argv[]) {
  return minimale::run(argc, argv);
}
