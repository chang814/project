/* 畅记复习调度：FSRS-6 记忆模型 + 回忆证据分级。
 *
 * 公式与默认参数移植自 open-spaced-repetition/ts-fsrs v6.0.0（MIT 许可），
 * 见 packages/fsrs/src/models/fsrs-6/。本文件只做本地计算，不联网、不上传日志。
 *
 * 学习证据（见 docs/review-algorithm-plan.md）：
 * - 跟练：只算“看过”，不改变记忆稳定性。
 * - 听音拼写：无提示一次拼对 = Good；只错一个键并自行改正 = Hard；
 *   用了提示或错两个键以上 = Again。暂不使用 Easy（速度需按个人基线校准，留待后续）。
 */
(function (root) {
  'use strict';

  const W = Object.freeze([
    0.212, 1.2931, 2.3065, 8.2956, 6.4133, 0.8334, 3.0194, 0.001, 1.8722, 0.1666,
    0.796, 1.4835, 0.0614, 0.2629, 1.6483, 0.6014, 1.8729, 0.5425, 0.0912, 0.0658, 0.1542,
  ]);
  const S_MIN = 0.001, S_MAX = 36500, D_MIN = 1, D_MAX = 10;
  const AGAIN = 1, HARD = 2, GOOD = 3, EASY = 4;
  const DAY = 86400000;
  const MAX_INTERVAL = 365;

  const clamp = (v, lo, hi) => Math.min(Math.max(v, lo), hi);
  const roundTo = (n, d) => { const f = 10 ** d; return Math.round(n * f) / f; };

  const DECAY = -W[20];
  const FACTOR = roundTo(Math.exp(Math.log(0.9) / DECAY) - 1, 8);

  function forgettingCurve(elapsedDays, stability) {
    return roundTo(Math.pow(1 + (FACTOR * elapsedDays) / stability, DECAY), 8);
  }
  function initStability(g) { return clamp(W[g - 1], S_MIN, S_MAX); }
  function initDifficulty(g) { return roundTo(W[4] - Math.exp((g - 1) * W[5]) + 1, 8); }
  function nextInterval(stability, retention) {
    return Math.max(Math.round((stability / FACTOR) * (Math.pow(retention, 1 / DECAY) - 1)), 1);
  }
  function linearDamping(delta, oldD) { return roundTo((delta * (10 - oldD)) / 9, 8); }
  function meanReversion(init, current) { return roundTo(W[7] * init + (1 - W[7]) * current, 8); }
  function nextDifficulty(d, g) {
    const next = d + linearDamping(-W[6] * (g - 3), d);
    return clamp(meanReversion(initDifficulty(EASY), next), D_MIN, D_MAX);
  }
  function nextRecallStability(d, s, r, g) {
    const hard = g === HARD ? W[15] : 1, easy = g === EASY ? W[16] : 1;
    return roundTo(clamp(s * (1 + Math.exp(W[8]) * (11 - d) * Math.pow(s, -W[9]) *
      (Math.exp((1 - r) * W[10]) - 1) * hard * easy), S_MIN, S_MAX), 8);
  }
  function nextForgetStability(d, s, r) {
    return roundTo(clamp(W[11] * Math.pow(d, -W[12]) * (Math.pow(s + 1, W[13]) - 1) *
      Math.exp((1 - r) * W[14]), S_MIN, S_MAX), 8);
  }
  function nextShortTermStability(s, g) {
    const sinc = Math.pow(s, -W[19]) * Math.exp(W[17] * (g - 3 + W[18]));
    return roundTo(clamp(s * (g >= HARD ? Math.max(sinc, 1) : sinc), S_MIN, S_MAX), 8);
  }
  /** FSRS-6 next_state with short-term scheduling enabled. */
  function nextState(state, elapsedDays, g) {
    if (!state || (!state.d && !state.s)) {
      return { d: clamp(initDifficulty(g), D_MIN, D_MAX), s: initStability(g) };
    }
    const { d, s } = state;
    const r = forgettingCurve(elapsedDays, s);
    let ns;
    if (elapsedDays === 0) ns = nextShortTermStability(s, g);
    else if (g === AGAIN) ns = clamp(roundTo(s / Math.exp(W[17] * W[18]), 8), S_MIN, nextForgetStability(d, s, r));
    else ns = nextRecallStability(d, s, r, g);
    return { d: nextDifficulty(d, g), s: ns };
  }

  // Local calendar days, so "today" means the learner's day, not UTC.
  function dayIndex(ms) {
    const t = new Date(ms);
    return Math.floor((Date.UTC(t.getFullYear(), t.getMonth(), t.getDate())) / DAY);
  }
  function startOfDayAfter(ms, days) {
    const t = new Date(ms);
    return new Date(t.getFullYear(), t.getMonth(), t.getDate() + days).getTime();
  }

  /** Turns what happened while typing one word into evidence. */
  function gradeAttempt(a) {
    if (a.mode !== 'dictation') return { kind: 'exposure' };
    if (a.hinted) return { kind: 'recall', grade: AGAIN };
    const errors = Math.max(0, Number(a.errors) || 0);
    if (errors === 0) return { kind: 'recall', grade: GOOD };
    if (errors === 1) return { kind: 'recall', grade: HARD };
    return { kind: 'recall', grade: AGAIN };
  }

  /** Applies one recall attempt. Returns a new entry; the input is not changed. */
  function review(entry, grade, now, retention) {
    const prev = entry && entry.s > 0 ? entry : null;
    const elapsed = prev && prev.last ? Math.max(0, dayIndex(now) - dayIndex(prev.last)) : 0;
    const state = nextState(prev ? { d: prev.d, s: prev.s } : null, elapsed, grade);
    const interval = grade === AGAIN ? 0 : Math.min(MAX_INTERVAL, nextInterval(state.s, retention));
    // A word forgotten today comes back tomorrow at the latest; in-session retries
    // handle the short term.
    const due = startOfDayAfter(now, grade === AGAIN ? 1 : interval);
    return {
      d: state.d, s: state.s, last: now, due,
      reps: ((prev && prev.reps) || 0) + 1,
      lapses: ((entry && entry.lapses) || 0) + (grade === AGAIN && prev ? 1 : 0),
      seen: (entry && entry.seen) || 0,
      g: grade, iv: grade === AGAIN ? 1 : interval,
    };
  }

  /**
   * Follow-along typing: counts as seen, never as remembered. A word that is
   * due is checked again tomorrow instead of being offered again and again.
   */
  function expose(entry, now) {
    const next = Object.assign({}, entry || {});
    next.seen = (next.seen || 0) + 1;
    next.seenAt = now;
    if (!(next.s > 0)) next.due = startOfDayAfter(now, 1);
    else if (next.due <= now) next.due = startOfDayAfter(now, 1);
    return next;
  }

  /**
   * Seeds an FSRS entry from the old fixed-interval record (streak 0–5,
   * intervals 1/3/7/14/30). Only the state is estimated; no log is invented.
   */
  function fromLegacy(old, now) {
    if (!old || !Number.isFinite(old.due)) return null;
    const streak = Math.max(0, Math.min(5, Number(old.streak) || 0));
    const lapses = Math.max(0, Number(old.lapses) || 0);
    const days = streak ? [1, 3, 7, 14, 30][streak - 1] : 0;
    const s = streak ? days : W[0];
    const d = clamp(initDifficulty(GOOD) + Math.min(lapses, 4) * 0.8, D_MIN, D_MAX);
    return { d, s, last: Math.min(now, old.due - days * DAY), due: old.due, reps: streak, lapses, seen: 0, legacy: true };
  }

  function retrievability(entry, now) {
    if (!entry || !(entry.s > 0) || !entry.last) return null;
    return forgettingCurve(Math.max(0, dayIndex(now) - dayIndex(entry.last)), entry.s);
  }
  function isDue(entry, now) {
    return !!entry && Number.isFinite(entry.due) && dayIndex(entry.due) <= dayIndex(now);
  }

  /** Validates an entry read from storage or a backup file. */
  function clean(e) {
    if (!e || typeof e !== 'object') return null;
    const num = (v, lo, hi) => Number.isFinite(v) && v >= lo && v <= hi;
    const out = {};
    if (e.s !== undefined || e.d !== undefined) {
      if (!num(e.s, S_MIN, S_MAX) || !num(e.d, D_MIN, D_MAX) || !num(e.last, 0, 8.64e15)) return null;
      out.s = e.s; out.d = e.d; out.last = e.last;
    }
    if (!num(e.due, 0, 8.64e15)) return null;
    out.due = e.due;
    out.reps = num(e.reps, 0, 1e6) ? Math.floor(e.reps) : 0;
    out.lapses = num(e.lapses, 0, 1e6) ? Math.floor(e.lapses) : 0;
    out.seen = num(e.seen, 0, 1e6) ? Math.floor(e.seen) : 0;
    if (num(e.seenAt, 0, 8.64e15)) out.seenAt = e.seenAt;
    if ([1, 2, 3, 4].includes(e.g)) out.g = e.g;
    if (num(e.iv, 0, 36500)) out.iv = e.iv;
    if (e.legacy === true) out.legacy = true;
    return out;
  }

  /**
   * Orders a practice pool: due words first (least likely remembered first),
   * then weak words, then new ones. When reviews pile up, new words take at
   * most `newShare` of each group so the backlog can shrink.
   */
  function orderPool(pool, recall, mistakes, now, groupSize, newShare) {
    const due = [], weak = [], fresh = [], later = [];
    for (const w of pool) {
      const e = recall[w.word];
      if (e && isDue(e, now)) due.push(w);
      else if (!e) (mistakes[w.word] ? weak : fresh).push(w);
      else if (mistakes[w.word]) weak.push(w);
      else later.push(w);
    }
    const risk = (w) => { const r = retrievability(recall[w.word], now); return r === null ? -1 : r; };
    due.sort((a, b) => risk(a) - risk(b) || recall[a.word].due - recall[b.word].due);
    later.sort((a, b) => recall[a.word].due - recall[b.word].due);
    const size = Math.max(1, groupSize);
    const reviewSlots = due.length + weak.length >= size ? Math.ceil(size * (1 - newShare)) : size;
    const group = [];
    const take = (list, n) => { while (n-- > 0 && list.length) group.push(list.shift()); };
    take(due, reviewSlots);
    take(weak, reviewSlots - group.length);
    take(fresh, size - group.length);
    take(due, size - group.length);
    take(weak, size - group.length);
    return group.concat(due, weak, fresh, later);
  }

  /**
   * Where a word missed in this session comes back: 3–5 words later the first
   * time, 10–15 the second. Returns -1 when it should wait for the next group.
   */
  function retryPosition(state) {
    const { tries, extraUsed, baseSize, index, length, random } = state;
    if (tries >= 2 || extraUsed >= Math.max(1, Math.ceil(baseSize * 0.3))) return -1;
    const rand = typeof random === 'function' ? random : Math.random;
    const [lo, hi] = tries === 0 ? [3, 5] : [10, 15];
    const gap = lo + Math.floor(rand() * (hi - lo + 1));
    const remaining = length - (index + 1);
    if (remaining < lo) return -1;
    return index + 1 + Math.min(gap, remaining);
  }

  const api = {
    VERSION: 'fsrs-6 (ts-fsrs v6.0.0 defaults)',
    WEIGHTS: W, AGAIN, HARD, GOOD, EASY,
    forgettingCurve, initStability, initDifficulty, nextInterval, nextDifficulty, nextState,
    gradeAttempt, review, expose, fromLegacy, retrievability, isDue, clean, orderPool, retryPosition, dayIndex,
  };
  root.ChangjiReview = api;
  if (typeof module === 'object' && module.exports) module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
