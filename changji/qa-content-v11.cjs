const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const context = vm.createContext({window: {}});
for (const file of ['words.js', 'words4.js', 'words-basic.js']) {
  vm.runInContext(fs.readFileSync('dist/' + file, 'utf8'), context);
}
const {WORDS, WORDS4, WORDS_BASIC} = context.window;
const isPair = (p) => p && typeof p.en === 'string' && p.en && typeof p.zh === 'string' && p.zh;
for (const pool of [WORDS, WORDS4, WORDS_BASIC]) {
  assert(pool.length > 0);
  assert.equal(new Set(pool.map(entry => entry.word)).size, pool.length);
  // Exam passages and exam-sourced sentences stay out of the public copy.
  assert(pool.every(entry => entry.word && entry.zh && !entry.examExamples && !entry.examSentenceSource));
  assert(pool.every(entry => /^[A-Za-z]+(?:[-'][A-Za-z]+)*$/.test(entry.word)));
}
// CET-4 / CET-6 come from KyleBing/english-vocabulary (BSD-3-Clause); its notice must ship with them.
for (const pool of [WORDS, WORDS4]) {
  assert(pool.length > 3000);
  assert(pool.every(entry => Array.isArray(entry.meanings) && entry.meanings.length && entry.meanings.every(m => m.text)));
  assert(pool.every(entry => (entry.examples || []).every(isPair) && (entry.phrases || []).every(isPair)));
  assert(pool.every(entry => !entry.sentence || new RegExp('\\b' + entry.word + '\\b', 'i').test(entry.sentence)));
}
assert(fs.readFileSync('dist/KYLEBING-LICENSE.txt', 'utf8').includes('Copyright (c) 2022-2026, KyleBing'));
assert(fs.readFileSync('THIRD-PARTY-NOTICES.md', 'utf8').includes('KyleBing/english-vocabulary'));
assert(!fs.existsSync('dist/content-v1.1.js'));
console.log(`PASS word lists: CET-6 ${WORDS.length}, CET-4 ${WORDS4.length}, basic ${WORDS_BASIC.length}; KyleBing notice shipped; no exam content`);
