function parseSeCheckOutput(output) {
  const text = String(output || '').replace(/\r\n/g, '\n').trim();
  if (!text) return null;

  const lineMatch = text.match(/Error on line\s+(\d+)/i);
  const line = lineMatch ? Math.max(0, Number(lineMatch[1]) - 1) : 0;

  const parts = text.split(/\n\s*\n/).map((part) => part.trim()).filter(Boolean);
  let message = parts.find((part) => !/^Error on line\b/i.test(part) && !/^\d+\s*\|/.test(part) && !/^Try:/i.test(part));
  if (!message) {
    message = text.split('\n').find((part) => part.trim() && !/^Error on line\b/i.test(part.trim())) || 'SE check failed.';
  }

  const caretMatch = text.match(/\n\s*\|([ \t]*)\^/);
  const column = caretMatch ? caretMatch[1].replace(/\t/g, '    ').length : 0;
  const hintMatch = text.match(/(?:^|\n)Try:\s*(.+)$/im);
  const hint = hintMatch ? hintMatch[1].trim() : '';

  return {
    line,
    column,
    message: message.trim(),
    hint,
    raw: text
  };
}

module.exports = { parseSeCheckOutput };
