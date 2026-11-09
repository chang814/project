const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const scope={window:{}};vm.createContext(scope);vm.runInContext(fs.readFileSync('dist/motion-physics.js','utf8'),scope);
const sample=scope.window.ChangjiMotion.sampleCollision;
for(const force of [.8,1.65]){
 const result=sample(force,[.6,-.3]);
 assert.equal(result.duration,280);
 for(const frames of result.frames){
  assert.equal(frames.at(-1).transform,'translateX(0px)');assert.equal(frames.at(-1).offset,1);
  assert(frames.every((frame,i)=>i===0||frame.offset>frames[i-1].offset));
  assert(frames.every(frame=>Number.isFinite(parseFloat(frame.transform.slice(11)))));
  assert(Math.max(...frames.map(frame=>Math.abs(parseFloat(frame.transform.slice(11)))))<4);
 }
 assert(result.frames[1].some(frame=>parseFloat(frame.transform.slice(11))>.1),'Neighbor receives impulse');
}
assert(Math.max(...sample(1.65).frames[0].map(f=>parseFloat(f.transform.slice(11))))>Math.max(...sample(.8).frames[0].map(f=>parseFloat(f.transform.slice(11)))));
const app=fs.readFileSync('dist/app.js','utf8'),passage=fs.readFileSync('dist/passage.js','utf8');
assert(app.includes('exitGlyph(s,true)'));assert(passage.includes('exitGlyph(letter,true)'));
assert(app.includes('duration:deleting?220:360'));
const animationCalls=[];
const glyph=()=>({tagName:'SPAN',nextElementSibling:null,getAnimations:()=>[],animate:(frames,timing)=>animationCalls.push({frames,timing})});
const active=glyph();active.nextElementSibling=glyph();
const motionScope={window:scope.window,db:{settings:{motion:'collision'}},effectsAllowed:()=>true,getComputedStyle:()=>({transform:'none'}),letterEffects:{}};
vm.createContext(motionScope);
vm.runInContext(app.slice(app.indexOf('function pulseInk('),app.indexOf('let previewTimers=')),motionScope);
motionScope.animateLetter(active);assert.equal(animationCalls.length,2);assert(animationCalls.every(a=>a.timing.duration===280));
animationCalls.length=0;motionScope.effectsAllowed=()=>false;motionScope.animateLetter(active);assert.equal(animationCalls.length,0);
motionScope.effectsAllowed=()=>true;motionScope.db.settings.motion='none';motionScope.animateLetter(active);assert.equal(animationCalls.length,0);
console.log('PASS collision impulse, finite bounded displacement, ordered samples, exact rest, distinct strengths and deletion routing');
