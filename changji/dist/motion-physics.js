/* Two anchored glyphs exchange an impulse at contact, then settle with damping. */
window.ChangjiMotion = {
 sampleCollision(strength = 1, initial = [0, 0], clearance = .4) {
  const dt = 1 / 240, duration = 280, frames = [[], []];
  const x = initial.map(v => Math.max(-3, Math.min(3, Number(v) || 0)));
  const velocity = [48 * strength, 0], stiffness = 640, damping = 31;
  const gap = Math.max(.2, Math.min(1.2, clearance));
  const first = [...x];
  for (let step = 0; step <= 72; step++) {
   if (step % 3 === 0) {
    for (let i = 0; i < 2; i++) frames[i].push({transform:`translateX(${x[i].toFixed(4)}px)`,offset:step / 72});
   }
   for (let i = 0; i < 2; i++) {
    velocity[i] += (-stiffness * x[i] - damping * velocity[i]) * dt;
    x[i] += velocity[i] * dt;
   }
   const overlap = x[0] - x[1] - gap;
   if (overlap > 0) {
    x[0] -= overlap / 2; x[1] += overlap / 2;
    if (velocity[0] > velocity[1]) {
     const impulse = (velocity[0] - velocity[1]) * .68;
     velocity[0] -= impulse; velocity[1] += impulse;
    }
   }
  }
  // Settle exactly at the anchor; there is no persistent layout displacement.
  for (let i=0;i<2;i++) {frames[i][0].transform=`translateX(${first[i]}px)`;frames[i].push({transform:'translateX(0px)',offset:1});frames[i][frames[i].length-2].offset=.96;}
  return {frames, duration};
 }
};
