const MODULES = [
  ['file', 'File reading, writing, copy, move, and directory helpers.'],
  ['path', 'Filesystem path helpers.'],
  ['time', 'SE Time value helpers.'],
  ['math', 'Mathematics, trigonometry, combinatorics, and statistics.'],
  ['random', 'Random integer and number generation.'],
  ['os', 'Operating-system and environment helpers.'],
  ['json', 'JSON parsing and serialization.'],
  ['text', 'Text manipulation helpers.'],
  ['collections', 'List, Map, Set, sorting, mapping, and filtering helpers.'],
  ['test', 'Assertions for SE tests.'],
  ['process', 'External process execution.'],
  ['http', 'HTTP client helpers.'],
  ['web', 'SE HTTP server and routing.'],
  ['js', 'JavaScript bridge.'],
  ['ts', 'TypeScript bridge.'],
  ['function', 'Function-value and higher-order helpers.'],
  ['async', 'Managed asynchronous tasks.'],
  ['threading', 'Managed worker tasks.'],
  ['option', 'Optional-value helpers.'],
  ['result', 'Success/failure result helpers.'],
  ['match', 'Functional matching helpers.'],
  ['db', 'Local key/value database.'],
  ['https', 'HTTPS client through curl.'],
  ['data', 'Mutable List/Map/Set data helpers.'],
  ['net', 'HTTP/HTTPS networking through curl.'],
  ['node', 'Node.js, npm, and npx bridge.'],
  ['next', 'Next.js project commands.'],
  ['game', 'Small canvas-game scene builder.'],
  ['statistics', 'Mean, median, variance, and standard deviation.'],
  ['regex', 'Regular-expression helpers.'],
  ['re', 'Alias for regex.'],
  ['base64', 'Base64 encoding and decoding.'],
  ['uuid', 'UUID v4 generation and validation.'],
  ['iter', 'Range, enumerate, zip, product, permutations, and combinations.'],
  ['itertools', 'Alias for iter.'],
  ['copy', 'Shallow and deep collection copying.'],
  ['operator', 'Operator functions.'],
  ['decimal', 'Text-based decimal arithmetic helpers.'],
  ['csv', 'CSV parsing, serialization, and files.'],
  ['datetime', 'UTC date/time text and timestamp helpers.'],
  ['hash', 'SHA-256 hashing helpers.'],
  ['hashlib', 'Alias for hash.'],
  ['pickle', 'Safe JSON-based serialization helpers.'],
  ['args', 'Command-line argument parsing helpers.'],
  ['argparse', 'Alias for args.'],
  ['log', 'Logging helpers.'],
  ['logging', 'Alias for log.'],
  ['shutil', 'Filesystem copy/move helpers.'],
  ['glob', 'Filesystem wildcard matching.'],
  ['zip', 'ZIP archive helpers.'],
  ['zipfile', 'Alias for zip.'],
  ['subprocess', 'Shell process helpers.'],
  ['socket', 'DNS resolution and simple TCP helpers.'],
  ['queue', 'FIFO queue helpers.'],
  ['sqlite', 'SQLite CLI bridge.'],
  ['sqlite3', 'Alias for sqlite.'],
  ['functools', 'Higher-order function helpers.'],
  ['enum', 'Runtime enumeration helpers.'],
  ['typing', 'Runtime type inspection helpers.']
];

const f = (name, usage, description, kind = 'function') => [name, usage, description, kind];

const MODULE_MEMBERS = {
  file: [
    f('read', 'read path', 'Read a file. Fallible.'),
    f('write', 'write path text', 'Write a file. Fallible.'),
    f('append', 'append path text', 'Append Text to a file. Fallible.'),
    f('open', 'open path', 'Open a file handle. Fallible.'),
    f('copy', 'copy source target', 'Copy a file. Fallible.'),
    f('move', 'move source target', 'Move or rename a path. Fallible.'),
    f('copytree', 'copytree source target', 'Recursively copy a directory. Fallible.'),
    f('remove', 'remove path', 'Remove a file or directory tree. Fallible.'),
    f('mkdir', 'mkdir path', 'Create directories. Fallible.')
  ],
  path: [
    f('join', 'join part...', 'Join one or more path parts.'),
    f('name', 'name path', 'Return the final path component.'),
    f('ext', 'ext path', 'Return the extension.'),
    f('parent', 'parent path', 'Return the parent path.'),
    f('exists', 'exists path', 'Check whether a path exists.'),
    f('is_file', 'is_file path', 'Check whether a path is a file.'),
    f('is_dir', 'is_dir path', 'Check whether a path is a directory.')
  ],
  time: [
    f('now', 'now', 'Return the current Time value.'),
    f('unix', 'unix time', 'Convert Time to a Unix timestamp.'),
    f('from_unix', 'from_unix timestamp', 'Convert a Unix timestamp to Time.'),
    f('iso', 'iso time', 'Format Time as UTC ISO-8601.')
  ],
  math: [
    f('pi', 'pi', 'Pi.', 'value'), f('e', 'e', 'Euler number.', 'value'),
    f('tau', 'tau', 'Tau (2*pi).', 'value'), f('inf', 'inf', 'Positive infinity.', 'value'),
    ...['sqrt','cbrt','abs','floor','ceil','round','trunc','sin','cos','tan','asin','acos','atan','sinh','cosh','tanh','asinh','acosh','atanh','exp','exp2','expm1','log','log10','log2','log1p','degrees','radians','gamma','lgamma','erf','erfc'].map((name) => f(name, `${name} number`, 'Numeric math helper.')),
    ...['atan2','pow','fmod','remainder','copysign','nextafter'].map((name) => f(name, `${name} a b`, 'Two-number math helper.')),
    f('hypot', 'hypot number number...', 'Euclidean norm for two or more values.'),
    f('min', 'min number number...', 'Minimum of two or more values.'),
    f('max', 'max number number...', 'Maximum of two or more values.'),
    f('clamp', 'clamp value min max', 'Clamp a number into a range.'),
    f('lerp', 'lerp a b t', 'Linear interpolation.'),
    f('map_range', 'map_range value in_min in_max out_min out_max', 'Map a value between ranges.'),
    f('sign', 'sign number', 'Return -1, 0, or 1.'),
    f('isfinite', 'isfinite number', 'Check whether a number is finite.'),
    f('isinf', 'isinf number', 'Check whether a number is infinite.'),
    f('isnan', 'isnan number', 'Check whether a number is NaN.'),
    f('gcd', 'gcd int int...', 'Greatest common divisor.'),
    f('lcm', 'lcm int int...', 'Least common multiple.'),
    f('factorial', 'factorial int', 'Factorial for a non-negative Int.'),
    f('comb', 'comb n k', 'Number of combinations.'),
    f('perm', 'perm n k', 'Number of permutations.'),
    f('sum', 'sum list', 'Sum numeric List values.'),
    f('mean', 'mean list', 'Mean of numeric List values.'),
    f('median', 'median list', 'Median of numeric List values.'),
    f('variance', 'variance list', 'Population variance helper.'),
    f('stddev', 'stddev list', 'Population standard deviation helper.')
  ],
  random: [
    f('int', 'int min max', 'Random Int in an inclusive range.'),
    f('num', 'num', 'Random Num in [0, 1).')
  ],
  os: [
    f('platform', 'platform', 'Current platform name.', 'value'),
    f('cwd', 'cwd', 'Current working directory.'),
    f('getenv', 'getenv name', 'Read an environment variable.'),
    f('has_env', 'has_env name', 'Check whether an environment variable exists.')
  ],
  json: [
    f('parse', 'parse text', 'Parse JSON Text into an SE value.'),
    f('stringify', 'stringify value', 'Serialize an SE value as compact JSON.'),
    f('pretty', 'pretty value', 'Serialize an SE value as formatted JSON.')
  ],
  text: [
    f('trim', 'trim text', 'Trim surrounding whitespace.'),
    f('contains', 'contains text search', 'Check whether Text contains a substring.'),
    f('starts', 'starts text prefix', 'Check a prefix.'),
    f('ends', 'ends text suffix', 'Check a suffix.'),
    f('replace', 'replace text old new', 'Replace text.'),
    f('split', 'split text separator', 'Split Text into a List.'),
    f('join', 'join values separator', 'Join values using a separator.'),
    f('repeat', 'repeat text count', 'Repeat Text.')
  ],
  collections: [
    f('reverse', 'reverse values', 'Reverse a collection.'),
    f('contains', 'contains values value', 'Check membership.'),
    f('first', 'first values', 'Return the first value.'),
    f('last', 'last values', 'Return the last value.'),
    f('unique', 'unique values', 'Remove duplicate values.'),
    f('sort', 'sort values', 'Sort values.'),
    f('keys', 'keys map', 'Return Map keys.'),
    f('values', 'values map', 'Return Map values.'),
    f('filter', 'filter list predicate', 'Return values for which predicate is true.'),
    f('map', 'map list function', 'Transform every value into a new List.'),
    f('reduce', 'reduce list initial function', 'Reduce a List into one value.'),
    f('slice', 'slice list start end', 'Return a List slice.'),
    f('take', 'take list count', 'Take the first count values.'),
    f('drop', 'drop list count', 'Drop the first count values.'),
    f('sort_by', 'sort_by list field', 'Sort by a field/key.'),
    f('sort_by_desc', 'sort_by_desc list field', 'Sort descending by field/key.'),
    f('sort_with', 'sort_with list comparator', 'Sort using a comparator function.')
  ],
  test: [
    f('ok', 'ok condition', 'Assert a Bool is true.'),
    f('equal', 'equal actual expected', 'Assert two values are equal.'),
    f('not_equal', 'not_equal actual expected', 'Assert two values differ.'),
    f('fail', 'fail message', 'Fail a test explicitly.')
  ],
  process: [
    f('run', 'run command args...', 'Run an external process. Fallible.'),
    f('output', 'output command', 'Capture process output. Fallible.')
  ],
  http: [
    f('get', 'get url', 'Perform an HTTP GET request. Fallible.'),
    f('post', 'post url body', 'Perform an HTTP POST request. Fallible.'),
    f('post_json', 'post_json url json', 'POST a JSON body. Fallible.'),
    f('request', 'request method url body', 'Perform a general HTTP request. Fallible.')
  ],
  web: [
    ...['get','post','put','patch','delete'].map((name) => f(name, `${name} path handler`, `Register an HTTP ${name.toUpperCase()} route.`)),
    f('listen', 'listen port', 'Start the SE web server. Fallible.'),
    f('text', 'text body', 'Create a text response.'),
    f('json', 'json body', 'Create a JSON response.'),
    f('response', 'response status body type', 'Create a custom response.'),
    f('method', 'method', 'Current request method.'),
    f('path', 'path', 'Current request path.'),
    f('query', 'query', 'Current request query string.'),
    f('body', 'body', 'Current request body.'),
    f('header', 'header name', 'Read a request header.'),
    f('param', 'param name', 'Read a route parameter.'),
    f('handle', 'handle method path body', 'Invoke a route in-process.'),
    f('handle_status', 'handle_status method path body', 'Return in-process route status.'),
    f('route_count', 'route_count', 'Number of registered routes.')
  ],
  js: [
    f('run', 'run file', 'Run a JavaScript file. Fallible.'),
    f('output', 'output file', 'Run JavaScript and capture output. Fallible.'),
    f('eval', 'eval code', 'Evaluate JavaScript and capture output. Fallible.')
  ],
  ts: [
    f('run', 'run file', 'Run TypeScript. Fallible.'),
    f('compile', 'compile file', 'Compile TypeScript. Fallible.'),
    f('output', 'output file', 'Run TypeScript and capture output. Fallible.')
  ],
  function: [
    f('bind', 'bind function args...', 'Partially bind function arguments.'),
    f('partial', 'partial function args...', 'Create a partially applied function.'),
    f('call', 'call function args...', 'Call a function value.'),
    f('pipe', 'pipe value functions...', 'Pass a value through functions.'),
    f('reduce', 'reduce function list initial?', 'Reduce a List using a function.'),
    f('map', 'map function list', 'Map a function over a List.'),
    f('filter', 'filter function list', 'Filter a List using a predicate.')
  ],
  async: [
    f('run', 'run function args...', 'Start a managed task.'),
    f('await', 'await task', 'Wait for a managed task. Fallible.'),
    f('ready', 'ready task', 'Check whether a task is complete.')
  ],
  threading: [
    f('run', 'run function args...', 'Start a managed worker task.'),
    f('join', 'join task', 'Wait for a managed worker task. Fallible.'),
    f('ready', 'ready task', 'Check whether a worker task is complete.')
  ],
  option: [
    f('some', 'some value', 'Create an Option containing a value.'),
    f('none', 'none', 'Create an empty Option.'),
    f('is_some', 'is_some option', 'Check whether an Option contains a value.'),
    f('is_none', 'is_none option', 'Check whether an Option is empty.'),
    f('value', 'value option', 'Get the contained value. Fallible.'),
    f('or', 'or option fallback', 'Return the value or a fallback.')
  ],
  result: [
    f('ok', 'ok value', 'Create a successful Result.'),
    f('err', 'err message', 'Create a failed Result.'),
    f('is_ok', 'is_ok result', 'Check for success.'),
    f('is_err', 'is_err result', 'Check for failure.'),
    f('value', 'value result', 'Get the success value. Fallible.'),
    f('error', 'error result', 'Get the error message.'),
    f('or', 'or result fallback', 'Return success value or fallback.')
  ],
  match: [
    f('value', 'value subject pattern handler ... fallback', 'Match values with handler functions.'),
    f('option', 'option option some_handler none_handler', 'Match an Option.'),
    f('result', 'result result ok_handler error_handler', 'Match a Result.')
  ],
  db: [
    f('open', 'open path', 'Open a local key/value database. Fallible.'),
    f('set', 'set database key value', 'Set and persist a Text value. Fallible.'),
    f('get', 'get database key', 'Get a value as Option.'),
    f('has', 'has database key', 'Check whether a key exists.'),
    f('remove', 'remove database key', 'Remove and persist a key. Fallible.'),
    f('keys', 'keys database', 'List keys.'),
    f('save', 'save database', 'Persist the database. Fallible.')
  ],
  https: [
    f('get', 'get url', 'Perform an HTTPS GET request. Fallible.'),
    f('post', 'post url body', 'Perform an HTTPS POST request. Fallible.'),
    f('post_json', 'post_json url json', 'POST JSON over HTTPS. Fallible.')
  ],
  data: [
    f('append', 'append list value', 'Append to a List.'),
    f('extend', 'extend list values', 'Extend a List.'),
    f('insert', 'insert list index value', 'Insert into a List.'),
    f('pop', 'pop list index?', 'Remove and return a List value.'),
    f('clear', 'clear collection', 'Clear a List, Map, or Set.'),
    f('copy', 'copy collection', 'Shallow-copy a List, Map, or Set.'),
    f('get', 'get map key default?', 'Read a Map value with optional default.'),
    f('set', 'set map key value', 'Set a Map value.'),
    f('update', 'update map source', 'Merge Map values.'),
    f('delete', 'delete map key', 'Delete a Map key.'),
    f('has', 'has collection value', 'Check a List value or Map key.'),
    f('keys', 'keys map', 'List Map keys.'),
    f('values', 'values map', 'List Map values.'),
    f('items', 'items map', 'List Map key/value pairs.')
  ],
  net: [
    f('get', 'get url', 'GET an HTTP/HTTPS URL through curl. Fallible.'),
    f('post', 'post url body', 'POST Text through curl. Fallible.'),
    f('post_json', 'post_json url json', 'POST JSON through curl. Fallible.'),
    f('request', 'request method url body', 'General network request. Fallible.'),
    f('download', 'download url path', 'Download a URL to a file. Fallible.')
  ],
  node: [
    f('version', 'version', 'Return Node.js version. Fallible.'),
    f('run', 'run file', 'Run a Node.js file. Fallible.'),
    f('output', 'output file', 'Run Node.js and capture output. Fallible.'),
    f('eval', 'eval code', 'Evaluate JavaScript with Node.js. Fallible.'),
    f('npm', 'npm args', 'Run npm arguments. Fallible.'),
    f('npx', 'npx args', 'Run npx arguments. Fallible.')
  ],
  next: [
    f('create', 'create project', 'Create a Next.js project. Fallible.'),
    f('dev', 'dev project', 'Run npm run dev. Fallible.'),
    f('build', 'build project', 'Run npm run build. Fallible.'),
    f('start', 'start project', 'Run npm run start. Fallible.'),
    f('lint', 'lint project', 'Run npm run lint. Fallible.')
  ],
  game: [
    f('new', 'new width height title', 'Create a game scene and return its id.'),
    f('background', 'background scene color', 'Set scene background.'),
    f('clear', 'clear scene', 'Clear draw commands.'),
    f('rect', 'rect scene x y width height color fill', 'Draw a rectangle.'),
    f('circle', 'circle scene x y radius color fill', 'Draw a circle.'),
    f('line', 'line scene x1 y1 x2 y2 color width', 'Draw a line.'),
    f('text', 'text scene text x y size color', 'Draw text.'),
    f('script', 'script scene javascript', 'Append browser JavaScript to a scene.'),
    f('html', 'html scene', 'Return generated scene HTML.'),
    f('save', 'save scene path', 'Save scene HTML. Fallible.'),
    f('show', 'show scene', 'Open scene in a browser. Fallible.')
  ],
  statistics: [
    f('mean', 'mean list', 'Arithmetic mean.'),
    f('median', 'median list', 'Median.'),
    f('variance', 'variance list', 'Sample variance.'),
    f('pvariance', 'pvariance list', 'Population variance.'),
    f('stdev', 'stdev list', 'Sample standard deviation.'),
    f('pstdev', 'pstdev list', 'Population standard deviation.')
  ],
  regex: [
    f('match', 'match pattern text', 'Check whether all Text matches a regex.'),
    f('search', 'search pattern text', 'Search Text with a regex.'),
    f('replace', 'replace pattern text replacement', 'Regex replacement.'),
    f('split', 'split pattern text', 'Split Text with a regex.')
  ],
  base64: [
    f('encode', 'encode text', 'Encode Text as Base64.'),
    f('decode', 'decode text', 'Decode Base64 Text. Fallible.')
  ],
  uuid: [
    f('v4', 'v4', 'Generate a random UUID v4.'),
    f('valid', 'valid text', 'Validate a UUID string.')
  ],
  iter: [
    f('range', 'range stop | range start stop step?', 'Build an Int range as a List.'),
    f('enumerate', 'enumerate list', 'Pair indexes with values.'),
    f('zip', 'zip list list', 'Zip two Lists.'),
    f('product', 'product list list', 'Cartesian product.'),
    f('permutations', 'permutations list r?', 'Generate permutations.'),
    f('combinations', 'combinations list r', 'Generate combinations.')
  ],
  copy: [
    f('shallow', 'shallow value', 'Shallow-copy List, Map, or Set values.'),
    f('deep', 'deep value', 'Recursively copy supported collection values.')
  ],
  operator: [
    ...['add','sub','mul','div','mod','eq','ne','lt','le','gt','ge'].map((name) => f(name, `${name} a b`, 'Call an operator as a function.'))
  ],
  decimal: [
    f('parse', 'parse text', 'Normalize decimal Text.'),
    f('add', 'add a b', 'Add decimal Text values.'),
    f('sub', 'sub a b', 'Subtract decimal Text values.'),
    f('mul', 'mul a b', 'Multiply decimal Text values.'),
    f('div', 'div a b precision?', 'Divide decimal Text values.'),
    f('quantize', 'quantize value precision', 'Format decimal Text to a precision.')
  ],
  csv: [
    f('parse', 'parse text', 'Parse CSV Text into rows.'),
    f('stringify', 'stringify rows', 'Serialize rows as CSV.'),
    f('read', 'read path', 'Read and parse a CSV file. Fallible.'),
    f('write', 'write path rows', 'Write rows as CSV. Fallible.')
  ],
  datetime: [
    f('now', 'now', 'Current UTC ISO timestamp.'),
    f('timestamp', 'timestamp', 'Current Unix timestamp.'),
    f('from_timestamp', 'from_timestamp timestamp', 'Convert timestamp to UTC ISO Text.'),
    f('format', 'format timestamp format', 'Format a timestamp.'),
    f('add_seconds', 'add_seconds timestamp seconds', 'Add seconds to a timestamp.')
  ],
  hash: [
    f('sha256', 'sha256 text', 'SHA-256 digest for Text. Fallible.'),
    f('file_sha256', 'file_sha256 path', 'SHA-256 digest for a file. Fallible.')
  ],
  pickle: [
    f('dumps', 'dumps value', 'Serialize a value as safe JSON Text.'),
    f('loads', 'loads text', 'Deserialize safe JSON Text. Fallible.')
  ],
  args: [
    f('parse', 'parse argv', 'Parse CLI argument Text values.'),
    f('get', 'get parsed name default?', 'Read a parsed argument.'),
    f('flag', 'flag parsed name', 'Read a Bool flag.')
  ],
  log: [
    f('level', 'level name', 'Set logging threshold.'),
    f('debug', 'debug message', 'Write a debug log.'),
    f('info', 'info message', 'Write an info log.'),
    f('warn', 'warn message', 'Write a warning log.'),
    f('error', 'error message', 'Write an error log.')
  ],
  shutil: [
    f('copy', 'copy source target', 'Copy a file. Fallible.'),
    f('move', 'move source target', 'Move or rename a path. Fallible.'),
    f('copytree', 'copytree source target', 'Recursively copy a directory. Fallible.'),
    f('remove', 'remove path', 'Remove a path tree. Fallible.'),
    f('mkdir', 'mkdir path', 'Create directories. Fallible.')
  ],
  glob: [
    f('match', 'match pattern text', 'Match Text against a wildcard pattern.'),
    f('find', 'find pattern', 'Find filesystem paths by wildcard. Fallible.')
  ],
  zip: [
    f('create', 'create archive files', 'Create a ZIP archive. Fallible.'),
    f('extract', 'extract archive directory', 'Extract a ZIP archive. Fallible.'),
    f('list', 'list archive', 'List ZIP entries. Fallible.')
  ],
  subprocess: [
    f('run', 'run command', 'Run a shell command. Fallible.'),
    f('output', 'output command', 'Capture shell command output. Fallible.')
  ],
  socket: [
    f('resolve', 'resolve host', 'Resolve a hostname. Fallible.'),
    f('tcp', 'tcp host port payload', 'Connect, send, and receive TCP data. Fallible.')
  ],
  queue: [
    f('new', 'new', 'Create a FIFO queue.'),
    f('put', 'put queue value', 'Append a value.'),
    f('get', 'get queue', 'Remove the first value. Fallible when empty.'),
    f('empty', 'empty queue', 'Check whether a queue is empty.'),
    f('size', 'size queue', 'Return queue size.')
  ],
  sqlite: [
    f('open', 'open path', 'Open a SQLite database handle.'),
    f('exec', 'exec database sql', 'Execute SQL. Fallible.'),
    f('query', 'query database sql', 'Run a query and return CSV-style rows. Fallible.')
  ],
  functools: [
    f('partial', 'partial function args...', 'Create a partially applied function.'),
    f('reduce', 'reduce function list initial?', 'Reduce a List.'),
    f('map', 'map function list', 'Map a function over a List.'),
    f('filter', 'filter function list', 'Filter a List.')
  ],
  enum: [
    f('make', 'make names', 'Create name -> integer enum Map starting at zero.'),
    f('name', 'name enum value', 'Find the name for a value. Fallible.'),
    f('value', 'value enum name', 'Find the value for a name. Fallible.'),
    f('has', 'has enum name', 'Check whether a name exists.')
  ],
  typing: [
    f('type_of', 'type_of value', 'Return the runtime SE type name.'),
    f('is', 'is value type_name', 'Check the runtime type name.'),
    f('cast', 'cast value type_name', 'Assert a runtime type and return the value.')
  ]
};

const ALIASES = {
  re: 'regex',
  itertools: 'iter',
  hashlib: 'hash',
  argparse: 'args',
  logging: 'log',
  zipfile: 'zip',
  sqlite3: 'sqlite'
};

for (const [alias, target] of Object.entries(ALIASES)) {
  MODULE_MEMBERS[alias] = MODULE_MEMBERS[target];
}

const MODULE_DESCRIPTIONS = Object.fromEntries(MODULES);
const BUILTIN_MODULES = MODULES.map(([name]) => name);

module.exports = {
  MODULES,
  MODULE_DESCRIPTIONS,
  BUILTIN_MODULES,
  MODULE_MEMBERS,
  ALIASES
};
