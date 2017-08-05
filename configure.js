#!/usr/bin/env node

const {Manifest} = require('@jeanlauliac/upd-configure');
const {execSync} = require('child_process');

const manifest = new Manifest();
const BUILD_DIR = '.build_files';

function getFlexCli() {
  const binPath = execSync('which flex', {encoding: 'utf8'}).split('\n')[0];
  const version =execSync(`${binPath} --version`, {encoding: 'utf8'})
    .split('\n')[0];
  if (!/^flex 2.5/.test(version)) {
    throw new Error('Flex 2.5 is required');
  }
  return manifest.cli_template(binPath, [
    {literals: ["-o"], variables: ["output_file", "input_files"]},
  ]);
}

function getBisonCli() {
  const binPath = execSync('which bison', {encoding: 'utf8'}).split('\n')[0];
  const version = execSync(`${binPath} --version`, {encoding: 'utf8'})
    .split('\n')[0];
  if (!/^bison \(GNU Bison\) 2\.3$/.test(version)) {
    throw new Error('GNU Bison 2.3 is required');
  }
  return manifest.cli_template(binPath, [
    {literals: ["-do"], variables: ["output_file", "input_files"]},
  ]);
}

const compiled_flex_files = manifest.rule(getFlexCli(), [
  manifest.source("(src/**/*.lx)"),
], `${BUILD_DIR}/($1).cpp`);

const compiled_bison_files = manifest.rule(getBisonCli(), [
  manifest.source("(src/**/*.bs)"),
], `${BUILD_DIR}/($1).cpp`);

const compile_cpp_cli = manifest.cli_template('clang++', [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: ["-std=c++14", "-Wall", "-fcolor-diagnostics", "-MMD", "-MF"],
    variables: ["dependency_file"]
  },
  {
    literals: ["-I", "/usr/local/include"],
    variables: ["input_files"],
  },
]);

const compiled_cpp_files = manifest.rule(compile_cpp_cli, [
  manifest.source("(src/**/*).cpp"),
], `${BUILD_DIR}/($1).o`, [compiled_bison_files]);

const compile_flex_cpp_cli = manifest.cli_template('clang++', [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: ["-std=c++14", "-Wall", "-Wno-deprecated-register",
      "-Wno-unused-function", "-Wno-unneeded-internal-declaration",
      "-fcolor-diagnostics", "-MMD", "-MF"],
    variables: ["dependency_file"]
  },
  {
    literals: ["-I", "/usr/local/include"],
    variables: ["input_files"],
  },
]);

const compiled_flex_cpp_files = manifest.rule(compile_flex_cpp_cli, [
  compiled_flex_files,
], `${BUILD_DIR}/($1).o`);

const compiled_bison_cpp_files = manifest.rule(compile_flex_cpp_cli, [
  compiled_bison_files,
], `${BUILD_DIR}/($1).o`);

manifest.rule(
  manifest.cli_template('clang++', [
    {literals: ["-o"], variables: ["output_file"]},
    {
      literals: ['-Wall', '-std=c++14',
        '-fcolor-diagnostics', '-L', '/usr/local/lib'],
      variables: ["input_files"]
    },
  ]),
  [compiled_cpp_files, compiled_flex_cpp_files, compiled_bison_cpp_files],
  "dist/minimale"
);

manifest.export(__dirname);
