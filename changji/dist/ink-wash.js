/* Move the existing halo, without adding layers or restarting a flash. */
(() => {
  const timers = new Map();
  const reduced = matchMedia('(prefers-reduced-motion: reduce)');
  function clear(host) {
    for (const [owner, timer] of timers) {
      if (host && owner !== host && !host.contains(owner)) continue;
      clearTimeout(timer);
      owner.style.removeProperty('--halo-shift');
      owner.removeAttribute('data-halo-moving');
      timers.delete(owner);
    }
  }
  function pulse(letter, { strength = 1, deleting = false } = {}) {
    if (reduced.matches || document.hidden || !letter?.isConnected) return;
    const host = letter.closest('#word-wrap, #passage-line, #preview-scene');
    if (!host) return;
    const bounds = host.getBoundingClientRect(), glyph = letter.getBoundingClientRect();
    if (!bounds.width || !glyph.width) return;
    const offset = (glyph.left + glyph.width / 2 - bounds.left - bounds.width / 2) * .15;
    const limit = deleting ? 12 : 22;
    const shift = Math.max(-limit, Math.min(limit, offset * strength));
    host.style.setProperty('--halo-shift', shift + 'px');
    host.setAttribute('data-halo-moving', deleting ? 'backspace' : 'typing');
    clearTimeout(timers.get(host));
    timers.set(host, setTimeout(() => clear(host), 170));
  }
  document.addEventListener('visibilitychange', () => { if (document.hidden) clear(); });
  reduced.addEventListener('change', () => { if (reduced.matches) clear(); });
  window.addEventListener('pagehide', () => clear());
  window.ChangjiInk = Object.freeze({ pulse, clear });
})();
