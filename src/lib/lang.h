#pragma once

#include <vector>

namespace minimale {
namespace lang {

struct unit_stmt;
struct comp_stmt;
struct expr;
struct type_annot;

struct ltr_string { std::string val; };
struct ltr_int { size_t val; };
struct type_ident { std::string ident; };

struct func_arg {
  std::string name;
  type_annot type;
};

struct comp {
  std::string name;
  std::vector<comp_stmt> stmts;
};

struct comp_field {
  std::string name;
  type_annot type;
};

struct comp_func {
  std::string name;
  std::vector<func_arg> args;
};

struct unit {
  std::vector<unit_stmt> stmts;
};

struct xml_text { std::string text; }
struct xml_interp { expr expr; };
struct xml_tag { std::string name; std::vector<xml_frag> frags; };

}
}
