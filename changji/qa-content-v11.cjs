const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const context = vm.createContext({window: {}});
for (const file of ['words.js', 'words4.js', 'words-basic.js']) {
  vm.runInContext(fs.readFileSync('dist/' + file, 'utf8'), context);
}
for (const pool of [context.window.WORDS, context.window.WORDS4, context.window.WORDS_BASIC]) {
  assert(pool.length > 0);
  assert.equal(new Set(pool.map(entry => entry.word)).size, pool.length);
  assert(pool.every(entry => entry.word && entry.zh && !entry.examExamples && !entry.examples));
}
assert(!fs.existsSync('dist/content-v1.1.js'));
console.log('PASS original demo entries and no bundled exam content');
