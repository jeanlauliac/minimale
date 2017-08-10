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
  function(const std::string& name): name(name) {}
  std::string name;
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

struct store {
  unit unit;
  std::vector<component> components;
  std::vector<field> fields;
  std::vector<function> functions;
  std::vector<literal_type_annotation> literal_type_annotations;
  std::vector<object_type_annotation> object_type_annotations;

  statement_id create_component(
    const std::string& name,
    std::vector<component_statement_id>&& statements
  );
  component_statement_id create_component_field(
    const std::string& name,
    const type_annotation_id& type_annotation
  );
  component_statement_id create_component_function(const std::string& name);
  type_annotation_id create_literal_type_annotation(const std::string& ident);
  type_annotation_id create_object_type_annotation(
    std::vector<object_type_annotation_field>&& fields
  );
};

}
