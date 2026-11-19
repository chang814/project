// Checks for dist/review-scheduler.js. Run: node qa-review.cjs
'use strict';
const assert = require('node:assert/strict');
const R = require('./dist/review-scheduler.js');

let checks = 0;
const ok = (cond, msg) => { assert.ok(cond, msg); checks++; };
const near = (a, b, msg, eps = 1e-9) => { assert.ok(Math.abs(a - b) <= eps, `${msg}: ${a} vs ${b}`); checks++; };

// ---------------------------------------------------------------------------
// 1. Formulas. A second transcription of FSRS-6 (ts-fsrs v6.0.0,
//    models/fsrs-6/algorithm.ts), written independently of the module, so a
//    slip in either copy shows up as a mismatch.
// ---------------------------------------------------------------------------
const w = [0.212, 1.2931, 2.3065, 8.2956, 6.4133, 0.8334, 3.0194, 0.001, 1.8722, 0.1666,
  0.796, 1.4835, 0.0614, 0.2629, 1.6483, 0.6014, 1.8729, 0.5425, 0.0912, 0.0658, 0.1542];
assert.deepEqual([...R.WEIGHTS], w); checks++;
const rt = (n) => Math.round(n * 1e8) / 1e8;
const cl = (v, a, b) => Math.min(Math.max(v, a), b);
const decay = -w[20];
const factor = rt(Math.exp(Math.pow(decay, -1) * Math.log(0.9)) - 1);
const curve = (t, s) => rt(Math.pow(1 + (factor * t) / s, decay));
const d0 = (g) => rt(w[4] - Math.exp((g - 1) * w[5]) + 1);
const ivl = (s, r) => Math.max(Math.round((s / factor) * (Math.pow(r, 1 / decay) - 1)), 1);
function step(st, t, g) {
  if (!st) return { s: cl(w[g - 1], 0.001, 36500), d: cl(d0(g), 1, 10) };
  const { s, d } = st, r = curve(t, s);
  let ns;
  if (t === 0) {
    const sinc = Math.pow(s, -w[19]) * Math.exp(w[17] * (g - 3 + w[18]));
    ns = rt(cl(s * (g >= 2 ? Math.max(sinc, 1) : sinc), 0.001, 36500));
  } else if (g === 1) {
    const forget = rt(cl(w[11] * Math.pow(d, -w[12]) * (Math.pow(s + 1, w[13]) - 1) * Math.exp((1 - r) * w[14]), 0.001, 36500));
    ns = cl(rt(s / Math.exp(w[17] * w[18])), 0.001, forget);
  } else {
    const hp = g === 2 ? w[15] : 1, eb = g === 4 ? w[16] : 1;
    ns = rt(cl(s * (1 + Math.exp(w[8]) * (11 - d) * Math.pow(s, -w[9]) * (Math.exp((1 - r) * w[10]) - 1) * hp * eb), 0.001, 36500));
  }
  const nd = d + rt((-w[6] * (g - 3) * (10 - d)) / 9);
  return { s: ns, d: cl(rt(w[7] * d0(4) + (1 - w[7]) * nd), 1, 10) };
}

let seed = 20261119;
const rand = () => ((seed = (seed * 1664525 + 1013904223) >>> 0) / 4294967296);
for (let run = 0; run < 2000; run++) {
  let a = null, b = null;
  const len = 1 + Math.floor(rand() * 14);
  for (let i = 0; i < len; i++) {
    const g = 1 + Math.floor(rand() * 4), t = i === 0 ? 0 : rand() < 0.2 ? 0 : Math.floor(rand() * 120);
    a = step(a, t, g); b = R.nextState(b, t, g);
    near(b.s, a.s, `stability run ${run} step ${i}`); near(b.d, a.d, `difficulty run ${run} step ${i}`);
    for (const r of [0.85, 0.9, 0.95]) assert.equal(R.nextInterval(b.s, r), ivl(a.s, r)); checks += 3;
    near(R.forgettingCurve(t, b.s), curve(t, a.s), 'forgetting curve');
  }
}

// Properties that follow from FSRS's definitions, independent of transcription.
near(R.forgettingCurve(0, 5), 1, 'nothing forgotten at t = 0');
for (const s of [0.5, 2.3065, 10, 80]) near(R.forgettingCurve(s, s), 0.9, 'stability is the 90% point', 1e-6);
for (const s of [2, 7.4, 30]) assert.equal(R.nextInterval(s, 0.9), Math.max(1, Math.round(s))), checks++;
ok(R.nextInterval(10, 0.95) < R.nextInterval(10, 0.9) && R.nextInterval(10, 0.9) < R.nextInterval(10, 0.85), 'higher target, shorter interval');
near(R.initStability(R.GOOD), 2.3065, 'first Good');
ok(R.initDifficulty(R.AGAIN) > R.initDifficulty(R.GOOD), 'forgetting first makes a word harder');
{
  const s0 = R.nextState(null, 0, R.GOOD), later = R.nextState(s0, 5, R.AGAIN);
  ok(later.s <= s0.s, 'forgetting never adds stability');
  const good = R.nextState(s0, 3, R.GOOD), hard = R.nextState(s0, 3, R.HARD);
  ok(good.s > hard.s && hard.s > s0.s, 'Good grows more than Hard, both grow');
}

// ---------------------------------------------------------------------------
// 2. Evidence: only unaided recall counts as remembering.
// ---------------------------------------------------------------------------
assert.deepEqual(R.gradeAttempt({ mode: 'follow', errors: 0 }), { kind: 'exposure' }); checks++;
assert.equal(R.gradeAttempt({ mode: 'dictation', errors: 0 }).grade, R.GOOD); checks++;
assert.equal(R.gradeAttempt({ mode: 'dictation', errors: 1 }).grade, R.HARD); checks++;
assert.equal(R.gradeAttempt({ mode: 'dictation', errors: 3 }).grade, R.AGAIN); checks++;
assert.equal(R.gradeAttempt({ mode: 'dictation', errors: 0, hinted: true }).grade, R.AGAIN); checks++;

const day = 86400000, t0 = new Date(2026, 10, 2, 9, 0).getTime();
{
  const first = R.review(null, R.GOOD, t0, 0.9);
  assert.equal(first.iv, 2); checks++;
  ok(R.dayIndex(first.due) === R.dayIndex(t0) + 2 && new Date(first.due).getHours() === 0, 'due at the start of a day');
  ok(!R.isDue(first, t0 + day) && R.isDue(first, t0 + 2 * day), 'due on day two, not before');
  // Same day, typed right again: the model's short-term rule, no big jump.
  const again = R.review(first, R.GOOD, t0 + 3600000, 0.9);
  ok(again.iv <= first.iv + 1, 'a second success on the same day barely moves the interval');
  // Two days later, remembered: interval grows.
  const later = R.review(first, R.GOOD, t0 + 2 * day, 0.9);
  ok(later.iv > first.iv, 'cross-day success lengthens the interval');
  const lapse = R.review(later, R.AGAIN, t0 + 9 * day, 0.9);
  ok(lapse.lapses === 1 && R.isDue(lapse, t0 + 10 * day) && lapse.s < later.s, 'a lapse is due tomorrow with lower stability');
}
{
  // Follow-along never builds memory, and defers a due word by a day.
  const fresh = R.expose(null, t0);
  ok(!(fresh.s > 0) && fresh.seen === 1 && R.isDue(fresh, t0 + day), 'seen words get a recall check tomorrow');
  const learned = R.review(null, R.GOOD, t0, 0.9), dueDay = learned.due + 3600000;
  const seen = R.expose(learned, dueDay);
  ok(seen.s === learned.s && seen.d === learned.d && seen.last === learned.last, 'follow-along leaves stability alone');
  ok(!R.isDue(seen, dueDay) && R.isDue(seen, dueDay + day), 'a due word seen in follow-along is checked again tomorrow');
}

// ---------------------------------------------------------------------------
// 3. Old 1/3/7/14/30 records carry over without inventing history.
// ---------------------------------------------------------------------------
{
  const m = R.fromLegacy({ streak: 3, lapses: 1, due: t0 + 7 * day }, t0);
  ok(m.s === 7 && m.due === t0 + 7 * day && m.legacy === true && m.reps === 3, 'legacy streak 3 → 7-day stability, same due date');
  ok(R.fromLegacy({ streak: 0, lapses: 2, due: t0 }, t0).s === w[0], 'legacy streak 0 starts from Again stability');
  ok(R.fromLegacy({}, t0) === null, 'broken legacy entry is skipped');
}

// ---------------------------------------------------------------------------
// 4. Backup validation.
// ---------------------------------------------------------------------------
ok(R.clean({ s: 3, d: 5, last: t0, due: t0 + day, reps: 2 }).s === 3, 'valid entry kept');
ok(R.clean({ s: -1, d: 5, last: t0, due: t0 }) === null, 'negative stability rejected');
ok(R.clean({ s: 3, d: 50, last: t0, due: t0 }) === null, 'difficulty out of range rejected');
ok(R.clean({ due: 'soon' }) === null, 'bad due rejected');
ok(R.clean({ due: t0, seen: 2 }).seen === 2, 'seen-only entry kept');

// ---------------------------------------------------------------------------
// 5. Choosing words: due first, riskiest first, new words limited in a backlog.
// ---------------------------------------------------------------------------
{
  const now = t0 + 30 * day;
  const pool = Array.from({ length: 40 }, (_, i) => ({ word: 'w' + i }));
  const recall = {};
  for (let i = 0; i < 20; i++) recall['w' + i] = { s: 1 + i, d: 5, last: now - 10 * day, due: now - day };
  for (let i = 20; i < 25; i++) recall['w' + i] = { s: 50, d: 5, last: now - day, due: now + 40 * day };
  const order = R.orderPool(pool, recall, { w30: { count: 1 } }, now, 10, 0.25);
  const group = order.slice(0, 10).map((w) => w.word);
  assert.equal(order.length, 40); checks++;
  assert.equal(new Set(order.map((w) => w.word)).size, 40); checks++;
  assert.equal(group.filter((x) => +x.slice(1) < 20).length, 8); checks++;
  ok(group[0] === 'w0', 'the word most likely forgotten comes first');
  ok(group.filter((x) => +x.slice(1) >= 25).length === 2, 'two new or weak words still make it in');
  ok(!group.some((x) => +x.slice(1) >= 20 && +x.slice(1) < 25), 'not-yet-due words stay out');
  const light = R.orderPool(pool, { w0: recall.w0 }, {}, now, 10, 0.25).slice(0, 10).map((w) => w.word);
  ok(light[0] === 'w0' && light.length === 10, 'with little due, new words fill the group');
}

// ---------------------------------------------------------------------------
// 6. Retries inside a group.
// ---------------------------------------------------------------------------
{
  const base = { baseSize: 20, extraUsed: 0, index: 2, length: 20 };
  const lo = R.retryPosition({ ...base, tries: 0, random: () => 0 });
  const hi = R.retryPosition({ ...base, tries: 0, random: () => 0.999 });
  ok(lo === 6 && hi === 8, 'first miss returns after 3–5 other words');
  ok(R.retryPosition({ ...base, tries: 1, random: () => 0 }) === 13, 'second miss returns after 10+ words');
  ok(R.retryPosition({ ...base, tries: 2, random: () => 0 }) === -1, 'at most two extra tries');
  ok(R.retryPosition({ ...base, tries: 0, extraUsed: 6, random: () => 0 }) === -1, 'extra tries capped at 30% of the group');
  ok(R.retryPosition({ ...base, tries: 0, index: 18, random: () => 0 }) === -1, 'too close to the end waits for the next group');
}

console.log(`Review scheduler checks passed: ${checks}`);
