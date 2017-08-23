#pragma once

namespace minimale {
namespace lang {

struct graph {
  struct unit {
    unit(graph* g, size_t id): g_(g), id_(id_) {}
    std::vector<unit_statement> statements() const {
      std::vector<unit_statement> result;
      for (const auto& edge: data_().statements) {
        result.emplace_back(edge);
      }
      return result;
    }

  private:
    const unit_data& data_() const { return *(g_->unit_map_->find(id_)); }
    unit_data& data_() { return *(g_->unit_map_->find(id_)); }
    graph* g_;
    size_t id_;
  };

  struct component {
    component(graph* g, size_t id): g_(g), id_(id_) {}
    const std::string& name() const { return data_().name; }
    std::string& name() { return data_().name; }

  private:
    const component_data& data_() const { return *(g_->component_map_->find(id_)); }
    component_data& data_() { return *(g_->component_map_->find(id_)); }
    graph* g_;
    size_t id_;
  };

  graph(): id_count_(0) {};

  unit add_unit();
  component add_component();
private:
  struct unit_data {
  };

  struct component_data {
    std::string name;
  };

  size_t id_count_;
  std::unordered_map<size_t, unit> unit_map_;
  std::unordered_map<size_t, component> component_map_;
};

}
}
