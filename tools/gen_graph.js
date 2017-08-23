#!/usr/bin/env node
'use strict';

const fs = require('fs');

function main() {
  const outPath = process.argv[2];
  const input = JSON.parse(fs.readFileSync(process.argv[3], 'utf8'));
  const st = fs.createWriteStream(outPath);
  st.write('#pragma once\n\n');
  for (const name of input.namespace) {
    st.write(`namespace ${name} {\n`);
  }
  st.write(`\n`);
  st.write('struct graph {\n');
  for (const [name, node] of Object.entries(input.node_types)) {
    st.write(`  struct ${name} {\n`);
    st.write(`    ${name}(graph* g, size_t id): g_(g), id_(id_) {}\n`);
    for (const [fname, ftype] of Object.entries(node.fields)) {
      st.write(`    const ${cpp_type_for(ftype)}& ${fname}() const { return data_().${fname}; }\n`);
      st.write(`    ${cpp_type_for(ftype)}& ${fname}() { return data_().${fname}; }\n`);
    }
    for (const [ename, etype] of Object.entries(node.edges || {})) {
      st.write(`    std::vector<${etype}> ${ename}() const {\n`);
      st.write(`      std::vector<${etype}> result;\n`);
      st.write(`      for (const auto& edge: data_().statements) {\n`);
      st.write(`        result.emplace_back(edge);\n`);
      st.write(`      }\n`);
      st.write(`      return result;\n`);
      st.write(`    }\n`);
        //` return data_().${ename}; }\n`);
    }
    st.write(`\n`);
    st.write(`  private:\n`);
    st.write(`    const ${name}_data& data_() const { return *(g_->${name}_map_->find(id_)); }\n`);
    st.write(`    ${name}_data& data_() { return *(g_->${name}_map_->find(id_)); }\n`);
    st.write(`    graph* g_;\n`);
    st.write(`    size_t id_;\n`);
    st.write(`  };\n\n`);
  }
  st.write('  graph(): id_count_(0) {};\n\n');
  for (const [name, node] of Object.entries(input.node_types)) {
    st.write(`  ${name} add_${name}();\n`);
  }
  st.write('private:\n');
  for (const [name, node] of Object.entries(input.node_types)) {
    st.write(`  struct ${name}_data {\n`);
    for (const [fname, ftype] of Object.entries(node.fields)) {
      st.write(`    ${cpp_type_for(ftype)} ${fname};\n`);
    }
    st.write(`  };\n\n`);
  }
  st.write('  size_t id_count_;\n');
  for (const [name, node] of Object.entries(input.node_types)) {
    st.write(`  std::unordered_map<size_t, ${name}> ${name}_map_;\n`);
  }
  st.write('};\n\n');
  for (const name of input.namespace) {
    st.write('}\n');
  }
}

function cpp_type_for(type) {
  switch (type) {
    case 'string':
      return 'std::string';
  }
  throw new Error(`unknown type ${type}`);
}

main();
