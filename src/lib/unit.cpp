#include "unit.h"

namespace minimale {

statement_id store::create_component(const std::string& name) {
  components.push_back(component(name));
  return statement_id(statement_type::component, components.size() - 1);
}

}
