const fs = require('fs');
const path = require('path');
const assert = require('assert');
const { BUILTIN_MODULES, MODULE_MEMBERS } = require('./language-data');
const { parseSeCheckOutput } = require('./diagnostics');

const root = path.resolve(__dirname, '..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');

function quotedNames(text) {
  return [...text.matchAll(/"([A-Za-z_][A-Za-z0-9_]*)"/g)].map((match) => match[1]);
}

function runtimeModules() {
  const ecosystem = read('src/runtime/ecosystem.cpp');
  const platform = read('src/runtime/platform.cpp');
  const advanced = read('src/runtime/advanced.cpp');
  const expansion = read('src/runtime/expansion.cpp');
  const formats = read('src/runtime/formats.cpp');

  const ecosystemBlock = ecosystem.match(/bool is_ecosystem_builtin[\s\S]*?names=\{([\s\S]*?)\};return names\.contains/);
  const platformBlock = platform.match(/bool is_platform_builtin[\s\S]*?names=\{([\s\S]*?)\};return names\.contains/);
  const advancedBlock = advanced.match(/bool is_advanced_builtin[\s\S]*?\{([\s\S]*?)\n\}/);
  const expansionBlock = expansion.match(/bool is_expansion_builtin[\s\S]*?names=\{([\s\S]*?)\};return names\.contains/);
  const formatsBlock = formats.match(/bool is_format_builtin\([^)]*\)\{return ([\s\S]*?);\}/);

  assert(ecosystemBlock, 'Could not parse ecosystem built-in module list.');
  assert(platformBlock, 'Could not parse platform built-in module list.');
  assert(advancedBlock, 'Could not parse advanced built-in module list.');
  assert(expansionBlock, 'Could not parse expansion built-in module list.');
  assert(formatsBlock, 'Could not parse format built-in module list.');

  const names = new Set([
    'file', 'path', 'time', 'math', 'random', 'os',
    ...quotedNames(ecosystemBlock[1]),
    ...quotedNames(platformBlock[1]),
    ...[...advancedBlock[1].matchAll(/name=="([^"]+)"/g)].map((match) => match[1]),
    ...quotedNames(expansionBlock[1]),
    ...[...formatsBlock[1].matchAll(/name=="([^"]+)"/g)].map((match) => match[1])
  ]);
  return [...names].sort();
}

const runtime = runtimeModules();
const extension = [...BUILTIN_MODULES].sort();
assert.deepStrictEqual(
  extension,
  runtime,
  `VS Code built-in module list differs from runtime.\nRuntime: ${runtime.join(', ')}\nExtension: ${extension.join(', ')}`
);

assert.strictEqual(new Set(BUILTIN_MODULES).size, BUILTIN_MODULES.length, 'Built-in module list contains duplicates.');
for (const name of BUILTIN_MODULES) {
  assert(Array.isArray(MODULE_MEMBERS[name]), `Missing MODULE_MEMBERS entry for ${name}.`);
  assert(MODULE_MEMBERS[name].length > 0, `Module ${name} has no IntelliSense members.`);
  const members = MODULE_MEMBERS[name];
  assert.strictEqual(new Set(members.map(([member]) => member)).size, members.length, `Duplicate members in ${name}.`);
}

const independentPackages = {
  re: 'regex', itertools: 'iter', hashlib: 'hash', argparse: 'args',
  logging: 'log', zipfile: 'zip', sqlite3: 'sqlite', config: 'dotenv',
  series: 'array', linear: 'matrix', dataset: 'table',
  http_server: 'web', router: 'web', dns: 'socket',
  gui: 'game', window: 'game', canvas: 'game', input: 'game',
  sprite: 'game', physics: 'game', sound: 'game', keyboard: 'game',
  mouse: 'game', animation: 'game', scene: 'game', collision: 'game',
  image: 'game', audio: 'game'
};
for (const name of Object.keys(independentPackages)) {
  assert(BUILTIN_MODULES.includes(name), `Independent package ${name} is not built in.`);
  assert(MODULE_MEMBERS[name] !== MODULE_MEMBERS[independentPackages[name]], `${name} still shares an alias member array.`);
  assert(MODULE_MEMBERS[name].length >= 6, `${name} needs a useful standalone API.`);
}
assert(!Object.hasOwn(require('./language-data'), 'ALIASES'), 'Alias registry should be removed.');

// Every native expansion operation should appear in the editor's member completions.
const expansionSource = read('src/runtime/expansion.cpp');
for (const match of expansionSource.matchAll(/if\(name=="([a-z_]+)"(?:\|\|name=="[a-z_]+")*\)return \{([\s\S]*?)\n \};/g)) {
  const module = match[1];
  for (const [, member] of match[2].matchAll(/^  \{"([a-z_]+)",\{/gm)) {
    assert(MODULE_MEMBERS[module].some(([name]) => name === member), `Missing ${module}.${member} completion.`);
  }
}
const gameApiOwners = {
  create: 'window', title: 'window', resize: 'window', fullscreen: 'window',
  image: 'canvas', text: 'canvas', rect: 'canvas', circle: 'canvas', line: 'canvas'
};
for (const [, member] of read('src/runtime/game_ext.cpp').matchAll(/x\["([a-z_]+)"\]=/g)) {
  const owner = gameApiOwners[member];
  if (owner) assert(MODULE_MEMBERS[owner].some(([name]) => name === member), `Missing ${owner}.${member} completion.`);
}

const grammar = JSON.parse(read('vscode/syntaxes/se.tmLanguage.json'));
assert.strictEqual(grammar.scopeName, 'source.se');
const modulePattern = grammar.repository['builtin-modules'].patterns[0].match;
const moduleRegex = new RegExp(modulePattern);
for (const name of BUILTIN_MODULES) {
  assert(moduleRegex.test(name), `TextMate grammar does not highlight module ${name}.`);
}
const grammarNames = modulePattern.match(/\(\?:([^)]*)\)/)[1].split('|').sort();
assert.deepStrictEqual(grammarNames, runtime, 'TextMate grammar module names differ from runtime.');

const pkg = JSON.parse(read('vscode/package.json'));
assert.strictEqual(pkg.version, '0.7.2');
for (const command of ['se.run', 'se.check', 'se.checkProblems', 'se.build', 'se.openGuide']) {
  assert(pkg.contributes.commands.some((entry) => entry.command === command), `Missing VS Code command ${command}.`);
}
assert(pkg.contributes.configuration.properties['se.diagnostics.enabled']);
assert(pkg.contributes.configuration.properties['se.diagnostics.delay']);

JSON.parse(read('vscode/snippets/se.json'));
assert(fs.existsSync(path.join(root, 'vscode/TUTORIAL-zh-TW.md')), 'Bundled syntax tutorial is missing.');

const parsed = parseSeCheckOutput([
  'Error on line 7',
  '',
  "Unknown name 'scroe'.",
  '',
  '7 | say scroe',
  '  |     ^',
  '',
  "Try: Check the variable name."
].join('\n'));

assert(parsed, 'Diagnostic parser returned no result.');
assert.strictEqual(parsed.line, 6);
assert.strictEqual(parsed.column, 4);
assert.strictEqual(parsed.message, "Unknown name 'scroe'.");
assert.strictEqual(parsed.hint, 'Check the variable name.');

console.log(`VS Code extension validation passed: ${BUILTIN_MODULES.length} built-in module names synchronized with runtime.`);
