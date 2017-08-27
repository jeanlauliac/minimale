#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');

function main() {
  const depPath = process.argv[2];
  const outPath = process.argv[3];
  const isCpp = path.extname(outPath) === '.cpp';
  const outDirname = path.dirname(outPath);
  const inPath = process.argv[4];
  const inDirname = path.dirname(inPath);
  const deps = fs.createWriteStream(depPath);
  deps.write(`${outPath}: ${inPath} ${path.relative(process.cwd(), __filename)}\n`);
  deps.end();
  const input = JSON.parse(fs.readFileSync(inPath, 'utf8'));
  const st = fs.createWriteStream(outPath);
  if (!isCpp) st.write('#pragma once\n\n');
  if (isCpp) {
    for (const name of input.include) {
      const incPath = path.join(inDirname, name);
      st.write(`#include "${path.relative(outDirname, incPath)}"\n`);
    }
    st.write('\n');
  } else {
    st.write(`#include <memory>\n`);
    st.write('\n');
  }
  for (const name of input.namespace) {
    st.write(`namespace ${name} {\n`);
  }
  st.write(`\n`);
  if (!isCpp) {
    const typeset = new Set();
    for (const [name, types] of Object.entries(input.variant_types)) {
      for (const [i, typename] of types.entries()) {
        if (typeset.has(typename)) continue;
        st.write(`struct ${typename};\n`);
        typeset.add(typename);
      }
    }
    st.write(`\n`);
  }
  for (const [name, types] of Object.entries(input.variant_types)) {
    if (!isCpp) st.write(`struct ${name} {\n`);
    const funcPrefix = isCpp ? `${name}::` : '';
    const indent = isCpp ? '' : '  ';
    st.write(`${indent}${funcPrefix}${name}()${isCpp ? ': p_(nullptr), t_(-1) {}' : ';'}\n`);
    //if (!isCpp) st.write(`${indent}${name}(const ${name}&);\n`);
    //st.write(`${indent}${funcPrefix}${name}(${name}&& x)${isCpp ? ': p_(std::move(x.p_)), t_(x.t_) {}' : ';'}\n`);
    //if (!isCpp) st.write(`${indent}${name}& ${funcPrefix}operator=(const ${name}& x);\n`);
    //st.write(`${indent}${name}& ${funcPrefix}operator=(${name}&& x)${isCpp ? ' { p_ = std::move(x.p_); t_ = x.t_; return *this; }' : ';'}\n`);
    st.write(`\n`);
    for (const [i, typename] of types.entries()) {
      st.write(`${indent}${funcPrefix}${name}(${typename}&& value)${isCpp ? ':' : ';'}\n`)
      if (isCpp) st.write(`    p_(reinterpret_cast<void*>(new ${typename}(value)), &${name}::delete_${typename}_), t_(${i}) {}\n`);
      if (!isCpp) {
        st.write(`${indent}bool is_${typename}() const { return t_ == ${i}; }\n`);
        st.write(`${indent}const ${typename}& as_${typename}() const { return *reinterpret_cast<${typename}*>(p_.get()); }\n`);
        st.write(`${indent}${typename}& as_${typename}() { return *reinterpret_cast<${typename}*>(p_.get()); }\n\n`);
      }
    }
    if (!isCpp) st.write(`private:\n`);
    for (const [i, typename] of types.entries()) {
      st.write(`${indent}${isCpp ? '' : 'static '}void ${funcPrefix}delete_${typename}_(void* p)${isCpp ? ` { delete reinterpret_cast<${typename}*>(p); }` : ';'}\n`);
    }
    st.write(`\n`);
    if (!isCpp) {
      st.write(`  std::shared_ptr<void> p_;\n`);
      st.write(`  char t_;\n`);
    }
    if (!isCpp) st.write(`};\n\n`);
  }
  for (const name of input.namespace) {
    st.write('}\n');
  }
  st.end();
}

main();
