#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');

function main() {
  const depPath = process.argv[2];
  const outPath = process.argv[3];
  const outDirname = path.dirname(outPath);
  const inPath = process.argv[4];
  const inDirname = path.dirname(inPath);
  const deps = fs.createWriteStream(depPath);
  deps.write(`${outPath}: ${inPath} ${path.relative(process.cwd(), __filename)}\n`);
  deps.end();
  const input = JSON.parse(fs.readFileSync(inPath, 'utf8'));
  const st = fs.createWriteStream(outPath);
  st.write('#pragma once\n\n');
  for (const name of input.include) {
    const incPath = path.join(inDirname, name);
    st.write(`#include "${path.relative(outDirname, incPath)}"\n`);
  }
  st.write('\n');
  for (const name of input.namespace) {
    st.write(`namespace ${name} {\n`);
  }
  st.write(`\n`);
  for (const [name, types] of Object.entries(input.variant_types)) {
    st.write(`struct ${name} {\n`);
    st.write(`  ${name} (): p_(nullptr, nullptr), t_(-1) {}\n`);
    st.write(`  ${name} (const ${name}&) = delete;\n`);
    st.write(`  ${name} (${name}&& x): p_(std::move(x.p_)), t_(x.t_) {}\n`);
    st.write(`  ${name}& operator=(const ${name}& x) = delete;\n`);
    st.write(`  ${name}& operator=(${name}&& x) { p_ = std::move(x.p_); t_ = x.t_; return *this; };\n`);
    st.write(`\n`);
    for (const [i, typename] of types.entries()) {
      st.write(`  ${name}(${typename}&& value):\n`)
      st.write(`    p_(reinterpret_cast<void*>(new ${typename}(value)), &delete_${typename}_), t_(${i}) {}\n`);
      st.write(`  bool is_${typename}() const { return t_ == ${i}; }\n`);
      st.write(`  const ${typename}& as_${typename}() const { return *reinterpret_cast<${typename}*>(p_.get()); }\n`);
      st.write(`  ${typename}& as_${typename}() { return *reinterpret_cast<${typename}*>(p_.get()); }\n\n`);
    }
    st.write(`private:\n`);
    for (const [i, typename] of types.entries()) {
      st.write(`  static void delete_${typename}_(void* p) { delete reinterpret_cast<${typename}*>(p); }\n`);
    }
    st.write(`\n`);
    st.write(`  std::unique_ptr<void, void(*)(void*)> p_;\n`);
    st.write(`  char t_;\n`);
    st.write(`};\n\n`);
  }
  for (const name of input.namespace) {
    st.write('}\n');
  }
  st.end();
}

main();
