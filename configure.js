#!/usr/bin/env node

const {Manifest} = require('@jeanlauliac/upd-configure');
const {execSync} = require('child_process');

const manifest = new Manifest();
const BUILD_DIR = 'gen';

function getFlexCli() {
  const binPath = execSync('which flex', {encoding: 'utf8'}).split('\n')[0];
  const version =execSync(`${binPath} --version`, {encoding: 'utf8'})
    .split('\n')[0];
  if (!/^flex 2\.6\.4/.test(version)) {
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
  if (!/^bison \(GNU Bison\) 3\.0\.4$/.test(version)) {
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

const variant_cli = manifest.cli_template('tools/gen_variants.js', [
  {literals: [], variables: ["dependency_file", "output_file", "input_files"]},
]);

const compiled_variant_h_files = manifest.rule(variant_cli, [
  manifest.source("(src/lib/lang_variants.json)"),
], `${BUILD_DIR}/($1).h`);

const compiled_variant_cpp_files = manifest.rule(variant_cli, [
  manifest.source("(src/lib/lang_variants.json)"),
], `${BUILD_DIR}/($1).cpp`);

const clangPath = execSync('which clang++', {encoding: 'utf8'}).split('\n')[0];

const compile_cpp_cli = manifest.cli_template(clangPath, [
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
  compiled_variant_cpp_files,
], `${BUILD_DIR}/($1).o`, [compiled_bison_files, compiled_variant_h_files]);

const compile_flex_cpp_cli = manifest.cli_template(clangPath, [
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
], `${BUILD_DIR}/($1).o`, [compiled_bison_files, compiled_variant_h_files]);

const compiled_bison_cpp_files = manifest.rule(compile_flex_cpp_cli, [
  compiled_bison_files,
], `${BUILD_DIR}/($1).o`, [compiled_variant_h_files]);

const minimale_binary = manifest.rule(
  manifest.cli_template(clangPath, [
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

const compile_mn_cli = manifest.cli_template('dist/minimale', [
  {variables: ["input_files", "output_file", "dependency_file"]},
  {literals: ['dist/minimale']}
]);

const typescript_examples = manifest.rule(compile_mn_cli, [
  manifest.source('examples/(*).mn'),
], `${BUILD_DIR}/example/($1).ts`, [minimale_binary]);

const compile_typescript_cli = manifest.cli_template('node_modules/.bin/tsc', [
  {
    literals: ['--module', 'CommonJS', '--outDir', 'dist/example'],
    variables: ["input_files"],
  },
]);

manifest.rule(
  compile_typescript_cli,
  [typescript_examples],
  `dist/example/$1.js`
);

manifest.rule(
  manifest.cli_template('/bin/cp', [{variables: ["input_files", "output_file"]}]),
  [manifest.source('examples/(*.html)')],
  `dist/example/$1`
);

manifest.export(__dirname);
