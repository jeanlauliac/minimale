#include "unit.h"

namespace minimale {

statement_id store::create_component(
  const std::string& name,
  std::vector<component_statement_id>&& statements
) {
  components.emplace_back(name, std::move(statements));
  return statement_id(statement_type::component, components.size() - 1);
}

component_statement_id store::create_component_field(
  const std::string& name,
  const type_annotation_id& type_annotation
) {
  fields.emplace_back(name, type_annotation);
  return component_statement_id(component_statement_type::field, fields.size() - 1);
}

component_statement_id store::create_component_function(
  const std::string& name,
  std::vector<minimale::function_statement_id>&& sts
) {
  functions.emplace_back(name, std::move(sts));
  return component_statement_id(component_statement_type::function, functions.size() - 1);
}

type_annotation_id store::create_literal_type_annotation(const std::string& ident) {
  literal_type_annotations.emplace_back(ident);
  return type_annotation_id(type_annotation_type::literal, literal_type_annotations.size() - 1);
}

type_annotation_id store::create_object_type_annotation(
  std::vector<object_type_annotation_field>&& fields
) {
  object_type_annotations.emplace_back(std::move(fields));
  return type_annotation_id(type_annotation_type::object, object_type_annotations.size() - 1);
}

function_statement_id store::create_return_statement() {
  return_statements.emplace_back();
  return function_statement_id(function_statement_type::return_, return_statements.size() - 1);
}

}
