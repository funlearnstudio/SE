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
const GAME_ALIAS_MEMBERS = [...MODULE_MEMBERS.game, ...MODULE_MEMBERS.game_extended];
delete MODULE_MEMBERS.game_extended;


const INDEPENDENT_MEMBERS = {
  re: [\n    f('find_all', 're find all', 'Pattern matching, extraction, replacement, and escaping..',
    f('count', 're count', 'Pattern matching, extraction, replacement, and escaping..',
    f('escape', 're escape', 'Pattern matching, extraction, replacement, and escaping..',
    f('groups', 're groups', 'Pattern matching, extraction, replacement, and escaping..',
    f('match', 're match', 'Pattern matching, extraction, replacement, and escaping..',
    f('search', 're search', 'Pattern matching, extraction, replacement, and escaping..',
    f('replace', 're replace', 'Pattern matching, extraction, replacement, and escaping..',
    f('split', 're split', 'Pattern matching, extraction, replacement, and escaping..'\n  ],
  itertools: [\n    f('chain', 'itertools chain', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('flatten', 'itertools flatten', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('chunked', 'itertools chunked', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('take', 'itertools take', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('drop', 'itertools drop', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('windows', 'itertools windows', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('cycle', 'itertools cycle', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('pairs', 'itertools pairs', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..',
    f('unique', 'itertools unique', 'Composable sequence iteration, chunking, windows, and lazy-style transforms..'\n  ],
  hashlib: [\n    f('sha256', 'hashlib sha256', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('sha512', 'hashlib sha512', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('file_sha256', 'hashlib file sha256', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('file_sha512', 'hashlib file sha512', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('hmac_sha256', 'hashlib hmac sha256', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('compare', 'hashlib compare', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..',
    f('to_hex', 'hashlib to hex', 'Cryptographic digests, file hashing, HMAC, and constant-time comparison..'\n  ],
  argparse: [\n    f('parser', 'argparse parser', 'Declare command-line options, parse arguments, and generate help text..',
    f('option', 'argparse option', 'Declare command-line options, parse arguments, and generate help text..',
    f('flag', 'argparse flag', 'Declare command-line options, parse arguments, and generate help text..',
    f('parse_args', 'argparse parse args', 'Declare command-line options, parse arguments, and generate help text..',
    f('help', 'argparse help', 'Declare command-line options, parse arguments, and generate help text..',
    f('get', 'argparse get', 'Declare command-line options, parse arguments, and generate help text..',
    f('has', 'argparse has', 'Declare command-line options, parse arguments, and generate help text..',
    f('positionals', 'argparse positionals', 'Declare command-line options, parse arguments, and generate help text..'\n  ],
  logging: [\n    f('get_logger', 'logging get logger', 'Named loggers, levels, formatting, and inspectable log records..',
    f('set_level', 'logging set level', 'Named loggers, levels, formatting, and inspectable log records..',
    f('debug', 'logging debug', 'Named loggers, levels, formatting, and inspectable log records..',
    f('info', 'logging info', 'Named loggers, levels, formatting, and inspectable log records..',
    f('warning', 'logging warning', 'Named loggers, levels, formatting, and inspectable log records..',
    f('error', 'logging error', 'Named loggers, levels, formatting, and inspectable log records..',
    f('critical', 'logging critical', 'Named loggers, levels, formatting, and inspectable log records..',
    f('format', 'logging format', 'Named loggers, levels, formatting, and inspectable log records..',
    f('records', 'logging records', 'Named loggers, levels, formatting, and inspectable log records..'\n  ],
  zipfile: [\n    f('create', 'zipfile create', 'Create, inspect, read, test, and extract ZIP archives..',
    f('open', 'zipfile open', 'Create, inspect, read, test, and extract ZIP archives..',
    f('entries', 'zipfile entries', 'Create, inspect, read, test, and extract ZIP archives..',
    f('read', 'zipfile read', 'Create, inspect, read, test, and extract ZIP archives..',
    f('write', 'zipfile write', 'Create, inspect, read, test, and extract ZIP archives..',
    f('extract', 'zipfile extract', 'Create, inspect, read, test, and extract ZIP archives..',
    f('test', 'zipfile test', 'Create, inspect, read, test, and extract ZIP archives..',
    f('is_zip', 'zipfile is zip', 'Create, inspect, read, test, and extract ZIP archives..'\n  ],
  sqlite3: [\n    f('connect', 'sqlite3 connect', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('execute', 'sqlite3 execute', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('executemany', 'sqlite3 executemany', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('query', 'sqlite3 query', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('query_one', 'sqlite3 query one', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('table_info', 'sqlite3 table info', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('begin', 'sqlite3 begin', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('commit', 'sqlite3 commit', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('rollback', 'sqlite3 rollback', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..',
    f('close', 'sqlite3 close', 'Parameterized SQLite queries, transactions, schema inspection, and connection lifecycle..'\n  ],
  config: [\n    f('parse', 'config parse', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('load', 'config load', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('get', 'config get', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('get_int', 'config get int', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('get_bool', 'config get bool', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('section', 'config section', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('merge', 'config merge', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('from_env', 'config from env', 'Structured application settings with typed access, sections, merging, and environment overrides..',
    f('validate', 'config validate', 'Structured application settings with typed access, sections, merging, and environment overrides..'\n  ],
  series: [\n    f('create', 'series create', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('sum', 'series sum', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('mean', 'series mean', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('min', 'series min', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('max', 'series max', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('diff', 'series diff', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('lag', 'series lag', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('moving_average', 'series moving average', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('cumulative_sum', 'series cumulative sum', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('returns', 'series returns', 'Time-series transforms, rolling statistics, differences, returns, and normalization..',
    f('normalize', 'series normalize', 'Time-series transforms, rolling statistics, differences, returns, and normalization..'\n  ],
  linear: [\n    f('vector', 'linear vector', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('identity', 'linear identity', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('add', 'linear add', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('subtract', 'linear subtract', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('scale', 'linear scale', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('transpose', 'linear transpose', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('multiply', 'linear multiply', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('determinant', 'linear determinant', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('inverse', 'linear inverse', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('solve', 'linear solve', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('dot', 'linear dot', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('norm', 'linear norm', 'Vector and linear algebra operations including matrix factorization helpers..',
    f('normalize', 'linear normalize', 'Vector and linear algebra operations including matrix factorization helpers..'\n  ],
  dataset: [\n    f('from_rows', 'dataset from rows', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('columns', 'dataset columns', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('select', 'dataset select', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('filter', 'dataset filter', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('sort', 'dataset sort', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('group_by', 'dataset group by', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('join', 'dataset join', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('unique', 'dataset unique', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('split', 'dataset split', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('describe', 'dataset describe', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..',
    f('train_test_split', 'dataset train test split', 'Tabular data selection, filtering, grouping, joins, summaries, and splits..'\n  ],
  http_server: [\n    f('create', 'http_server create', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('route', 'http_server route', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('middleware', 'http_server middleware', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('static', 'http_server static', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('listen', 'http_server listen', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('respond', 'http_server respond', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('status', 'http_server status', 'Build HTTP services with middleware, static assets, and response helpers..',
    f('header', 'http_server header', 'Build HTTP services with middleware, static assets, and response helpers..'\n  ],
  router: [\n    f('add', 'router add', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('get', 'router get', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('post', 'router post', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('put', 'router put', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('delete', 'router delete', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('match', 'router match', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('dispatch', 'router dispatch', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('params', 'router params', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..',
    f('not_found', 'router not found', 'Standalone route registration, parameter matching, dispatch, and fallback handlers..'\n  ],
  dns: [\n    f('resolve', 'dns resolve', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('resolve4', 'dns resolve4', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('resolve6', 'dns resolve6', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('reverse', 'dns reverse', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('is_ip', 'dns is ip', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('lookup_mx', 'dns lookup mx', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..',
    f('lookup_txt', 'dns lookup txt', 'Resolve IPv4/IPv6, reverse-lookup addresses, validate IPs, and query DNS records..'\n  ],
  gui: [\n    f('window', 'gui window', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('panel', 'gui panel', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('label', 'gui label', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('button', 'gui button', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('input', 'gui input', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('checkbox', 'gui checkbox', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('show', 'gui show', 'Compose browser interfaces from windows, panels, controls, and labels..',
    f('close', 'gui close', 'Compose browser interfaces from windows, panels, controls, and labels..'\n  ],
  window: [\n    f('create', 'window create', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('title', 'window title', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('resize', 'window resize', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('fullscreen', 'window fullscreen', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('center', 'window center', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('close', 'window close', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('width', 'window width', 'Manage browser game windows, dimensions, titles, and fullscreen state..',
    f('height', 'window height', 'Manage browser game windows, dimensions, titles, and fullscreen state..'\n  ],
  canvas: [\n    f('size', 'canvas size', 'Draw and export canvas scenes with size and primitive helpers..',
    f('clear', 'canvas clear', 'Draw and export canvas scenes with size and primitive helpers..',
    f('rect', 'canvas rect', 'Draw and export canvas scenes with size and primitive helpers..',
    f('circle', 'canvas circle', 'Draw and export canvas scenes with size and primitive helpers..',
    f('line', 'canvas line', 'Draw and export canvas scenes with size and primitive helpers..',
    f('text', 'canvas text', 'Draw and export canvas scenes with size and primitive helpers..',
    f('image', 'canvas image', 'Draw and export canvas scenes with size and primitive helpers..',
    f('save', 'canvas save', 'Draw and export canvas scenes with size and primitive helpers..'\n  ],
  input: [\n    f('key_down', 'input key down', 'Track key and pointer state and register input callbacks..',
    f('key_pressed', 'input key pressed', 'Track key and pointer state and register input callbacks..',
    f('mouse_position', 'input mouse position', 'Track key and pointer state and register input callbacks..',
    f('mouse_down', 'input mouse down', 'Track key and pointer state and register input callbacks..',
    f('on_key', 'input on key', 'Track key and pointer state and register input callbacks..',
    f('on_click', 'input on click', 'Track key and pointer state and register input callbacks..'\n  ],
  sprite: [\n    f('load', 'sprite load', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('draw', 'sprite draw', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('scale', 'sprite scale', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('rotate', 'sprite rotate', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('flip', 'sprite flip', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('animate', 'sprite animate', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('remove', 'sprite remove', 'Load, draw, transform, animate, and inspect sprite bounds..',
    f('bounds', 'sprite bounds', 'Load, draw, transform, animate, and inspect sprite bounds..'\n  ],
  physics: [\n    f('body', 'physics body', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('velocity', 'physics velocity', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('gravity', 'physics gravity', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('force', 'physics force', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('integrate', 'physics integrate', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('collide', 'physics collide', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('distance', 'physics distance', 'Integrate simple bodies, forces, gravity, and collision checks..',
    f('clamp', 'physics clamp', 'Integrate simple bodies, forces, gravity, and collision checks..'\n  ],
  sound: [\n    f('tone', 'sound tone', 'Generate tones and manage sound playback, volume, and fades..',
    f('beep', 'sound beep', 'Generate tones and manage sound playback, volume, and fades..',
    f('noise', 'sound noise', 'Generate tones and manage sound playback, volume, and fades..',
    f('volume', 'sound volume', 'Generate tones and manage sound playback, volume, and fades..',
    f('play', 'sound play', 'Generate tones and manage sound playback, volume, and fades..',
    f('stop', 'sound stop', 'Generate tones and manage sound playback, volume, and fades..',
    f('fade', 'sound fade', 'Generate tones and manage sound playback, volume, and fades..'\n  ],
  keyboard: [\n    f('pressed', 'keyboard pressed', 'Query keyboard state and register key press/release callbacks..',
    f('just_pressed', 'keyboard just pressed', 'Query keyboard state and register key press/release callbacks..',
    f('just_released', 'keyboard just released', 'Query keyboard state and register key press/release callbacks..',
    f('key_code', 'keyboard key code', 'Query keyboard state and register key press/release callbacks..',
    f('on_press', 'keyboard on press', 'Query keyboard state and register key press/release callbacks..',
    f('on_release', 'keyboard on release', 'Query keyboard state and register key press/release callbacks..'\n  ],
  mouse: [\n    f('position', 'mouse position', 'Read pointer position/buttons and handle movement, clicks, and wheel input..',
    f('button_down', 'mouse button down', 'Read pointer position/buttons and handle movement, clicks, and wheel input..',
    f('just_clicked', 'mouse just clicked', 'Read pointer position/buttons and handle movement, clicks, and wheel input..',
    f('wheel', 'mouse wheel', 'Read pointer position/buttons and handle movement, clicks, and wheel input..',
    f('on_move', 'mouse on move', 'Read pointer position/buttons and handle movement, clicks, and wheel input..',
    f('on_click', 'mouse on click', 'Read pointer position/buttons and handle movement, clicks, and wheel input..'\n  ],
  animation: [\n    f('tween', 'animation tween', 'Create cancellable tweens, sequences, repeats, and easing curves..',
    f('sequence', 'animation sequence', 'Create cancellable tweens, sequences, repeats, and easing curves..',
    f('repeat', 'animation repeat', 'Create cancellable tweens, sequences, repeats, and easing curves..',
    f('ease', 'animation ease', 'Create cancellable tweens, sequences, repeats, and easing curves..',
    f('cancel', 'animation cancel', 'Create cancellable tweens, sequences, repeats, and easing curves..',
    f('is_running', 'animation is running', 'Create cancellable tweens, sequences, repeats, and easing curves..'\n  ],
  scene: [\n    f('new', 'scene new', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('background', 'scene background', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('clear', 'scene clear', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('add', 'scene add', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('remove', 'scene remove', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('transition', 'scene transition', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('save', 'scene save', 'Create and manage scenes, backgrounds, objects, and transitions..',
    f('load', 'scene load', 'Create and manage scenes, backgrounds, objects, and transitions..'\n  ],
  collision: [\n    f('point_rect', 'collision point rect', 'Test common point, rectangle, and circle collision shapes..',
    f('rect_rect', 'collision rect rect', 'Test common point, rectangle, and circle collision shapes..',
    f('circle_circle', 'collision circle circle', 'Test common point, rectangle, and circle collision shapes..',
    f('circle_rect', 'collision circle rect', 'Test common point, rectangle, and circle collision shapes..',
    f('overlap', 'collision overlap', 'Test common point, rectangle, and circle collision shapes..',
    f('sweep', 'collision sweep', 'Test common point, rectangle, and circle collision shapes..'\n  ],
  image: [\n    f('load', 'image load', 'Load, crop, resize, transform, inspect, and save images..',
    f('draw', 'image draw', 'Load, crop, resize, transform, inspect, and save images..',
    f('crop', 'image crop', 'Load, crop, resize, transform, inspect, and save images..',
    f('resize', 'image resize', 'Load, crop, resize, transform, inspect, and save images..',
    f('flip', 'image flip', 'Load, crop, resize, transform, inspect, and save images..',
    f('pixel', 'image pixel', 'Load, crop, resize, transform, inspect, and save images..',
    f('dimensions', 'image dimensions', 'Load, crop, resize, transform, inspect, and save images..',
    f('save', 'image save', 'Load, crop, resize, transform, inspect, and save images..'\n  ],
  audio: [\n    f('load', 'audio load', 'Load and control audio playback, looping, duration, and position..',
    f('play', 'audio play', 'Load and control audio playback, looping, duration, and position..',
    f('pause', 'audio pause', 'Load and control audio playback, looping, duration, and position..',
    f('stop', 'audio stop', 'Load and control audio playback, looping, duration, and position..',
    f('volume', 'audio volume', 'Load and control audio playback, looping, duration, and position..',
    f('loop', 'audio loop', 'Load and control audio playback, looping, duration, and position..',
    f('duration', 'audio duration', 'Load and control audio playback, looping, duration, and position..',
    f('position', 'audio position', 'Load and control audio playback, looping, duration, and position..'\n  ]
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
