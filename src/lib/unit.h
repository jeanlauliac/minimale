#pragma once

#include <vector>
#include <string>

namespace minimale {

template <typename Type>
struct id_of {
  id_of() {}
  id_of(Type type, size_t value): type(type), value(value) {}
  Type type;
  size_t value;
};

enum class statement_type { component };
typedef id_of<statement_type> statement_id;

enum class component_statement_type { field, function };
typedef id_of<component_statement_type> component_statement_id;

enum class type_annotation_type { object, literal };
typedef id_of<type_annotation_type> type_annotation_id;

enum class function_statement_type { return_ };
typedef id_of<function_statement_type> function_statement_id;

enum class expression_type { reference, member_access, xml_tag };
typedef id_of<expression_type> expression_id;

enum class xml_fragment_type { text, interpolation };
typedef id_of<xml_fragment_type> xml_fragment_id;

struct component {
  component(
    const std::string& name,
    std::vector<component_statement_id>&& statements
  ): name(name), statements(statements) {}
  std::string name;
  std::vector<component_statement_id> statements;
};

struct field {
  field(
    const std::string& name,
    const type_annotation_id& type_annotation
  ): name(name), type_annotation(type_annotation) {}
  std::string name;
  type_annotation_id type_annotation;
};

struct function {
  function(
    const std::string& name,
    std::vector<minimale::function_statement_id>&& sts
  ): name(name), statements(std::move(sts)) {}
  std::string name;
  std::vector<minimale::function_statement_id> statements;
};

struct unit {
  std::vector<statement_id> statements;
};

struct object_type_annotation_field {
  object_type_annotation_field() {}
  object_type_annotation_field(
    const std::string& name,
    const type_annotation_id& type_annotation
  ): name(name), type_annotation(type_annotation) {}
  std::string name;
  type_annotation_id type_annotation;
};

struct object_type_annotation {
  object_type_annotation(std::vector<object_type_annotation_field>&& fields):
    fields(std::move(fields)) {}
  std::vector<object_type_annotation_field> fields;
};

struct literal_type_annotation {
  literal_type_annotation(const std::string& ident): identifier(ident) {}
  std::string identifier;
};

struct return_statement {
  return_statement(const expression_id& xp): expression(xp) {}
  expression_id expression;
};

struct member_access {
  member_access(const expression_id& xp, const std::string ident):
    expression(xp), identifier(ident) {}
  expression_id expression;
  std::string identifier;
};

struct xml_tag {
  xml_tag(
    const std::string& tag_name,
    std::vector<xml_fragment_id>&& fragments
  ): tag_name(tag_name), fragments(std::move(fragments)) {}
  std::string tag_name;
  std::vector<xml_fragment_id> fragments;
};

struct store {
  unit unit;
  std::vector<component> components;
  std::vector<field> fields;
  std::vector<function> functions;
  std::vector<literal_type_annotation> literal_type_annotations;
  std::vector<object_type_annotation> object_type_annotations;
  std::vector<return_statement> return_statements;
  std::vector<std::string> references;
  std::vector<member_access> member_accesses;
  std::vector<xml_tag> xml_tags;
  std::vector<std::string> xml_text_fragments;
  std::vector<expression_id> xml_interpolations;

  statement_id create_component(
    const std::string& name,
    std::vector<component_statement_id>&& statements
  );
  component_statement_id create_component_field(
    const std::string& name,
    const type_annotation_id& type_annotation
  );
  component_statement_id create_component_function(
    const std::string& name,
    std::vector<minimale::function_statement_id>&& sts
  );
  type_annotation_id create_literal_type_annotation(const std::string& ident);
  type_annotation_id create_object_type_annotation(
    std::vector<object_type_annotation_field>&& fields
  );
  function_statement_id create_return_statement(const expression_id& xp);
  expression_id create_reference(const std::string& ident);
  expression_id
  create_member_access(const expression_id& xp, const std::string ident);
  expression_id create_xml_tag(
    const std::string& tag_name,
    std::vector<xml_fragment_id>&& fragments
  );
  xml_fragment_id create_xml_text_fragment(const std::string& text);
  xml_fragment_id create_xml_interpolation(const expression_id& xp);
};

}
