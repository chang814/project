'use strict';
const fs = require('node:fs');
const path = require('node:path');

const courses = new Map([
  ['cet6', ['words.js', 'WORDS']],
  ['cet4', ['words4.js', 'WORDS4']],
  ['basic', ['words-basic.js', 'WORDS_BASIC']]
]);

try {
  const [course, source] = process.argv.slice(2);
  if (!courses.has(course) || !source) {
    throw new Error('用法：npm run import:vocab -- <cet6|cet4|basic> <词库文件>');
  }
  const [file, globalName] = courses.get(course);
  const lines = fs.readFileSync(source, 'utf8').replace(/^\uFEFF/, '').split(/\r?\n/);
  const entries = new Map();
  lines.forEach((line, i) => {
    if (!line.trim()) return;
    const [spelling, zh = '', en = '', ...extra] = line.split('\t').map(field => field.trim());
    const word = spelling.toLowerCase();
    if (!/^[a-z]+(?:[-'][a-z]+)*$/.test(word) || extra.length) {
      throw new Error('第 ' + (i + 1) + ' 行格式有误，请使用英文单词及可选的 Tab 分隔释义。');
    }
    entries.set(word, {word, zh: zh || '暂无释义', en, definitions: en ? [en] : []});
  });
  if (!entries.size) throw new Error('文件里没有可导入的词条。');
  const outputDir = path.join(__dirname, '..', '.local-vocabulary');
  fs.mkdirSync(outputDir, {recursive: true});
  fs.writeFileSync(path.join(outputDir, file), 'window.' + globalName + '=' + JSON.stringify([...entries.values()]) + ';\n', 'utf8');
  console.log('已导入 ' + entries.size + ' 词到 ' + course + '。打开或刷新本地网页即可使用。');
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
