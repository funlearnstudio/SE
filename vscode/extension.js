const vscode = require('vscode');
const fs = require('fs/promises');
const path = require('path');
const { execFile } = require('child_process');
const { BUILTIN_MODULES, MODULES, MODULE_DESCRIPTIONS, MODULE_MEMBERS } = require('./language-data');
const { parseSeCheckOutput } = require('./diagnostics');

let seTerminal;
let diagnosticCollection;
let executableWarningShown = false;
const diagnosticTimers = new Map();
const diagnosticGenerations = new Map();

const SE_COMPLETIONS = [
  ['say', 'keyword', 'Output a value.'],
  ['ask', 'keyword', 'Read user input.'],
  ['make', 'keyword', 'Define a function.'],
  ['give', 'keyword', 'Return a value.'],
  ['if', 'keyword', 'Start a conditional branch.'],
  ['else', 'keyword', 'Start an alternative branch.'],
  ['repeat', 'keyword', 'Repeat a block a fixed number of times.'],
  ['for', 'keyword', 'Loop over values.'],
  ['while', 'keyword', 'Repeat while a condition is true.'],
  ['in', 'keyword', 'Used for iteration and membership.'],
  ['use', 'keyword', 'Import an SE module.'],
  ['type', 'keyword', 'Define a user type.'],
  ['match', 'keyword', 'Match a value.'],
  ['case', 'keyword', 'Define a match case.'],
  ['try', 'keyword', 'Handle or propagate a recoverable failure.'],
  ['fail', 'keyword', 'Create a recoverable failure.'],
  ['await', 'keyword', 'Wait for asynchronous work where supported.'],
  ['wait', 'keyword', 'Pause for a Duration.'],
  ['and', 'keyword', 'Logical AND.'],
  ['or', 'keyword', 'Logical OR.'],
  ['not', 'keyword', 'Logical NOT.'],
  ['true', 'value', 'Boolean true.'],
  ['false', 'value', 'Boolean false.'],
  ['none', 'value', 'Empty/none literal where accepted.'],
  ...MODULES.map(([name, description]) => [name, 'module', description]),

  ['html', 'web', 'HTML structure inside an SE Web component.'],
  ['css', 'web', 'CSS styles inside an SE Web component.'],
  ['style', 'web', 'Component styling.'],
  ['page', 'web', 'Define a web page.'],
  ['when', 'web', 'Handle an event.'],
  ['native', 'web', 'Embed native browser content in SE Web.']
];

const TYPE_MEMBERS = {
  Text: [
    ['len', 'property', 'Text length.'],
    ['upper', 'method', 'Uppercase Text.'],
    ['lower', 'method', 'Lowercase Text.']
  ],
  List: [
    ['len', 'property', 'List length.']
  ],
  Map: [
    ['len', 'property', 'Map size.']
  ],
  Set: [
    ['len', 'property', 'Set size.']
  ],
  Error: [
    ['message', 'property', 'Human-readable error message.']
  ]
};

function quote(value) {
  return `"${String(value).replace(/"/g, '\\"')}"`;
}

function terminalForRun() {
  const config = vscode.workspace.getConfiguration('se');
  const reuse = config.get('reuseTerminal', true);
  if (reuse) {
    if (!seTerminal || seTerminal.exitStatus) {
      seTerminal = vscode.window.createTerminal('SE');
    }
    return seTerminal;
  }
  return vscode.window.createTerminal('SE');
}

async function run(command) {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== 'se') {
    vscode.window.showErrorMessage('Open an SE .se file first.');
    return;
  }

  if (editor.document.isUntitled) {
    vscode.window.showErrorMessage('Save the SE file before running it.');
    return;
  }

  if (editor.document.isDirty) {
    const saved = await editor.document.save();
    if (!saved) {
      vscode.window.showErrorMessage('Could not save the SE file.');
      return;
    }
  }

  const config = vscode.workspace.getConfiguration('se');
  const executable = config.get('executablePath', 'se');
  const file = editor.document.uri.fsPath;
  const cwd = vscode.workspace.getWorkspaceFolder(editor.document.uri)?.uri.fsPath
    || require('path').dirname(file);

  const terminal = terminalForRun();
  terminal.show(true);
  terminal.sendText(`cd ${quote(cwd)} && ${quote(executable)} ${command} ${quote(file)}`, true);
}

function completionKind(type) {
  switch (type) {
    case 'keyword': return vscode.CompletionItemKind.Keyword;
    case 'module': return vscode.CompletionItemKind.Module;
    case 'web': return vscode.CompletionItemKind.Property;
    case 'value': return vscode.CompletionItemKind.Value;
    case 'function': return vscode.CompletionItemKind.Function;
    case 'method': return vscode.CompletionItemKind.Method;
    case 'property': return vscode.CompletionItemKind.Property;
    case 'type': return vscode.CompletionItemKind.Class;
    case 'variable': return vscode.CompletionItemKind.Variable;
    default: return vscode.CompletionItemKind.Text;
  }
}

function indentOf(text) {
  const match = text.match(/^\s*/);
  return match ? match[0].replace(/\t/g, '    ').length : 0;
}

function stripComment(line) {
  let inString = false;
  let escaped = false;
  for (let i = 0; i < line.length; i++) {
    const ch = line[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (ch === '\\' && inString) {
      escaped = true;
      continue;
    }
    if (ch === '"') {
      inString = !inString;
      continue;
    }
    if (ch === '#' && !inString) return line.slice(0, i);
  }
  return line;
}

function parseFunctionHeader(code) {
  const match = code.match(/^\s*make\s+([A-Za-z_][A-Za-z0-9_]*)(?:\[[^\]]+\])?\s*(.*)$/);
  if (!match) return null;
  const name = match[1];
  const tail = match[2].trim();
  const returnSplit = tail.split(/\s+->\s+/);
  const paramText = returnSplit[0] || '';
  const returnType = returnSplit.length > 1 ? returnSplit[1].trim() : undefined;
  const params = paramText
    ? paramText.split(/\s+/).filter(Boolean).map((token) => {
      const pair = token.split(':');
      return { name: pair[0], type: pair[1] };
    })
    : [];
  return { name, params, returnType };
}

function inferValueType(rhs, model) {
  const text = rhs.trim();
  if (/^"(?:[^"\\]|\\.)*"$/.test(text)) return 'Text';
  if (/^(true|false)$/.test(text)) return 'Bool';
  if (/^-?\d+$/.test(text)) return 'Int';
  if (/^-?(?:\d+\.\d*|\d*\.\d+)$/.test(text)) return 'Num';
  if (/^\[/.test(text)) return 'List';
  if (/^\{/.test(text)) return 'Map';
  const ctor = text.match(/^([A-Z][A-Za-z0-9_]*)\b/);
  if (ctor && model.types.has(ctor[1])) return ctor[1];
  const source = text.match(/^([A-Za-z_][A-Za-z0-9_]*)$/);
  if (source && model.variables.has(source[1])) return model.variables.get(source[1]).type;
  return undefined;
}

function analyzeDocument(document) {
  const model = {
    functions: new Map(),
    types: new Map(),
    variables: new Map(),
    modules: new Set(),
    symbols: []
  };

  let currentType = null;
  let currentTypeIndent = -1;

  for (let lineNumber = 0; lineNumber < document.lineCount; lineNumber++) {
    const raw = document.lineAt(lineNumber).text;
    const code = stripComment(raw);
    const trimmed = code.trim();
    const indent = indentOf(code);

    if (!trimmed) continue;

    if (currentType && indent <= currentTypeIndent) {
      currentType = null;
      currentTypeIndent = -1;
    }

    const useMatch = trimmed.match(/^use\s+([A-Za-z_][A-Za-z0-9_.]*)$/);
    if (useMatch) {
      model.modules.add(useMatch[1]);
      continue;
    }

    const typeMatch = trimmed.match(/^type\s+([A-Za-z_][A-Za-z0-9_]*)(?:\[[^\]]+\])?/);
    if (typeMatch) {
      const name = typeMatch[1];
      const info = {
        name,
        line: lineNumber,
        range: document.lineAt(lineNumber).range,
        fields: new Map(),
        methods: new Map()
      };
      model.types.set(name, info);
      model.symbols.push({ name, kind: vscode.SymbolKind.Class, line: lineNumber });
      currentType = info;
      currentTypeIndent = indent;
      continue;
    }

    const fn = parseFunctionHeader(code);
    if (fn) {
      const info = {
        ...fn,
        line: lineNumber,
        range: document.lineAt(lineNumber).range,
        signature: `${fn.name}${fn.params.length ? ' ' + fn.params.map((p) => p.type ? `${p.name}:${p.type}` : p.name).join(' ') : ''}${fn.returnType ? ` -> ${fn.returnType}` : ''}`
      };
      if (currentType && indent > currentTypeIndent) {
        currentType.methods.set(fn.name, info);
        model.symbols.push({ name: `${currentType.name}.${fn.name}`, kind: vscode.SymbolKind.Method, line: lineNumber });
      } else {
        model.functions.set(fn.name, info);
        model.symbols.push({ name: fn.name, kind: vscode.SymbolKind.Function, line: lineNumber });
      }
      continue;
    }

    const assignment = code.match(/^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$/);
    if (assignment) {
      const name = assignment[1];
      const rhs = assignment[2];
      if (currentType && indent > currentTypeIndent) {
        currentType.fields.set(name, {
          name,
          type: inferValueType(rhs, model),
          line: lineNumber,
          range: document.lineAt(lineNumber).range
        });
        model.symbols.push({ name: `${currentType.name}.${name}`, kind: vscode.SymbolKind.Field, line: lineNumber });
      } else {
        model.variables.set(name, {
          name,
          type: inferValueType(rhs, model),
          line: lineNumber,
          range: document.lineAt(lineNumber).range
        });
        model.symbols.push({ name, kind: vscode.SymbolKind.Variable, line: lineNumber });
      }
    }
  }

  return model;
}

function makeCompletion(label, kind, detail, documentation, sortText) {
  const item = new vscode.CompletionItem(label, kind);
  item.detail = detail;
  item.sortText = sortText || `1-${label}`;
  if (documentation) item.documentation = new vscode.MarkdownString(documentation);
  return item;
}

function moduleMemberItems(moduleName) {
  return (MODULE_MEMBERS[moduleName] || []).map(([name, signature, description, kind = 'function']) => {
    const item = makeCompletion(
      name,
      completionKind(kind),
      `${moduleName}.${signature}`,
      description,
      `0-${name}`
    );
    item.insertText = name;
    return item;
  });
}

function typeMemberItems(typeName, model) {
  const items = [];

  for (const [name, kind, description] of TYPE_MEMBERS[typeName] || []) {
    items.push(makeCompletion(name, completionKind(kind), `${typeName} ${kind}`, description, `0-${name}`));
  }

  const userType = model.types.get(typeName);
  if (userType) {
    for (const [name, field] of userType.fields) {
      items.push(makeCompletion(
        name,
        vscode.CompletionItemKind.Field,
        field.type ? `${typeName}.${name}: ${field.type}` : `${typeName}.${name}`,
        'SE field',
        `0-${name}`
      ));
    }
    for (const [name, method] of userType.methods) {
      items.push(makeCompletion(
        name,
        vscode.CompletionItemKind.Method,
        `${typeName}.${method.signature}`,
        'SE method',
        `0-${name}`
      ));
    }
  }

  return items;
}

function memberTarget(document, position) {
  const prefix = document.lineAt(position.line).text.slice(0, position.character);
  const match = prefix.match(/([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)?$/);
  return match ? match[1] : null;
}

function createCompletionProvider() {
  return {
    provideCompletionItems(document, position) {
      const model = analyzeDocument(document);
      const prefix = document.lineAt(position.line).text.slice(0, position.character);
      const useContext = prefix.match(/^\s*use\s+([A-Za-z_][A-Za-z0-9_.]*)?$/);

      if (useContext) {
        return MODULES.map(([name, description]) => {
          const item = makeCompletion(
            name,
            vscode.CompletionItemKind.Module,
            'SE built-in module',
            description,
            `0-${name}`
          );
          item.insertText = name;
          return item;
        });
      }

      const target = memberTarget(document, position);
      if (target) {
        if (model.modules.has(target) || MODULE_MEMBERS[target]) {
          return moduleMemberItems(target);
        }
        if (model.variables.has(target)) {
          const type = model.variables.get(target).type;
          if (type) return typeMemberItems(type, model);
        }
        if (model.types.has(target)) {
          return typeMemberItems(target, model);
        }
        return [];
      }

      const items = [];

      for (const [word, type, description] of SE_COMPLETIONS) {
        items.push(makeCompletion(
          word,
          completionKind(type),
          `SE ${type}`,
          description,
          `0-${word}`
        ));
      }

      for (const [name, info] of model.functions) {
        items.push(makeCompletion(
          name,
          vscode.CompletionItemKind.Function,
          info.signature,
          'Function defined in this SE file.',
          `1-${name}`
        ));
      }

      for (const [name] of model.types) {
        items.push(makeCompletion(
          name,
          vscode.CompletionItemKind.Class,
          `SE type ${name}`,
          'Type defined in this SE file.',
          `1-${name}`
        ));
      }

      for (const [name, info] of model.variables) {
        items.push(makeCompletion(
          name,
          vscode.CompletionItemKind.Variable,
          info.type ? `${name}: ${info.type}` : 'SE variable',
          'Variable defined in this SE file.',
          `2-${name}`
        ));
      }

      return items;
    }
  };
}

function wordAt(document, position) {
  const range = document.getWordRangeAtPosition(position, /[A-Za-z_][A-Za-z0-9_]*/);
  if (!range) return null;
  return { word: document.getText(range), range };
}

function createHoverProvider() {
  return {
    provideHover(document, position) {
      const found = wordAt(document, position);
      if (!found) return null;
      const model = analyzeDocument(document);
      const { word, range } = found;

      const fn = model.functions.get(word);
      if (fn) {
        return new vscode.Hover(new vscode.MarkdownString(`**SE function**\n\n\`${fn.signature}\``), range);
      }

      const type = model.types.get(word);
      if (type) {
        const lines = [`**SE type ${word}**`];
        if (type.fields.size) lines.push('', `Fields: ${[...type.fields.keys()].map((x) => `\`${x}\``).join(', ')}`);
        if (type.methods.size) lines.push('', `Methods: ${[...type.methods.keys()].map((x) => `\`${x}\``).join(', ')}`);
        return new vscode.Hover(new vscode.MarkdownString(lines.join('\n')), range);
      }

      const variable = model.variables.get(word);
      if (variable) {
        const text = variable.type ? `**SE variable**\n\n\`${word}: ${variable.type}\`` : `**SE variable**\n\n\`${word}\``;
        return new vscode.Hover(new vscode.MarkdownString(text), range);
      }

      const beforeWord = document.lineAt(position.line).text.slice(0, range.start.character);
      const moduleMatch = beforeWord.match(/([A-Za-z_][A-Za-z0-9_]*)\.$/);
      if (moduleMatch) {
        const moduleName = moduleMatch[1];
        const member = (MODULE_MEMBERS[moduleName] || []).find(([name]) => name === word);
        if (member) {
          const [, signature, description] = member;
          return new vscode.Hover(
            new vscode.MarkdownString(`**${moduleName}.${signature}**\n\n${description}`),
            range
          );
        }
      }

      if (MODULE_DESCRIPTIONS[word]) {
        return new vscode.Hover(
          new vscode.MarkdownString(`**SE module ${word}**\n\n${MODULE_DESCRIPTIONS[word]}\n\nUse: \`use ${word}\``),
          range
        );
      }

      const builtin = SE_COMPLETIONS.find(([name]) => name === word);
      if (builtin) {
        return new vscode.Hover(new vscode.MarkdownString(`**SE ${builtin[1]}**\n\n${builtin[2]}`), range);
      }

      return null;
    }
  };
}

function createDefinitionProvider() {
  return {
    provideDefinition(document, position) {
      const found = wordAt(document, position);
      if (!found) return null;
      const model = analyzeDocument(document);
      const { word } = found;
      const info = model.functions.get(word) || model.types.get(word) || model.variables.get(word);
      if (!info) return null;
      return new vscode.Location(document.uri, new vscode.Position(info.line, 0));
    }
  };
}

function createDocumentSymbolProvider() {
  return {
    provideDocumentSymbols(document) {
      const model = analyzeDocument(document);
      const result = [];

      for (const [name, type] of model.types) {
        const symbol = new vscode.DocumentSymbol(
          name,
          'SE type',
          vscode.SymbolKind.Class,
          type.range,
          type.range
        );

        for (const [fieldName, field] of type.fields) {
          symbol.children.push(new vscode.DocumentSymbol(
            fieldName,
            field.type || 'field',
            vscode.SymbolKind.Field,
            field.range,
            field.range
          ));
        }

        for (const [methodName, method] of type.methods) {
          symbol.children.push(new vscode.DocumentSymbol(
            methodName,
            method.signature,
            vscode.SymbolKind.Method,
            method.range,
            method.range
          ));
        }

        result.push(symbol);
      }

      for (const [name, fn] of model.functions) {
        result.push(new vscode.DocumentSymbol(
          name,
          fn.signature,
          vscode.SymbolKind.Function,
          fn.range,
          fn.range
        ));
      }

      for (const [name, variable] of model.variables) {
        result.push(new vscode.DocumentSymbol(
          name,
          variable.type || 'variable',
          vscode.SymbolKind.Variable,
          variable.range,
          variable.range
        ));
      }

      return result;
    }
  };
}

function memberSignatureHelp(line) {
  const match = line.match(/([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*)\s+(.*)$/);
  if (!match) return null;
  const [, moduleName, memberName, argsText] = match;
  const member = (MODULE_MEMBERS[moduleName] || []).find(([name]) => name === memberName);
  if (!member) return null;
  const [, signature, description, kind = 'function'] = member;
  if (kind !== 'function') return null;

  const pieces = signature.split(/\s+/).slice(1);
  const params = pieces.filter((part) => part && part !== '|' && !part.includes('range'));
  if (!params.length) return null;

  const args = argsText.trim() ? argsText.trim().split(/\s+/) : [];
  const activeParameter = Math.max(0, Math.min(params.length - 1, args.length ? args.length - 1 : 0));
  const help = new vscode.SignatureHelp();
  const sig = new vscode.SignatureInformation(`${moduleName}.${signature}`, description);
  sig.parameters = params.map((param) => new vscode.ParameterInformation(param));
  help.signatures = [sig];
  help.activeSignature = 0;
  help.activeParameter = activeParameter;
  return help;
}

function createSignatureHelpProvider() {
  return {
    provideSignatureHelp(document, position) {
      const line = document.lineAt(position.line).text.slice(0, position.character);
      const moduleHelp = memberSignatureHelp(line);
      if (moduleHelp) return moduleHelp;

      const model = analyzeDocument(document);
      let best = null;
      for (const [name, fn] of model.functions) {
        const index = line.lastIndexOf(name);
        if (index < 0) continue;
        const before = index === 0 ? '' : line[index - 1];
        if (before && /[A-Za-z0-9_]/.test(before)) continue;
        if (!best || index > best.index) best = { index, name, fn };
      }

      if (!best || !best.fn.params.length) return null;

      const afterName = line.slice(best.index + best.name.length).trimStart();
      const args = afterName ? afterName.split(/\s+/) : [];
      const activeParameter = Math.max(0, Math.min(best.fn.params.length - 1, args.length ? args.length - 1 : 0));

      const help = new vscode.SignatureHelp();
      const sig = new vscode.SignatureInformation(best.fn.signature, 'SE low-punctuation function call');
      sig.parameters = best.fn.params.map((param) => new vscode.ParameterInformation(
        param.type ? `${param.name}:${param.type}` : param.name
      ));
      help.signatures = [sig];
      help.activeSignature = 0;
      help.activeParameter = activeParameter;
      return help;
    }
  };
}

function createReferenceProvider() {
  return {
    provideReferences(document, position, context) {
      const found = wordAt(document, position);
      if (!found) return [];
      const word = found.word;
      const refs = [];
      const regex = new RegExp(`\\b${word.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\$&')}\\b`, 'g');

      for (let lineNumber = 0; lineNumber < document.lineCount; lineNumber++) {
        const text = stripComment(document.lineAt(lineNumber).text);
        let match;
        while ((match = regex.exec(text)) !== null) {
          const range = new vscode.Range(
            new vscode.Position(lineNumber, match.index),
            new vscode.Position(lineNumber, match.index + word.length)
          );
          if (!context.includeDeclaration) {
            const model = analyzeDocument(document);
            const declaration = model.functions.get(word) || model.types.get(word) || model.variables.get(word);
            if (declaration && declaration.line === lineNumber) continue;
          }
          refs.push(new vscode.Location(document.uri, range));
        }
      }
      return refs;
    }
  };
}

function execFileAsync(executable, args, options) {
  return new Promise((resolve, reject) => {
    execFile(executable, args, options, (error, stdout, stderr) => {
      if (error) {
        error.stdout = stdout;
        error.stderr = stderr;
        reject(error);
        return;
      }
      resolve({ stdout, stderr });
    });
  });
}

function diagnosticRange(document, parsed) {
  const line = Math.max(0, Math.min(parsed.line, Math.max(0, document.lineCount - 1)));
  const text = document.lineAt(line).text;
  const start = Math.max(0, Math.min(parsed.column, text.length));
  const end = Math.min(text.length, Math.max(start + 1, start));
  return new vscode.Range(line, start, line, end);
}

async function compilerDiagnostics(document, generation) {
  if (!diagnosticCollection || document.languageId !== 'se' || document.uri.scheme !== 'file') return;

  const config = vscode.workspace.getConfiguration('se');
  if (!config.get('diagnostics.enabled', true)) {
    diagnosticCollection.delete(document.uri);
    return;
  }

  const executable = config.get('executablePath', 'se');
  const originalPath = document.uri.fsPath;
  const cwd = vscode.workspace.getWorkspaceFolder(document.uri)?.uri.fsPath || path.dirname(originalPath);
  const temporary = document.isDirty;
  const checkPath = temporary
    ? path.join(path.dirname(originalPath), `.${path.basename(originalPath)}.vscode-${process.pid}-${Date.now()}.se`)
    : originalPath;

  try {
    if (temporary) await fs.writeFile(checkPath, document.getText(), 'utf8');
    await execFileAsync(executable, ['check', checkPath], {
      cwd,
      windowsHide: true,
      timeout: 15000,
      maxBuffer: 1024 * 1024
    });

    if (diagnosticGenerations.get(document.uri.toString()) === generation) {
      diagnosticCollection.delete(document.uri);
    }
  } catch (error) {
    if (error && error.code === 'ENOENT') {
      diagnosticCollection.delete(document.uri);
      if (!executableWarningShown) {
        executableWarningShown = true;
        vscode.window.showWarningMessage(
          `SE diagnostics could not find '${executable}'. Install SE or set SE: Executable Path.`
        );
      }
      return;
    }

    const output = [error?.stderr, error?.stdout].filter(Boolean).join('\n');
    const parsed = parseSeCheckOutput(output);
    if (!parsed) return;
    if (diagnosticGenerations.get(document.uri.toString()) !== generation) return;

    const diagnostic = new vscode.Diagnostic(
      diagnosticRange(document, parsed),
      parsed.message,
      vscode.DiagnosticSeverity.Error
    );
    diagnostic.source = 'SE';
    diagnostic.code = 'se-check';
    if (parsed.hint) {
      diagnostic.message += `\nHint: ${parsed.hint}`;
    }
    diagnosticCollection.set(document.uri, [diagnostic]);
  } finally {
    if (temporary) {
      try { await fs.unlink(checkPath); } catch (_) { /* already removed */ }
    }
  }
}

function scheduleDiagnostics(document, immediate = false) {
  if (!document || document.languageId !== 'se' || document.uri.scheme !== 'file') return;
  const key = document.uri.toString();
  const previous = diagnosticTimers.get(key);
  if (previous) clearTimeout(previous);

  const generation = (diagnosticGenerations.get(key) || 0) + 1;
  diagnosticGenerations.set(key, generation);
  const delay = immediate ? 0 : Math.max(100, vscode.workspace.getConfiguration('se').get('diagnostics.delay', 450));
  const timer = setTimeout(() => {
    diagnosticTimers.delete(key);
    compilerDiagnostics(document, generation);
  }, delay);
  diagnosticTimers.set(key, timer);
}

async function checkProblems() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== 'se') {
    vscode.window.showErrorMessage('Open an SE .se file first.');
    return;
  }
  scheduleDiagnostics(editor.document, true);
}

async function openSyntaxGuide(context) {
  const guide = vscode.Uri.joinPath(context.extensionUri, 'TUTORIAL-zh-TW.md');
  await vscode.commands.executeCommand('markdown.showPreview', guide);
}

function activate(context) {
  const selector = { language: 'se', scheme: 'file' };
  diagnosticCollection = vscode.languages.createDiagnosticCollection('se');

  context.subscriptions.push(
    diagnosticCollection,
    vscode.languages.registerCompletionItemProvider(selector, createCompletionProvider(), '.'),
    vscode.languages.registerHoverProvider(selector, createHoverProvider()),
    vscode.languages.registerDefinitionProvider(selector, createDefinitionProvider()),
    vscode.languages.registerDocumentSymbolProvider(selector, createDocumentSymbolProvider()),
    vscode.languages.registerSignatureHelpProvider(selector, createSignatureHelpProvider(), ' '),
    vscode.languages.registerReferenceProvider(selector, createReferenceProvider()),
    vscode.commands.registerCommand('se.run', () => run('run')),
    vscode.commands.registerCommand('se.check', () => run('check')),
    vscode.commands.registerCommand('se.checkProblems', checkProblems),
    vscode.commands.registerCommand('se.build', () => run('build')),
    vscode.commands.registerCommand('se.openGuide', () => openSyntaxGuide(context)),
    vscode.workspace.onDidOpenTextDocument((document) => scheduleDiagnostics(document, true)),
    vscode.workspace.onDidSaveTextDocument((document) => scheduleDiagnostics(document, true)),
    vscode.workspace.onDidChangeTextDocument((event) => scheduleDiagnostics(event.document, false)),
    vscode.workspace.onDidCloseTextDocument((document) => {
      const key = document.uri.toString();
      const timer = diagnosticTimers.get(key);
      if (timer) clearTimeout(timer);
      diagnosticTimers.delete(key);
      diagnosticGenerations.delete(key);
      diagnosticCollection.delete(document.uri);
    }),
    vscode.workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration('se.executablePath')) executableWarningShown = false;
      if (event.affectsConfiguration('se.diagnostics')) {
        for (const document of vscode.workspace.textDocuments) scheduleDiagnostics(document, true);
      }
    }),
    vscode.window.onDidCloseTerminal((terminal) => {
      if (terminal === seTerminal) seTerminal = undefined;
    })
  );

  for (const document of vscode.workspace.textDocuments) {
    scheduleDiagnostics(document, true);
  }
}

function deactivate() {
  for (const timer of diagnosticTimers.values()) clearTimeout(timer);
  diagnosticTimers.clear();
  if (diagnosticCollection) diagnosticCollection.dispose();
  if (seTerminal) seTerminal.dispose();
}

module.exports = { activate, deactivate };
