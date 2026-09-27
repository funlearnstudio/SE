const vscode = require('vscode');

let seTerminal;

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
  ['try', 'keyword', 'Handle a recoverable failure.'],
  ['fail', 'keyword', 'Create a failure.'],
  ['await', 'keyword', 'Wait for an asynchronous task.'],
  ['and', 'keyword', 'Logical AND.'],
  ['or', 'keyword', 'Logical OR.'],
  ['not', 'keyword', 'Logical NOT.'],
  ['true', 'value', 'Boolean true.'],
  ['false', 'value', 'Boolean false.'],

  ['http', 'module', 'HTTP client/server utilities.'],
  ['https', 'module', 'HTTPS client utilities.'],
  ['json', 'module', 'JSON parsing and serialization.'],
  ['db', 'module', 'Database utilities and adapters.'],
  ['math', 'module', 'Mathematics utilities.'],
  ['collections', 'module', 'Collection helpers.'],
  ['file', 'module', 'File utilities.'],
  ['path', 'module', 'Path utilities.'],
  ['time', 'module', 'Time utilities.'],
  ['os', 'module', 'Operating-system utilities.'],
  ['option', 'module', 'Optional-value helpers.'],
  ['result', 'module', 'Success/failure helpers.'],
  ['async', 'module', 'Managed task helpers.'],
  ['function', 'module', 'Function-value helpers.'],
  ['statistics', 'module', 'Statistics helpers.'],
  ['regex', 'module', 'Regular-expression helpers.'],
  ['re', 'module', 'Alias for regex helpers.'],
  ['decimal', 'module', 'Decimal arithmetic helpers.'],
  ['csv', 'module', 'CSV parsing and file helpers.'],
  ['datetime', 'module', 'Date/time formatting helpers.'],
  ['hash', 'module', 'Hashing helpers.'],
  ['hashlib', 'module', 'Alias for hashing helpers.'],
  ['base64', 'module', 'Base64 encoding helpers.'],
  ['uuid', 'module', 'UUID helpers.'],
  ['iter', 'module', 'Iterator and combinatorics helpers.'],
  ['itertools', 'module', 'Alias for iterator helpers.'],
  ['pickle', 'module', 'Safe JSON-based serialization helpers.'],
  ['args', 'module', 'Command-line argument parsing helpers.'],
  ['argparse', 'module', 'Alias for argument parsing helpers.'],
  ['log', 'module', 'Structured logging helpers.'],
  ['logging', 'module', 'Alias for logging helpers.'],
  ['shutil', 'module', 'Filesystem copy/move helpers.'],
  ['glob', 'module', 'Filesystem wildcard helpers.'],
  ['zip', 'module', 'ZIP archive helpers.'],
  ['zipfile', 'module', 'Alias for ZIP archive helpers.'],
  ['subprocess', 'module', 'External process helpers.'],
  ['socket', 'module', 'TCP socket helpers.'],
  ['threading', 'module', 'Managed thread/task helpers.'],
  ['queue', 'module', 'FIFO queue helpers.'],
  ['sqlite', 'module', 'SQLite CLI bridge.'],
  ['sqlite3', 'module', 'Alias for SQLite helpers.'],
  ['functools', 'module', 'Higher-order function helpers.'],
  ['operator', 'module', 'Operator functions.'],
  ['copy', 'module', 'Shallow/deep copy helpers.'],
  ['enum', 'module', 'Enumeration helpers.'],
  ['typing', 'module', 'Runtime type inspection helpers.'],

  ['html', 'web', 'HTML structure inside an SE Web component.'],
  ['css', 'web', 'CSS styles inside an SE Web component.'],
  ['js', 'web', 'Browser behavior inside an SE Web component.'],
  ['style', 'web', 'Component styling.'],
  ['page', 'web', 'Define a web page.'],
  ['when', 'web', 'Handle an event.']
];

const MODULE_MEMBERS = {
  collections: [
    ['filter', 'filter list predicate', 'Return values for which predicate is true.'],
    ['map', 'map list function', 'Transform every value into a new List.'],
    ['reduce', 'reduce list initial function', 'Reduce a List into one value.'],
    ['slice', 'slice list start end', 'Return a List slice.'],
    ['take', 'take list count', 'Take the first count values.'],
    ['drop', 'drop list count', 'Drop the first count values.'],
    ['sort_by', 'sort_by list field', 'Sort by a field/key.'],
    ['sort_by_desc', 'sort_by_desc list field', 'Sort descending by field/key.'],
    ['sort_with', 'sort_with list comparator', 'Sort using a comparator function.']
  ],
  option: [
    ['some', 'some value', 'Create an Option containing a value.'],
    ['none', 'none', 'Create an empty Option.'],
    ['is_some', 'is_some option', 'Check whether an Option contains a value.'],
    ['is_none', 'is_none option', 'Check whether an Option is empty.'],
    ['value', 'value option', 'Get the contained value or fail.'],
    ['or', 'or option fallback', 'Return the value or a fallback.']
  ],
  result: [
    ['ok', 'ok value', 'Create a successful Result.'],
    ['err', 'err value', 'Create a failed Result.'],
    ['is_ok', 'is_ok result', 'Check for success.'],
    ['is_err', 'is_err result', 'Check for failure.'],
    ['value', 'value result', 'Get the success value.'],
    ['error', 'error result', 'Get the error value.'],
    ['or', 'or result fallback', 'Return success value or fallback.']
  ],
  async: [
    ['run', 'run function args...', 'Start a managed task.'],
    ['await', 'await task', 'Wait for a managed task.'],
    ['ready', 'ready task', 'Check whether a task is complete.']
  ],
  function: [
    ['bind', 'bind function args...', 'Partially bind function arguments.'],
    ['partial', 'partial function args...', 'Create a partially applied function.'],
    ['call', 'call function args...', 'Call a function value.'],
    ['pipe', 'pipe value functions...', 'Pass a value through functions.'],
    ['reduce', 'reduce function list initial?', 'Reduce a List using a function.'],
    ['map', 'map function list', 'Map a function over a List.'],
    ['filter', 'filter function list', 'Filter a List using a predicate.']
  ],
  threading: [
    ['run', 'run function args...', 'Start a managed worker task.'],
    ['join', 'join task', 'Wait for a managed worker task.'],
    ['ready', 'ready task', 'Check whether a worker task is complete.']
  ],
  db: [
    ['open', 'open path', 'Open a local SE key/value database.'],
    ['set', 'set store key value', 'Set a Text key/value pair.'],
    ['get', 'get store key', 'Read a value by key.'],
    ['has', 'has store key', 'Check whether a key exists.'],
    ['remove', 'remove store key', 'Remove a key.'],
    ['keys', 'keys store', 'List keys in the store.'],
    ['save', 'save store', 'Persist the store.'],
    ['connect', 'connect adapter target args...', 'Connect to an external database adapter.'],
    ['exec', 'exec connection action payload', 'Execute an external database action.'],
    ['adapter', 'adapter connection', 'Return the adapter name for a connection.']
  ],
  https: [
    ['get', 'get url', 'Perform an HTTPS GET request.'],
    ['post', 'post url body', 'Perform an HTTPS POST request.'],
    ['post_json', 'post_json url json', 'POST a JSON body over HTTPS.']
  ],
  http: [
    ['get', 'get url', 'Perform an HTTP GET request.'],
    ['post', 'post url body', 'Perform an HTTP POST request.'],
    ['post_json', 'post_json url json', 'POST a JSON body.']
  ],
  json: [
    ['parse', 'parse text', 'Parse JSON Text into an SE value.'],
    ['stringify', 'stringify value', 'Serialize an SE value as JSON Text.']
  ]
};

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
  return (MODULE_MEMBERS[moduleName] || []).map(([name, signature, description]) => {
    const item = makeCompletion(
      name,
      vscode.CompletionItemKind.Function,
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

function createSignatureHelpProvider() {
  return {
    provideSignatureHelp(document, position) {
      const line = document.lineAt(position.line).text.slice(0, position.character);
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

function activate(context) {
  const selector = { language: 'se', scheme: 'file' };

  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider(selector, createCompletionProvider(), '.'),
    vscode.languages.registerHoverProvider(selector, createHoverProvider()),
    vscode.languages.registerDefinitionProvider(selector, createDefinitionProvider()),
    vscode.languages.registerDocumentSymbolProvider(selector, createDocumentSymbolProvider()),
    vscode.languages.registerSignatureHelpProvider(selector, createSignatureHelpProvider(), ' '),
    vscode.languages.registerReferenceProvider(selector, createReferenceProvider()),
    vscode.commands.registerCommand('se.run', () => run('run')),
    vscode.commands.registerCommand('se.check', () => run('check')),
    vscode.commands.registerCommand('se.build', () => run('build')),
    vscode.window.onDidCloseTerminal((terminal) => {
      if (terminal === seTerminal) seTerminal = undefined;
    })
  );
}

function deactivate() {
  if (seTerminal) seTerminal.dispose();
}

module.exports = { activate, deactivate };
