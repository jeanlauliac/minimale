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
  std::vector<minimale::function_argument>&& args,
  std::vector<minimale::function_statement_id>&& sts
) {
  functions.emplace_back(name, std::move(args), std::move(sts));
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

function_statement_id store::create_return_statement(const expression_id& xp) {
  return_statements.emplace_back(xp);
  return function_statement_id(function_statement_type::return_, return_statements.size() - 1);
}

function_statement_id store::create_assignment(
  const expression_id& into,
  const expression_id& from
) {
  assignments.emplace_back(into, from);
  return function_statement_id(function_statement_type::assignment, assignments.size() - 1);
}

expression_id store::create_reference(const std::string& ident) {
  references.push_back(ident);
  return expression_id(expression_type::reference, references.size() - 1);
}

expression_id store::create_member_access(const expression_id& xp, const std::string ident) {
  member_accesses.emplace_back(xp, ident);
  return expression_id(expression_type::member_access, member_accesses.size() - 1);
}

expression_id store::create_string(const std::string& value) {
  strings.emplace_back(value);
  return expression_id(expression_type::string, strings.size() - 1);
}

expression_id store::create_xml_tag(
  const std::string& tag_name,
  std::vector<xml_fragment_id>&& fragments
) {
  xml_tags.emplace_back(tag_name, std::move(fragments));
  return expression_id(expression_type::xml_tag, xml_tags.size() - 1);
}

xml_fragment_id store::create_xml_text_fragment(const std::string& text) {
  xml_text_fragments.push_back(text);
  return xml_fragment_id(xml_fragment_type::text, xml_text_fragments.size() - 1);
}

xml_fragment_id store::create_xml_interpolation(const expression_id& xp) {
  xml_interpolations.push_back(xp);
  return xml_fragment_id(xml_fragment_type::interpolation, xml_interpolations.size() - 1);
}

}
