#pragma once

#include "../../.build_files/src/lib/lang_variants.json.h"
#include <vector>

namespace minimale {
namespace lang {

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
  std::vector<func_stmt> stmts;
};

struct unit {
  std::vector<unit_stmt> stmts;
};

struct xml_text { std::string text; };
struct xml_interp { expr expr; };
struct xml_tag { std::string name; std::vector<xml_frag> frags; };

struct obj_type_field {
  std::string name;
  type_annot type;
};

struct obj_type_annot {
  std::vector<obj_type_field> fields;
};

struct return_stmt {
  expr expr;
};

struct assigment {
  expr assigned;
  expr value;
};

struct ref {
  std::string ident;
};

struct member_access {
  expr expr;
  std::string member;
};

}
}
