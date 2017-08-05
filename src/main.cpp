#include "../.build_files/src/lib/parsing.bs.hpp"
#include <iostream>
#include <memory>

extern "C" int yyparse();
extern FILE* yyin;

namespace minimale {

typedef std::unique_ptr<FILE, decltype(&fclose)> unique_file;

int run(int argc, char *argv[]) {
  unique_file file(fopen(argv[1], "r"), fclose);
  if (!file) {
    throw std::runtime_error("cannot open file");
  }
  yyin = file.get();

  do {
    yyparse();
  } while (!feof(yyin));

  return 0;
}

}

void yyerror(const char *s) {
  throw std::runtime_error(std::string("parse error: ") + s);
}

int main(int argc, char *argv[]) {
  return minimale::run(argc, argv);
}
