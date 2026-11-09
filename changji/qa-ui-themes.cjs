// Real Edge regression. Uses an isolated browser profile and never the user's data.
// node qa-ui-themes.cjs: full matrix + workflows.
// --flows-only: reuse the unchanged matrix in report.json and repeat workflows.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const out='outputs/v112-ui';
const palettes=['sage','nordic','warm','snow','graphite','ocean','mist','sand'];
const flowOnly=process.argv.includes('--flows-only');
const report=flowOnly?JSON.parse(fs.readFileSync(`${out}/report.json`,'utf8')):{browser:'',checks:[],themes:[],screenshots:[],errors:[]};
report.complete=false;
const rgb=s=>(s.match(/[\d.]+/g)||[]).slice(0,3).map(Number);
const luminance=c=>rgb(c).map(v=>{v/=255;return v<=.04045?v/12.92:((v+.055)/1.055)**2.4;}).reduce((n,v,i)=>n+v*[.2126,.7152,.0722][i],0);
const contrast=(a,b)=>{const x=luminance(a),y=luminance(b);return (Math.max(x,y)+.05)/(Math.min(x,y)+.05);};
const check=name=>{report.checks.push(name);console.log('PASS',name);};
const settle=p=>p.waitForTimeout(480);
async function shot(p,name){const file=`${out}/${name}.png`;await settle(p);await p.screenshot({path:file,fullPage:true});report.screenshots.push(file);}
async function open(p,pane='study'){
 if(!await p.locator('#settings-dialog').evaluate(e=>e.open))await p.locator('#mobile-settings').click();
 await p.locator(`[data-settings-tab=${pane}]`).click();await settle(p);
}
async function close(p){await p.getByRole('button',{name:'关闭设置',exact:true}).click();await p.waitForFunction(()=>!document.querySelector('#settings-dialog').open);await settle(p);}
async function choose(p,id,value){
 const select=p.locator('#'+id);const label=await select.evaluate((e,value)=>[...e.options].find(o=>o.value===value)?.textContent,value);
 await p.locator('#'+id+' + .choice-trigger').click();
 await p.locator('#choice-menu').getByRole('option',{name:label,exact:true}).click();
 await p.waitForTimeout(260);assert.equal(await select.inputValue(),value);
}
async function route(p,id){await open(p,'data');await p.locator(`#panel-data [data-view=${id}]`).click();await p.waitForFunction(()=>!document.querySelector('#settings-dialog').open);await settle(p);}
async function overflow(p){assert.equal(await p.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);}

(async()=>{
 fs.mkdirSync(out,{recursive:true});const browser=await chromium.launch({channel:'msedge',headless:true});report.browser=browser.version();
 try{
 const context=await browser.newContext({viewport:{width:1440,height:900},acceptDownloads:true});
 // Audio/translation service availability is explicitly outside this local UI regression.
 await context.route('https://**/*',route=>route.abort());
 await context.addInitScript(()=>{
  if(!localStorage.getItem('cijian.cet6.v1'))localStorage.setItem('cijian.cet6.v1',JSON.stringify({version:1,mode:'follow',theme:'dark',settings:{keySoundVersion:2,sound:false,voice:false,auto:false,adaptive:false,order:'original',size:5},mastered:{},mistakes:{},days:{},logs:[]}));
 });
 const p=await context.newPage();p.on('pageerror',e=>report.errors.push(e.message));
 await p.goto('http://127.0.0.1:4176');await p.evaluate(()=>document.fonts.ready);await settle(p);
 // Follow: stable glyphs, immediate wrong/correct input, deletion, Tab and completion.
 const word=await p.evaluate(()=>current().word);
 await p.locator('#typing').pressSequentially(word[0]);
 await p.locator('#word span').first().evaluate(e=>e.dataset.qaStable='yes');
 await p.locator('#typing').pressSequentially('z');assert.equal(await p.locator('#word span').first().getAttribute('data-qa-stable'),'yes');
 assert.ok(await p.locator('#word .wrong').count());await p.locator('#typing').press('Backspace');
 assert.equal(await p.locator('#typing').inputValue(),word[0]);
 await p.locator('#typing').press('Tab');await settle(p);assert.equal(await p.locator('#word-details').evaluate(e=>e.open),true);
 await p.reload();await settle(p);assert.equal(await p.locator('#word-details').evaluate(e=>e.open),true);
 await p.locator('#typing').press('Tab');await settle(p);
 await p.locator('#typing').fill(word);assert.equal(await p.locator('#stage').evaluate(e=>e.classList.contains('success')),true);
 await p.locator('#typing').press('Space');assert.equal(await p.locator('#position').textContent(),'02');
 const second=await p.evaluate(()=>current().word);await p.locator('#typing').fill(second);await p.locator('#typing').press('Enter');assert.equal(await p.locator('#position').textContent(),'03');
 check('follow: stable letters, wrong input, immediate Backspace, Tab persistence, Space/Enter');
 // Real keyboard selection and all eight palette states; measure actual rendered colors.
 if(!flowOnly){
 await open(p,'feel');
 for(const palette of palettes){
  await choose(p,'palette-setting',palette);
  const state=await p.evaluate(()=>{
   const css=getComputedStyle(document.documentElement),read=selector=>{const e=document.querySelector(selector),s=getComputedStyle(e);return {color:s.color,bg:s.backgroundColor,outline:s.outlineColor,transition:s.transitionDuration};};
   return {palette:document.documentElement.dataset.palette,canvas:read('body').bg,accent:css.getPropertyValue('--accent').trim(),meta:document.querySelector('meta[name=theme-color]').content,text:read('.setting-row').color,secondary:read('.settings-tabs button:not([aria-selected=true])').color,surface:read('#settings-dialog').bg,raised:read('#palette-setting + .choice-trigger').bg,trigger:read('#palette-setting + .choice-trigger'),swatch:[...document.querySelectorAll('#palette-setting + .choice-trigger .palette-preview i')].map(e=>getComputedStyle(e).backgroundColor)};
  });
  assert.equal(state.palette,palette);assert.deepEqual(rgb(state.canvas),rgb(state.swatch[0]));
  assert.equal(state.meta,await p.evaluate(()=>getComputedStyle(document.documentElement).getPropertyValue('--canvas').trim()));
  for(const color of [state.text,state.secondary])assert.ok(contrast(color,state.surface)>=4.5,`${palette} setting contrast ${contrast(color,state.surface)}`);
  state.textContrast=contrast(state.text,state.surface);state.secondaryContrast=contrast(state.secondary,state.surface);
  // Hover, keyboard focus, pressed selection and disabled state are visible in the same theme.
  const trigger=p.locator('#palette-setting + .choice-trigger');await trigger.hover();
  state.hover=await trigger.evaluate(e=>getComputedStyle(e).backgroundColor);
  const bounds=await trigger.boundingBox();await p.mouse.move(bounds.x+10,bounds.y+10);await p.mouse.down();await p.waitForTimeout(180);state.active=await trigger.evaluate(e=>getComputedStyle(e).backgroundColor);await p.mouse.up();await p.keyboard.press('Escape');
  state.disabled=await p.locator('#key-sound-preview').evaluate(e=>({disabled:e.disabled,opacity:getComputedStyle(e).opacity}));assert.equal(state.disabled.disabled,true);assert.ok(Number(state.disabled.opacity)<=.5);
  await trigger.focus();state.focus=await trigger.evaluate(e=>({width:getComputedStyle(e).outlineWidth,color:getComputedStyle(e).outlineColor}));
  assert.notEqual(state.focus.width,'0px');
  await trigger.press('ArrowDown');await p.locator('#choice-menu').getByRole('option',{selected:true}).press('Escape');
  assert.equal(await trigger.getAttribute('aria-expanded'),'false');
  await close(p);
  await p.locator('#typing').fill('z');
  state.error=await p.locator('#word .wrong').first().evaluate(e=>({color:getComputedStyle(e).color,decoration:getComputedStyle(e).textDecorationLine}));
  assert.ok(state.error.decoration.includes('underline'));assert.ok(contrast(state.error.color,state.canvas)>=3);if(['sage','snow'].includes(palette))await shot(p,`error-${palette}-1440`);
  await p.locator('#typing').fill((await p.evaluate(()=>current().word))[0]);
  state.entered=await p.locator('#word .correct').first().evaluate(e=>getComputedStyle(e).color);
  assert.ok(contrast(state.entered,state.canvas)>=4.5);
  await overflow(p);report.themes.push(state);await shot(p,`theme-${palette}-1440`);await p.locator('#typing').fill('');await open(p,'feel');
 }
 check('all eight themes: colors, swatches, text contrast, hover/focus, keyboard dropdown, error underline, entered letters');
 // Representative dark/light screenshots cover every required viewport and main surface.
 for(const palette of ['sage','snow']){
  await choose(p,'palette-setting',palette);await close(p);
  for(const viewport of [{width:1440,height:900},{width:768,height:1024},{width:390,height:844}]){
   await p.setViewportSize(viewport);await overflow(p);await shot(p,`${palette}-follow-${viewport.width}`);
   await open(p,'feel');await overflow(p);await shot(p,`${palette}-settings-${viewport.width}`);
   await p.locator('#palette-setting + .choice-trigger').click();await shot(p,`${palette}-choices-${viewport.width}`);await p.keyboard.press('Escape');await close(p);
   await p.locator('[data-mode=dictation]').click();await settle(p);
   assert.equal(await p.locator('#study-explanation').evaluate(e=>getComputedStyle(e).visibility),'hidden');
   await p.locator('#typing').press('Tab');assert.equal(await p.locator('#word .masked').count(),0);
   await p.locator('#typing').fill(await p.evaluate(()=>current().word));await settle(p);assert.equal(await p.locator('#study-explanation').evaluate(e=>getComputedStyle(e).visibility),'visible');
   await shot(p,`${palette}-listening-${viewport.width}`);
   await p.locator('[data-mode=cloze]').click();await settle(p);await p.locator('#passage-input').press('Tab');await overflow(p);await shot(p,`${palette}-passage-${viewport.width}`);
   await p.locator('[data-mode=follow]').click();await settle(p);
  }
  await open(p,'feel');
 }
 check('1440/768/390: dark and light follow, settings, choices, listening and passage screenshots without root overflow');
 }else await open(p,'feel');
 await choose(p,'palette-setting','ocean');await close(p);await p.reload();await settle(p);assert.equal(await p.locator('html').getAttribute('data-palette'),'ocean');
 // Group selection and whole group completion.
 await p.setViewportSize({width:1440,height:900});await open(p,'study');await choose(p,'size-setting','5');await p.locator('#lesson-select').click();
 await p.locator('.group-option').nth(1).click();await close(p);
 assert.equal(await p.locator('#quick-group-no').textContent(),'02');
 for(let i=0;i<5;i++){await p.locator('#typing').fill(await p.evaluate(()=>current().word));await p.locator('#typing').press('Enter');}
 assert.equal(await p.locator('#continue').count(),1);await p.keyboard.press('Enter');await settle(p);assert.equal(await p.locator('#practice-view').isVisible(),true);
 check('theme refresh persistence, explicit group selection, five-word summary and Enter next group');
 // Course switching, library empty state and a long real dictionary entry.
 for(const course of ['basic','cet4','cet6']){await open(p,'study');await choose(p,'course-select',course);await close(p);assert.equal(await p.evaluate(()=>db.course),course);}
 await route(p,'library');await p.getByRole('textbox',{name:'搜索词库',exact:true}).fill('zzzzzzzzzz');assert.ok((await p.locator('#other-view').textContent()).includes('没有找到'));
 await p.getByRole('textbox',{name:'搜索词库',exact:true}).fill('mortgage');assert.ok(await p.locator('.list-row').count());await p.locator('.list-row button').first().click();await settle(p);
 await p.locator('#typing').press('Tab');await p.setViewportSize({width:390,height:844});await overflow(p);await shot(p,'long-definition-390');
 await route(p,'review');await shot(p,'review-390');await p.locator('#practice-return').click();await settle(p);
 check('three courses, library empty/search, long definitions at 390, review and return');
 // Article error recovery, continued input checkpoints and finish/next.
 await open(p,'study');await choose(p,'passage-source','custom');await p.locator('#passage-custom-text').fill('two words');await p.locator('#passage-use').click();assert.equal(await p.locator('#passage-custom-text').getAttribute('aria-invalid'),'true');
 await p.locator('#passage-custom-title').fill('QA local passage');await p.locator('#passage-custom-text').fill('Extraordinarily long words remain readable.');await p.locator('#passage-use').click();await p.waitForFunction(()=>!document.querySelector('#settings-dialog').open);await settle(p);
 const token=await p.evaluate(()=>passageState.tokens[passageState.index].target);await p.locator('#passage-input').fill(token);await p.locator('#passage-input').press('Space');
 const passageIndex=await p.evaluate(()=>passageState.index);await p.reload();await settle(p);assert.equal(await p.evaluate(()=>passageState.index),passageIndex);
 await p.locator('#passage-input').fill('z');assert.ok(await p.locator('.passage-active .wrong').count());await p.locator('#passage-input').press('Backspace');assert.equal(await p.locator('#passage-input').inputValue(),'');
 while(!await p.evaluate(()=>passageState.finished)){await p.locator('#passage-input').fill(await p.evaluate(()=>passageState.tokens[passageState.index].target));if(!await p.evaluate(()=>passageState.finished))await p.locator('#passage-input').press('Enter');}
 assert.equal(await p.locator('#passage-next').isVisible(),true);await p.locator('#passage-next').click();await settle(p);assert.equal(await p.evaluate(()=>passageState.finished),false);
 check('custom article validation/recovery, long word fit, Space/Enter, wrong/deletion, reload checkpoint, finish and next');
 // Records and actual JSON/CSV downloads; malformed import leaves records intact.
 await route(p,'stats');const count=await p.locator('.attempt-record').count();assert.ok(count>0);await shot(p,'records-390');
 await p.locator('.record-tools .choice-trigger').first().click();await p.keyboard.press('End');await p.keyboard.press('Enter');await settle(p);
 const csvPromise=p.waitForEvent('download');await p.getByRole('button',{name:'导出记录',exact:true}).click();await (await csvPromise).saveAs(path.join(out,'records.csv'));
 await open(p,'data');const jsonPromise=p.waitForEvent('download');await p.locator('#export').click();const backup=await jsonPromise;await backup.saveAs(path.join(out,'backup.json'));
 const prior=await p.evaluate(()=>({mastered:JSON.stringify(db.mastered),logs:db.logs.length}));
 await p.locator('#import').setInputFiles({name:'invalid.json',mimeType:'application/json',buffer:Buffer.from('{}')});assert.ok((await p.locator('#toast').textContent()).includes('格式不正确'));
 await p.locator('#import').setInputFiles(path.join(out,'backup.json'));await p.waitForFunction(()=>document.querySelector('#toast').textContent.includes('\u5df2\u5408\u5e76'));assert.ok((await p.locator('#toast').textContent()).includes('已合并'));
 assert.deepEqual(await p.evaluate(()=>({mastered:JSON.stringify(db.mastered),logs:db.logs.length})),prior);
 await close(p);check('records filter, CSV/JSON downloads, invalid import recovery and idempotent backup merge');
 // Management surfaces and error states in two themes, plus automatic advance.
 for(const palette of ['sage','snow']){
  await open(p,'feel');await choose(p,'palette-setting',palette);await close(p);
  for(const destination of ['library','review','stats']){await route(p,destination);await overflow(p);await shot(p,`${palette}-${destination}-390`);}
 }
 await p.locator('#practice-return').click();await p.locator('[data-mode=follow]').click();await open(p,'study');await p.locator('#auto-setting').check();await choose(p,'speed-setting','400');await close(p);
 const beforeWord=await p.evaluate(()=>current().word);await p.locator('#typing').fill(beforeWord);await p.waitForFunction(w=>current()?.word!==w,beforeWord);await p.locator('#typing').pressSequentially('a');assert.equal(await p.locator('#typing').inputValue(),'a');await p.locator('#typing').fill('');
 await open(p,'feel');await choose(p,'motion-setting','none');await close(p);await p.locator('#typing').pressSequentially('a');assert.equal(await p.locator('#word').evaluate(e=>e.getAnimations({subtree:true}).filter(a=>a.playState==='running').length),0);
 check('two-theme library/review/records, 400ms auto advance, continuous input and no-motion setting');
 // Real reduced-motion emulation, including palette, preview and typing.
 await p.emulateMedia({reducedMotion:'reduce'});if(await p.locator('#practice-return').isVisible())await p.locator('#practice-return').click();await p.locator('[data-mode=follow]').click();await open(p,'feel');
 await choose(p,'palette-setting','sand');await p.locator('#motion-preview').click();await p.waitForTimeout(120);
 assert.equal(await p.evaluate(()=>document.querySelector('#settings-dialog').getAnimations({subtree:true}).filter(a=>a.playState==='running').length),0);
 assert.equal(await p.locator('#palette-setting + .choice-trigger').evaluate(e=>getComputedStyle(e).transitionDuration),'0s');
 await close(p);await p.locator('#typing').fill('a');assert.equal(await p.locator('#live-caret').evaluate(e=>getComputedStyle(e).animationName),'none');await shot(p,'reduced-motion-390');
 assert.deepEqual(report.errors,[]);check('OS reduced motion: no running sheet/preview transitions, no caret animation, responsive input, no JS errors');
 report.complete=true;report.checks=[...new Set(report.checks)];report.screenshots=[...new Set(report.screenshots)];
 await context.close();
 }finally{await browser.close();fs.writeFileSync(`${out}/report.json`,JSON.stringify(report,null,2));}
})().catch(e=>{console.error(e);process.exitCode=1;});
