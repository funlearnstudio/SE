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
  ['re', 'Pattern matching, extraction, replacement, and escaping.'],
  ['base64', 'Base64 encoding and decoding.'],
  ['uuid', 'UUID v4 generation and validation.'],
  ['iter', 'Range, enumerate, zip, product, permutations, and combinations.'],
  ['itertools', 'Composable sequence iteration, chunking, windows, and lazy-style transforms.'],
  ['copy', 'Shallow and deep collection copying.'],
  ['operator', 'Operator functions.'],
  ['decimal', 'Text-based decimal arithmetic helpers.'],
  ['csv', 'CSV parsing, serialization, and files.'],
  ['datetime', 'UTC date/time text and timestamp helpers.'],
  ['hash', 'SHA-256 hashing helpers.'],
  ['hashlib', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison.'],
  ['pickle', 'Safe JSON-based serialization helpers.'],
  ['args', 'Command-line argument parsing helpers.'],
  ['argparse', 'Declare command-line options, parse arguments, and generate help text.'],
  ['log', 'Logging helpers.'],
  ['logging', 'Named loggers, levels, formatting, and inspectable log records.'],
  ['shutil', 'Filesystem copy/move helpers.'],
  ['glob', 'Filesystem wildcard matching.'],
  ['zip', 'ZIP archive helpers.'],
  ['zipfile', 'Create, inspect, read, test, and extract ZIP archives.'],
  ['subprocess', 'Shell process helpers.'],
  ['socket', 'DNS resolution and simple TCP helpers.'],
  ['queue', 'FIFO queue helpers.'],
  ['sqlite', 'SQLite CLI bridge.'],
  ['sqlite3', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle.'],
  ['functools', 'Higher-order function helpers.'],
  ['enum', 'Runtime enumeration helpers.'],
  ['typing', 'Runtime type inspection helpers.'],
  ['url', 'URL percent encoding and query strings.'],
  ['encoding', 'Hexadecimal encoding and UTF-8 validation.'],
  ['dotenv', 'Parse simple KEY=VALUE configuration.'],
  ['config', 'Structured application settings with typed access, sections, merging, and environment overrides.'],
  ['array', 'Numeric List statistics and slicing.'],
  ['series', 'Time-series transforms, rolling statistics, differences, returns, and normalization.'],
  ['matrix', 'Matrix multiplication, transposition, and dot products.'],
  ['linear', 'Vector and linear algebra operations including matrix factorization helpers.'],
  ['probability', 'Factorial and combination counts.'],
  ['fraction', 'Reduced fractions and decimal conversion.'],
  ['complex', 'Complex numbers represented as [real, imaginary].'],
  ['calculus', 'One-variable polynomial evaluation, derivative, and definite integral.'],
  ['units', 'Length, time, mass, and temperature conversions.'],
  ['table', 'Read and project columns from Map rows.'],
  ['dataset', 'Tabular data selection, filtering, grouping, joins, summaries, and splits.'],
  ['cookie', 'Parse Cookie headers and create Set-Cookie values.'],
  ['cors', 'Construct CORS response headers.'],
  ['template', 'HTML-escaped {{key}} template rendering.'],
  ['static', 'Read rooted static files and detect MIME types.'],
  ['upload', 'Save data within an existing root directory.'],
  ['tilemap', 'Parse and inspect text tile maps.'],
  ['http_server', 'Build HTTP services with middleware, static assets, and response helpers.'],
  ['router', 'Standalone route registration, parameter matching, dispatch, and fallback handlers.'],
  ['dns', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records.'],
  ['toml', 'TOML parsing through Python 3.11+.'],
  ['yaml', 'YAML parsing and serialization (requires PyYAML).'],
  ['xml', 'XML parsing and escaping through Python.'],
  ['markdown', 'CommonMark rendering (requires markdown-it-py).'],
  ['crypto', 'SHA-256, HMAC, secure randomness, and constant-time comparison.'],
  ['jwt', 'HS256 signing and verification.'],
  ['session', 'Signed, stateless session claims using HS256.'],
  ['auth', 'PBKDF2 password hashing and verification.'],
  ['email', 'Compose and parse email messages.'],
  ['smtp', 'Send mail with SMTP over TLS.'],
  ['imap', 'Read mail subjects with IMAP over TLS.'],
  ['ftp', 'List, download, and upload files through FTPS.'],
  ['ssh', 'Run a remote command using Paramiko and known_hosts.'],
  ['websocket', 'One-message WSS exchange (requires websockets).'],
  ['ai', 'Chat with a compatible HTTPS AI endpoint.'],
  ['embedding', 'Create vectors through a compatible HTTPS embedding endpoint.'],
  ['ml', 'One-variable linear regression and prediction.'],
  ['tensor', 'Numeric tensor shape, addition, and 2D multiplication.'],
  ['video', 'Add browser video to an SE game scene.'],
  ['camera', 'Request and display browser camera video.'],
  ['gui', 'Compose browser interfaces from windows, panels, controls, and labels.'],
  ['window', 'Manage browser game windows, dimensions, titles, and fullscreen state.'],
  ['canvas', 'Draw and export canvas scenes with size and primitive helpers.'],
  ['input', 'Track key and pointer state and register input callbacks.'],
  ['sprite', 'Load, draw, transform, animate, and inspect sprite bounds.'],
  ['physics', 'Integrate simple bodies, forces, gravity, and collision checks.'],
  ['sound', 'Generate tones and manage sound playback, volume, and fades.'],
  ['keyboard', 'Query keyboard state and register key press/release callbacks.'],
  ['mouse', 'Read pointer position/buttons and handle movement, clicks, and wheel input.'],
  ['animation', 'Create cancellable tweens, sequences, repeats, and easing curves.'],
  ['scene', 'Create and manage scenes, backgrounds, objects, and transitions.'],
  ['collision', 'Test common point, rectangle, and circle collision shapes.'],
  ['image', 'Load, crop, resize, transform, inspect, and save images.'],
  ['audio', 'Browser audio alias for the game scene API.']
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
  game_extended: [
    f('image', 'image scene url x y width height', 'Draw a browser image.'),
    f('sprite', 'sprite scene name url x y width height', 'Add an image sprite.'),
    f('sprite_color', 'sprite_color scene name x y width height color', 'Add a colored sprite.'),
    f('position', 'position scene name x y', 'Set a sprite position.'),
    f('move', 'move scene name dx dy', 'Move a sprite.'),
    f('velocity', 'velocity scene name vx vy', 'Set sprite velocity.'),
    f('animate', 'animate scene fps', 'Start the sprite animation loop.'),
    f('key_move', 'key_move scene name key dx dy', 'Move a sprite when a key is pressed.'),
    f('follow_mouse', 'follow_mouse scene name', 'Follow the mouse with a sprite.'),
    f('sound', 'sound scene name url', 'Register a browser sound.'),
    f('play', 'play scene name loop volume', 'Play a registered sound.'),
    f('stop', 'stop scene name', 'Stop a registered sound.'),
    f('fullscreen', 'fullscreen scene', 'Enable double-click fullscreen.'),
    f('camera', 'camera scene x y', 'Set the scene camera offset.'),
    f('particles', 'particles scene x y count color speed', 'Emit particles.'),
    f('rect_hit', 'rect_hit ax ay aw ah bx by bw bh', 'Test rectangle overlap.'),
    f('circle_hit', 'circle_hit ax ay ar bx by br', 'Test circle overlap.'),
    f('distance', 'distance x1 y1 x2 y2', 'Distance between two points.'),
    f('vector', 'vector x y', 'Create a 2D vector List.')
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
  ],
  url: [
    f('encode', 'encode text', 'Percent-encode URL text.'),
    f('decode', 'decode text', 'Decode percent-encoded text. Fallible.'),
    f('query', 'query map', 'Build a URL query string from a Map.'),
    f('parse_query', 'parse_query text', 'Parse a query string into a Map. Fallible.')
  ],
  encoding: [
    f('hex', 'hex text', 'Encode text as hexadecimal.'),
    f('unhex', 'unhex text', 'Decode hexadecimal text. Fallible.'),
    f('utf8_valid', 'utf8_valid text', 'Check UTF-8 validity.')
  ],
  dotenv: [
    f('parse', 'parse text', 'Parse KEY=VALUE lines into a Map. Fallible.'),
    f('get', 'get config key', 'Read a required key from a configuration Map. Fallible.')
  ],
  array: [
    f('sum', 'sum values', 'Sum a numeric List.'),
    f('mean', 'mean values', 'Mean of a numeric List. Fallible.'),
    f('slice', 'slice values start end', 'Slice a List with checked bounds. Fallible.')
  ],
  matrix: [
    f('transpose', 'transpose rows', 'Transpose a matrix. Fallible.'),
    f('multiply', 'multiply left right', 'Multiply two matrices. Fallible.'),
    f('dot', 'dot left right', 'Dot product of equal-length vectors. Fallible.')
  ],
  probability: [
    f('factorial', 'factorial n', 'Factorial for 0 to 20. Fallible.'),
    f('choose', 'choose n k', 'Combination count with bounded inputs. Fallible.')
  ],
  fraction: [
    f('make', 'make numerator denominator', 'Create a reduced fraction List. Fallible.'),
    f('decimal', 'decimal fraction', 'Convert a fraction List to a number. Fallible.')
  ],
  complex: [
    f('make', 'make real imaginary', 'Create a complex number List.'),
    f('add', 'add left right', 'Add complex numbers. Fallible.'),
    f('multiply', 'multiply left right', 'Multiply complex numbers. Fallible.'),
    f('magnitude', 'magnitude value', 'Magnitude of a complex number. Fallible.')
  ],
  calculus: [
    f('polynomial', 'polynomial coefficients x', 'Evaluate a polynomial with ascending coefficients.'),
    f('derivative', 'derivative coefficients x', 'Evaluate its derivative.'),
    f('integral', 'integral coefficients start end', 'Definite integral of a polynomial.')
  ],
  units: [
    f('convert', 'convert value from_unit to_unit', 'Convert compatible length, time, or mass units. Fallible.'),
    f('celsius_to_fahrenheit', 'celsius_to_fahrenheit value', 'Convert Celsius to Fahrenheit.'),
    f('fahrenheit_to_celsius', 'fahrenheit_to_celsius value', 'Convert Fahrenheit to Celsius.')
  ],
  table: [
    f('column', 'column rows name', 'Extract a column from Map rows. Fallible.'),
    f('row_count', 'row_count rows', 'Count table rows.'),
    f('select', 'select rows columns', 'Project table columns. Fallible.')
  ],
  cookie: [
    f('parse', 'parse header', 'Parse a Cookie header into a Map.'),
    f('set', 'set name value', 'Create a Set-Cookie header. Fallible.')
  ],
  cors: [
    f('allow_origin', 'allow_origin origin', 'Create CORS origin headers. Fallible.'),
    f('preflight', 'preflight origin methods', 'Create CORS preflight headers. Fallible.')
  ],
  template: [
    f('escape', 'escape text', 'Escape HTML text.'),
    f('render', 'render source values', 'Render escaped {{key}} placeholders. Fallible.')
  ],
  static: [
    f('mime', 'mime filename', 'Detect MIME type from an extension.'),
    f('read', 'read root filename', 'Read a file inside a root directory. Fallible.')
  ],
  upload: [
    f('save', 'save root filename body', 'Save a file within an existing root directory. Fallible.')
  ],
  tilemap: [
    f('parse', 'parse text', 'Parse a rectangular text tilemap. Fallible.'),
    f('at', 'at map x y', 'Read a tile at coordinates. Fallible.'),
    f('size', 'size map', 'Return tilemap dimensions.')
  ],
  toml: [f('parse', 'parse text', 'Parse TOML using Python 3.11+. Fallible.')],
  yaml: [
    f('parse', 'parse text', 'Parse YAML using PyYAML. Fallible.'),
    f('stringify', 'stringify value', 'Serialize YAML using PyYAML. Fallible.')
  ],
  xml: [
    f('parse', 'parse text', 'Parse XML using Python. Fallible.'),
    f('escape', 'escape text', 'Escape XML text. Fallible.')
  ],
  markdown: [f('render', 'render text', 'Render Markdown using markdown-it-py. Fallible.')],
  crypto: [
    f('sha256', 'sha256 text', 'Compute a SHA-256 hex digest. Fallible.'),
    f('hmac_sha256', 'hmac_sha256 key text', 'Compute an HMAC-SHA256 hex digest. Fallible.'),
    f('random_hex', 'random_hex byte_count', 'Generate cryptographically random hex. Fallible.'),
    f('constant_time_equal', 'constant_time_equal left right', 'Compare text in constant time. Fallible.')
  ],
  jwt: [
    f('sign', 'sign claims secret', 'Sign HS256 JSON claims. Fallible.'),
    f('verify', 'verify token secret', 'Verify an HS256 token and return claims. Fallible.')
  ],
  session: [
    f('encode', 'encode claims secret', 'Encode signed session claims. Fallible.'),
    f('decode', 'decode token secret', 'Decode signed session claims. Fallible.')
  ],
  auth: [
    f('hash_password', 'hash_password password', 'Create a PBKDF2 password hash. Fallible.'),
    f('verify_password', 'verify_password password encoded_hash', 'Verify a password hash. Fallible.')
  ],
  email: [
    f('compose', 'compose sender recipient subject body', 'Compose an email message. Fallible.'),
    f('parse', 'parse message', 'Parse an email message. Fallible.')
  ],
  smtp: [f('send', 'send host port username password recipient message', 'Send email over SMTP with TLS. Fallible.')],
  imap: [f('subjects', 'subjects host username password mailbox limit', 'Read IMAP message subjects over TLS. Fallible.')],
  ftp: [
    f('list', 'list host username password directory', 'List files over FTPS. Fallible.'),
    f('download', 'download host username password filename', 'Download a file over FTPS. Fallible.'),
    f('upload', 'upload host username password filename body', 'Upload a file over FTPS. Fallible.')
  ],
  ssh: [f('run', 'run host username command', 'Run an SSH command using known_hosts. Fallible.')],
  websocket: [f('exchange', 'exchange url message', 'Exchange a message over WSS. Fallible.')],
  ai: [f('chat', 'chat endpoint api_key model prompt', 'Request an HTTPS chat completion. Fallible.')],
  embedding: [f('create', 'create endpoint api_key model text', 'Request an HTTPS embedding vector. Fallible.')],
  ml: [
    f('linear_regression', 'linear_regression xs ys', 'Fit a one-variable linear regression. Fallible.'),
    f('predict', 'predict model x', 'Predict a numeric value. Fallible.')
  ],
  tensor: [
    f('shape', 'shape values', 'Return tensor dimensions. Fallible.'),
    f('add', 'add left right', 'Add tensors elementwise. Fallible.'),
    f('matmul', 'matmul left right', 'Multiply 2D tensors. Fallible.')
  ],
  video: [
    f('add', 'add scene url x y width height', 'Add browser video to a scene.'),
    f('stop', 'stop scene', 'Stop scene video playback.')
  ],
  camera: [
    f('start', 'start scene x y width height', 'Request browser camera for a scene.'),
    f('stop', 'stop scene', 'Stop browser camera capture.')
  ]
};

// Game aliases expose the base scene API plus browser sprite, physics, and audio helpers.
// `use game` itself exposes only the base API in the current runtime.

const INDEPENDENT_MEMBERS = {
  re: [
    f('find_all', 're.find_all', 'Pattern matching, extraction, replacement, and escaping.'),
    f('count', 're.count', 'Pattern matching, extraction, replacement, and escaping.'),
    f('escape', 're.escape', 'Pattern matching, extraction, replacement, and escaping.'),
    f('groups', 're.groups', 'Pattern matching, extraction, replacement, and escaping.'),
    f('match', 're.match', 'Pattern matching, extraction, replacement, and escaping.'),
    f('search', 're.search', 'Pattern matching, extraction, replacement, and escaping.'),
    f('replace', 're.replace', 'Pattern matching, extraction, replacement, and escaping.'),
    f('split', 're.split', 'Pattern matching, extraction, replacement, and escaping.')
  ],
  itertools: [
    f('chain', 'itertools.chain', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('flatten', 'itertools.flatten', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('chunked', 'itertools.chunked', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('take', 'itertools.take', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('drop', 'itertools.drop', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('windows', 'itertools.windows', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('cycle', 'itertools.cycle', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('pairs', 'itertools.pairs', 'Composable sequence iteration, chunking, windows, and transforms.'),
    f('unique', 'itertools.unique', 'Composable sequence iteration, chunking, windows, and transforms.')
  ],
  hashlib: [
    f('sha256', 'sha256 args...', 'Cryptographic hash operation.'),
    f('file_sha256', 'file_sha256 args...', 'Cryptographic hash operation.'),
    f('sha512', 'sha512 args...', 'Cryptographic hash operation.'),
    f('file_sha512', 'file_sha512 args...', 'Cryptographic hash operation.'),
    f('digest', 'digest args...', 'Cryptographic hash operation.'),
    f('file_digest', 'file_digest args...', 'Cryptographic hash operation.'),
    f('hmac_sha256', 'hmac_sha256 args...', 'Cryptographic hash operation.'),
    f('compare', 'compare args...', 'Cryptographic hash operation.'),
    f('to_hex', 'to_hex args...', 'Cryptographic hash operation.')
  ],
  argparse: [
    f('parse_args', 'parse_args args...', 'Standalone argparse operation.'),
    f('get', 'get args...', 'Standalone argparse operation.'),
    f('flag', 'flag args...', 'Standalone argparse operation.'),
    f('help', 'help args...', 'Standalone argparse operation.'),
    f('has', 'has args...', 'Standalone argparse operation.'),
    f('positionals', 'positionals args...', 'Standalone argparse operation.'),
    f('get_int', 'get_int args...', 'Standalone argparse operation.'),
    f('require', 'require args...', 'Standalone argparse operation.')
  ],
  logging: [
    f('set_level', 'set_level args...', 'Standalone logging operation.'),
    f('debug', 'debug args...', 'Standalone logging operation.'),
    f('info', 'info args...', 'Standalone logging operation.'),
    f('warning', 'warning args...', 'Standalone logging operation.'),
    f('error', 'error args...', 'Standalone logging operation.'),
    f('critical', 'critical args...', 'Standalone logging operation.'),
    f('records', 'records args...', 'Standalone logging operation.')
  ],
  zipfile: [
    f('create', 'create args...', 'Standalone zipfile operation.'),
    f('extract', 'extract args...', 'Standalone zipfile operation.'),
    f('is_zip', 'is_zip args...', 'Standalone zipfile operation.'),
    f('entries', 'entries args...', 'Standalone zipfile operation.'),
    f('read', 'read args...', 'Standalone zipfile operation.'),
    f('test', 'test args...', 'Standalone zipfile operation.')
  ],
  sqlite3: [
    f('open', 'open args...', 'Standalone sqlite3 operation.'),
    f('exec', 'exec args...', 'Standalone sqlite3 operation.'),
    f('query', 'query args...', 'Standalone sqlite3 operation.'),
    f('query_one', 'query_one args...', 'Standalone sqlite3 operation.'),
    f('tables', 'tables args...', 'Standalone sqlite3 operation.'),
    f('table_info', 'table_info args...', 'Standalone sqlite3 operation.'),
    f('execute_many', 'execute_many args...', 'Standalone sqlite3 operation.'),
    f('backup', 'backup args...', 'Standalone sqlite3 operation.')
  ],
  config: [
    f('parse', 'parse args...', 'Standalone config operation.'),
    f('get', 'get args...', 'Standalone config operation.'),
    f('get_int', 'get_int args...', 'Standalone config operation.'),
    f('get_bool', 'get_bool args...', 'Standalone config operation.'),
    f('section', 'section args...', 'Standalone config operation.'),
    f('merge', 'merge args...', 'Standalone config operation.')
  ],
  series: [
    f('sum', 'sum args...', 'Standalone series operation.'),
    f('mean', 'mean args...', 'Standalone series operation.'),
    f('min', 'min args...', 'Standalone series operation.'),
    f('max', 'max args...', 'Standalone series operation.'),
    f('diff', 'diff args...', 'Standalone series operation.'),
    f('lag', 'lag args...', 'Standalone series operation.'),
    f('moving_average', 'moving_average args...', 'Standalone series operation.'),
    f('cumulative_sum', 'cumulative_sum args...', 'Standalone series operation.'),
    f('returns', 'returns args...', 'Standalone series operation.'),
    f('normalize', 'normalize args...', 'Standalone series operation.')
  ],
  linear: [
    f('transpose', 'transpose args...', 'Standalone linear operation.'),
    f('multiply', 'multiply args...', 'Standalone linear operation.'),
    f('dot', 'dot args...', 'Standalone linear operation.'),
    f('add', 'add args...', 'Standalone linear operation.'),
    f('subtract', 'subtract args...', 'Standalone linear operation.'),
    f('scale', 'scale args...', 'Standalone linear operation.'),
    f('identity', 'identity args...', 'Standalone linear operation.'),
    f('determinant', 'determinant args...', 'Standalone linear operation.'),
    f('inverse', 'inverse args...', 'Standalone linear operation.'),
    f('norm', 'norm args...', 'Standalone linear operation.'),
    f('normalize', 'normalize args...', 'Standalone linear operation.')
  ],
  dataset: [
    f('row_count', 'row_count args...', 'Standalone dataset operation.'),
    f('select', 'select args...', 'Standalone dataset operation.'),
    f('describe', 'describe args...', 'Standalone dataset operation.'),
    f('train_test_split', 'train_test_split args...', 'Standalone dataset operation.'),
    f('columns', 'columns args...', 'Standalone dataset operation.'),
    f('filter_eq', 'filter_eq args...', 'Standalone dataset operation.'),
    f('unique', 'unique args...', 'Standalone dataset operation.'),
    f('split', 'split args...', 'Standalone dataset operation.')
  ],
  http_server: [
    f('get', 'get args...', 'Standalone http_server operation.'),
    f('post', 'post args...', 'Standalone http_server operation.'),
    f('put', 'put args...', 'Standalone http_server operation.'),
    f('patch', 'patch args...', 'Standalone http_server operation.'),
    f('delete', 'delete args...', 'Standalone http_server operation.'),
    f('listen', 'listen args...', 'Standalone http_server operation.'),
    f('text', 'text args...', 'Standalone http_server operation.'),
    f('json', 'json args...', 'Standalone http_server operation.'),
    f('response', 'response args...', 'Standalone http_server operation.'),
    f('method', 'method args...', 'Standalone http_server operation.'),
    f('path', 'path args...', 'Standalone http_server operation.'),
    f('query', 'query args...', 'Standalone http_server operation.'),
    f('body', 'body args...', 'Standalone http_server operation.'),
    f('header', 'header args...', 'Standalone http_server operation.'),
    f('param', 'param args...', 'Standalone http_server operation.'),
    f('route_count', 'route_count args...', 'Standalone http_server operation.')
  ],
  router: [
    f('get', 'get args...', 'Standalone router operation.'),
    f('post', 'post args...', 'Standalone router operation.'),
    f('put', 'put args...', 'Standalone router operation.'),
    f('delete', 'delete args...', 'Standalone router operation.'),
    f('path', 'path args...', 'Standalone router operation.'),
    f('param', 'param args...', 'Standalone router operation.'),
    f('handle', 'handle args...', 'Standalone router operation.'),
    f('handle_status', 'handle_status args...', 'Standalone router operation.'),
    f('route_count', 'route_count args...', 'Standalone router operation.')
  ],
  dns: [
    f('resolve', 'resolve args...', 'Standalone dns operation.'),
    f('resolve4', 'resolve4 args...', 'Standalone dns operation.'),
    f('resolve6', 'resolve6 args...', 'Standalone dns operation.'),
    f('reverse', 'reverse args...', 'Standalone dns operation.'),
    f('is_ip', 'is_ip args...', 'Standalone dns operation.'),
    f('lookup_mx', 'lookup_mx args...', 'Standalone dns operation.'),
    f('lookup_txt', 'lookup_txt args...', 'Standalone dns operation.')
  ],
  gui: [
    f('new', 'gui.new', 'Focused gui APIs for SE scenes.'),
    f('rect', 'gui.rect', 'Focused gui APIs for SE scenes.'),
    f('circle', 'gui.circle', 'Focused gui APIs for SE scenes.'),
    f('text', 'gui.text', 'Focused gui APIs for SE scenes.'),
    f('show', 'gui.show', 'Focused gui APIs for SE scenes.'),
    f('save', 'gui.save', 'Focused gui APIs for SE scenes.'),
    f('html', 'gui.html', 'Focused gui APIs for SE scenes.')
  ],
  window: [
    f('new', 'window.new', 'Focused window APIs for SE scenes.'),
    f('fullscreen', 'window.fullscreen', 'Focused window APIs for SE scenes.'),
    f('show', 'window.show', 'Focused window APIs for SE scenes.')
  ],
  canvas: [
    f('background', 'canvas.background', 'Focused canvas APIs for SE scenes.'),
    f('clear', 'canvas.clear', 'Focused canvas APIs for SE scenes.'),
    f('rect', 'canvas.rect', 'Focused canvas APIs for SE scenes.'),
    f('circle', 'canvas.circle', 'Focused canvas APIs for SE scenes.'),
    f('line', 'canvas.line', 'Focused canvas APIs for SE scenes.'),
    f('text', 'canvas.text', 'Focused canvas APIs for SE scenes.')
  ],
  input: [
    f('key_move', 'input.key_move', 'Focused input APIs for SE scenes.'),
    f('follow_mouse', 'input.follow_mouse', 'Focused input APIs for SE scenes.')
  ],
  sprite: [
    f('image', 'sprite.image', 'Focused sprite APIs for SE scenes.'),
    f('sprite', 'sprite.sprite', 'Focused sprite APIs for SE scenes.'),
    f('sprite_color', 'sprite.sprite_color', 'Focused sprite APIs for SE scenes.'),
    f('position', 'sprite.position', 'Focused sprite APIs for SE scenes.'),
    f('move', 'sprite.move', 'Focused sprite APIs for SE scenes.'),
    f('velocity', 'sprite.velocity', 'Focused sprite APIs for SE scenes.'),
    f('animate', 'sprite.animate', 'Focused sprite APIs for SE scenes.')
  ],
  physics: [
    f('velocity', 'physics.velocity', 'Focused physics APIs for SE scenes.'),
    f('rect_hit', 'physics.rect_hit', 'Focused physics APIs for SE scenes.'),
    f('circle_hit', 'physics.circle_hit', 'Focused physics APIs for SE scenes.'),
    f('distance', 'physics.distance', 'Focused physics APIs for SE scenes.'),
    f('vector', 'physics.vector', 'Focused physics APIs for SE scenes.'),
    f('particles', 'physics.particles', 'Focused physics APIs for SE scenes.'),
    f('camera', 'physics.camera', 'Focused physics APIs for SE scenes.')
  ],
  sound: [
    f('sound', 'sound.sound', 'Focused sound APIs for SE scenes.'),
    f('play', 'sound.play', 'Focused sound APIs for SE scenes.'),
    f('stop', 'sound.stop', 'Focused sound APIs for SE scenes.')
  ],
  keyboard: [
    f('key_move', 'keyboard.key_move', 'Focused keyboard APIs for SE scenes.')
  ],
  mouse: [
    f('follow_mouse', 'mouse.follow_mouse', 'Focused mouse APIs for SE scenes.'),
    f('camera', 'mouse.camera', 'Focused mouse APIs for SE scenes.')
  ],
  animation: [
    f('animate', 'animation.animate', 'Focused animation APIs for SE scenes.'),
    f('move', 'animation.move', 'Focused animation APIs for SE scenes.'),
    f('velocity', 'animation.velocity', 'Focused animation APIs for SE scenes.')
  ],
  scene: [
    f('new', 'scene.new', 'Focused scene APIs for SE scenes.'),
    f('background', 'scene.background', 'Focused scene APIs for SE scenes.'),
    f('clear', 'scene.clear', 'Focused scene APIs for SE scenes.'),
    f('html', 'scene.html', 'Focused scene APIs for SE scenes.'),
    f('save', 'scene.save', 'Focused scene APIs for SE scenes.'),
    f('show', 'scene.show', 'Focused scene APIs for SE scenes.')
  ],
  collision: [
    f('rect_hit', 'collision.rect_hit', 'Focused collision APIs for SE scenes.'),
    f('circle_hit', 'collision.circle_hit', 'Focused collision APIs for SE scenes.'),
    f('distance', 'collision.distance', 'Focused collision APIs for SE scenes.'),
    f('vector', 'collision.vector', 'Focused collision APIs for SE scenes.')
  ],
  image: [
    f('image', 'image.image', 'Focused image APIs for SE scenes.'),
    f('sprite', 'image.sprite', 'Focused image APIs for SE scenes.')
  ],
  audio: [
    f('sound', 'audio.sound', 'Focused audio APIs for SE scenes.'),
    f('play', 'audio.play', 'Focused audio APIs for SE scenes.'),
    f('stop', 'audio.stop', 'Focused audio APIs for SE scenes.')
  ]
};
Object.assign(MODULE_MEMBERS, INDEPENDENT_MEMBERS);

const MODULE_DESCRIPTIONS = Object.fromEntries(MODULES);
const BUILTIN_MODULES = MODULES.map(([name]) => name);

module.exports = {
  MODULES,
  MODULE_DESCRIPTIONS,
  BUILTIN_MODULES,
  MODULE_MEMBERS
};
