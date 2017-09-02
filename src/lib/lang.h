#pragma once

#include "../../.build_files/src/lib/lang_variants.json.h"
#include <vector>
#include <string>

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

struct xml_text { std::string val; };
struct xml_interp { expr expr; };

struct xml_attr {
  std::string name;
  expr val;
};

struct xml_tag {
  std::string name;
  std::vector<xml_attr> attrs;
  std::vector<xml_frag> frags;
};

struct obj_type_field {
  std::string name;
  type_annot type;
};

struct obj_type_annot {
  std::vector<obj_type_field> fields;
};

struct ltr_type_annot {
  std::string ident;
};

struct return_stmt {
  expr expr;
};

struct assignment {
  expr assigned;
  expr val;
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
