#include <iostream>

extern "C" int yylex();

namespace minimale {

int run(int argc, char *argv[]) {
  yylex();
  return 0;
}

}

int main(int argc, char *argv[]) {
  return minimale::run(argc, argv);
}
