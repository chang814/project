const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync('dist/app.js','utf8');
class Element{closest(selector){return selector==='#practice-view'?{}:null;}}
let scale=1,updates=0,stops=0,prevented=0;
const context={Element,view:'practice',$:()=>({open:false}),Number,Math,wordZoom:()=>scale,setWordZoom:value=>{scale=Math.max(.6,Math.min(1.6,value));updates++;},stopWheelMotion:()=>stops++};
vm.createContext(context);
const handler=source.slice(source.indexOf('function handleWordZoomWheel('),source.indexOf("document.addEventListener('wheel',handleWordZoomWheel"));
vm.runInContext(handler,context);
const event=(extra={})=>({target:new Element(),ctrlKey:true,metaKey:false,altKey:false,deltaY:-10,deltaMode:0,preventDefault:()=>prevented++,...extra});
context.handleWordZoomWheel(event({ctrlKey:false}));assert.equal(updates,0);assert.equal(prevented,0);
context.handleWordZoomWheel(event());assert(scale>1&&scale<1.02);assert.equal(stops,1);assert.equal(prevented,1);
const larger=scale;context.handleWordZoomWheel(event({deltaY:10}));assert(scale<larger);assert(Math.abs(scale-1)<1e-8);
for(let i=0;i<100;i++)context.handleWordZoomWheel(event({deltaY:-240}));assert.equal(scale,1.6);
for(let i=0;i<100;i++)context.handleWordZoomWheel(event({deltaY:240}));assert.equal(scale,.6);
let before=updates;context.handleWordZoomWheel(event({target:{}}));assert.equal(updates,before);
context.$=()=>({open:true});context.handleWordZoomWheel(event());assert.equal(updates,before);
context.$=()=>({open:false});context.view='stats';context.handleWordZoomWheel(event());assert.equal(updates,before);
console.log('PASS wheel routing, continuous increments, inverse movement, limits, settings and other views excluded');
