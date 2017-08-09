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

struct component {
  component(const std::string& name): name(name) {}

  std::string name;
};

struct unit {
  std::vector<statement_id> statements;
};

struct store {
  unit unit;
  std::vector<component> components;

  statement_id create_component(const std::string& name);
};

}
