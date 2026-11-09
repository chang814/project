/* Lifecycle/interaction checks without browser automation. Visual QA remains separate. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

function eventTarget() {
  const listeners = new Map();
  return {
    addEventListener(name, listener) {
      if (!listeners.has(name)) listeners.set(name, []);
      listeners.get(name).push(listener);
    },
    dispatch(name) { for (const listener of listeners.get(name) || []) listener(); }
  };
}

const pending = new Map();
let seq = 0;
const values = new Map();
const host = {
  style: {setProperty: (k,v) => values.set(k,v), removeProperty: k => values.delete(k)},
  setAttribute: (k,v) => values.set(k,v), removeAttribute: k => values.delete(k),
  getBoundingClientRect: () => ({left: 100, width: 600})
};
const letter = {
  isConnected: true, closest: () => host,
  getBoundingClientRect: () => ({left: 160, width: 40})
};
const window = eventTarget(), document = eventTarget(), media = eventTarget();
vm.runInNewContext(fs.readFileSync('dist/ink-wash.js','utf8'), {
  window, document, matchMedia: () => media,
  setTimeout: fn => { pending.set(++seq,fn); return seq; },
  clearTimeout: id => pending.delete(id)
});
const ink = window.ChangjiInk;
for (let i=0; i<100; i++) ink.pulse(letter);
assert.equal(pending.size, 1);
assert.equal(values.get('--halo-shift'), '-22px');
ink.pulse(letter,{deleting:true});
assert.equal(values.get('--halo-shift'), '-12px');
assert.equal(values.get('data-halo-moving'), 'backspace');
pending.values().next().value();
assert.equal(values.size, 0);
assert.equal(pending.size, 0);
ink.pulse(letter);
media.matches = true;
media.dispatch('change');
ink.pulse(letter);
assert.equal(values.size, 0);
media.matches = false;
ink.pulse(letter);
document.hidden = true;
document.dispatch('visibilitychange');
ink.pulse(letter);
assert.equal(values.size, 0);
console.log('PASS halo recenter, bounded movement, rapid input, reduced motion and background cleanup');
