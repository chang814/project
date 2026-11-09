/* One moving reading line. Imported articles and checkpoints stay in this browser. */
var passageCatalogCourse=null;
var passageState=null,passageSetup=false,passageMotion=null,passageCatalog=null,passageSizeObserver=null,passageObservedWord=null;
function passageTokens(text){return text.replace(/[–—]/g,' ').split(/\s+/).filter(Boolean).map(raw=>({raw,target:raw.toLowerCase().replace(/[‘’]/g,"'").replace(/^[^a-z0-9]+|[^a-z0-9]+$/g,'')})).filter(t=>t.target);}
function passageMaterials(){if(passageCatalog&&passageCatalogCourse===db.course)return passageCatalog;passageCatalogCourse=db.course;const seen=new Set();passageCatalog=[];for(const item of window.RECENT_EXAM_EXCERPTS||[]){if(item.level!==(db.course==='cet4'?'CET4':'CET6')||db.course==='basic')continue;seen.add(item.en);passageCatalog.push({text:item.en,title:formatExamSource({...item,verification:'transcription'}),source:'网络转录，未核验官方原卷',year:item.year});}for(const w of (db.course==='cet4'?WORDS4:db.course==='basic'?WORDS_BASIC:WORDS))for(const item of w.examExamples||[]){const text=item.en?.trim();if(!text||seen.has(text)||passageTokens(text).length<6)continue;seen.add(text);passageCatalog.push({title:formatExamSource(item.source),source:item.source.verification==='transcription'?'网络转录，未核验官方原卷':(item.source.level==='CET4'?'四级':'六级')+'词库真题节选',text,year:item.source.year});}return passageCatalog.sort((a,b)=>((s)=>{const [y,m]=String(s||'0').split('.').map(Number);return y*12+(m||0);})(b.year)-((s)=>{const [y,m]=String(s||'0').split('.').map(Number);return y*12+(m||0);})(a.year));}
function selectedPassage(){if(db.settings.passageSource==='custom'&&db.passageCustom?.text)return db.passageCustom;const list=passageMaterials();return list[(db.settings.passageExamIndex||0)%Math.max(1,list.length)]||{title:'练习短文',source:'原创练习材料',text:'Learning a language takes time and attention. Small steps become meaningful when we return to them every day. Read carefully, listen closely, and leave enough room to think. A useful habit is built through patient practice rather than a single perfect attempt.'};}
function setupPassage(){if(passageSetup)return;passageSetup=true;$('#passage-translation-retry').onclick=()=>updatePassageDetails(true);if('ResizeObserver'in window){passageSizeObserver=new ResizeObserver(()=>{const recenter=()=>{if(mode!=='cloze'||!passageState)return;const active=$('.passage-active'),line=$('#passage-line'),rail=$('#passage-rail');if(active)rail.style.transform='translateX('+(line.clientWidth/2-active.offsetLeft-active.offsetWidth/2)+'px)';};if(passageMotion?.playState==='running')passageMotion.finished.then(recenter,()=>{});else recenter();});passageSizeObserver.observe($('#passage-rail'));}const input=$('#passage-input');input.addEventListener('input',passageInput);input.addEventListener('compositionend',passageInput);input.addEventListener('paste',e=>e.preventDefault());input.addEventListener('keydown',e=>{if(e.isComposing||e.ctrlKey||e.metaKey||e.altKey)return;if(e.code==='Space'||e.key==='Enter'){e.preventDefault();if(e.repeat)return;if(passageState?.finished){$('#passage-next').click();return;}advancePassage();}if(e.key==='Escape'){e.preventDefault();input.blur();}});$('#passage-line').onclick=()=>input.focus({preventScroll:true});$('#passage-next').onclick=nextPassage;$('#passage-material').onclick=()=>{settingsPane='study';openSettings();$('#passage-settings').scrollIntoView({block:'nearest'});};$('#passage-source').value=db.settings.passageSource||'exam';$('#passage-custom-title').value=db.passageCustom?.title||'';$('#passage-custom-text').value=db.passageCustom?.text||'';$('#passage-import').hidden=$('#passage-source').value!=='custom';$('#passage-source').onchange=e=>{db.settings.passageSource=e.target.value;$('#passage-import').hidden=e.target.value!=='custom';persist();if(mode==='cloze'&&(e.target.value!=='custom'||db.passageCustom?.text))startPassage(false);};$('#passage-use').onclick=()=>{const text=$('#passage-custom-text').value.trim(),tokens=passageTokens(text),error=$('#passage-error');if(tokens.length<3){error.textContent='请粘贴至少包含 3 个英文单词的文章。';$('#passage-custom-text').setAttribute('aria-invalid','true');return;}error.textContent='';$('#passage-custom-text').removeAttribute('aria-invalid');db.passageCustom={title:$('#passage-custom-title').value.trim()||'自定义文章',source:'自定义文章',text};db.settings.passageSource='custom';delete db.passageResume;persist();mode='cloze';closeSettings(()=>startSession());};window.addEventListener('resize',()=>{if(mode==='cloze'&&passageState)renderPassageLine(false);});}
var passageChangeMotion=null;
async function nextPassage(){
 if(passageChangeMotion)return;
 const stage=$('#passage-stage'),state=passageState,button=$('#passage-next');
 const moving=effectsAllowed();
 button.disabled=true;
 const exit=moving?stage.animate([{opacity:1,transform:'translateY(0)'},{opacity:0,transform:'translateY(-7px)'}],{duration:150,easing:'ease-in',fill:'forwards'}):null;
 passageChangeMotion=exit||true;
 try{
  if(exit)await exit.finished;
  if(mode!=='cloze'||view!=='practice'||passageState!==state)return;
  if(db.settings.passageSource!=='custom')db.settings.passageExamIndex=(db.settings.passageExamIndex||0)+1;
  delete db.passageResume;persist();startPassage(false);
  const entrance=moving?stage.animate([{opacity:0,transform:'translateY(9px)'},{opacity:1,transform:'translateY(0)'}],{duration:320,easing:'cubic-bezier(.16,1,.3,1)'}):null;
  exit?.cancel();passageChangeMotion=entrance||true;
  if(entrance)await entrance.finished;
 }catch{}finally{exit?.cancel();passageChangeMotion=null;button.disabled=false;}
}
function startPassage(resume=true){document.body.dataset.practiceMode='cloze';setupPassage();passageMotion?.cancel();passageMotion=null;if(passageObservedWord)passageSizeObserver?.unobserve(passageObservedWord);passageObservedWord=null;$('#passage-rail').replaceChildren();clearAdvance();solved=false;paused=false;queue=[];index=0;reviewing=false;db.mode=mode;const material=selectedPassage(),tokens=passageTokens(material.text),saved=db.passageResume;const restore=resume&&saved?.text===material.text&&saved.index<tokens.length;passageState={material,tokens,index:restore?saved.index:0,keys:restore?saved.keys:0,correct:restore?saved.correct:0,errors:restore?saved.errors:0,elapsed:restore?saved.elapsed:0,last:0,finished:false};$('#practice-view').classList.add('passage-mode');$('#passage-stage').hidden=false;$('#passage-material').hidden=false;$('#passage-material').textContent=db.settings.passageSource==='custom'?'自定义文章':'示例短文';$('#passage-title').textContent=material.title;$('#passage-title').title=material.source;$('#passage-title').hidden=true;$('#passage-details').hidden=!db.settings.passageDetailsOpen;$('#passage-result').hidden=true;$('#passage-next').hidden=true;$('#passage-input').disabled=false;$('#passage-input').value='';$$('[data-mode]').forEach(b=>{b.classList.toggle('selected',b.dataset.mode===mode);b.setAttribute('aria-pressed',b.dataset.mode===mode);});$('.modes').style.setProperty('--mode-index',2);setView('practice');renderPassageLine(false);persist();requestAnimationFrame(()=>{if(mode!=='cloze')return;renderPassageLine(false);$('#passage-input').focus({preventScroll:true});const state=passageState;document.fonts.ready.then(()=>{if(mode==='cloze'&&passageState===state&&!state.finished)renderPassageLine(false);});});}
function renderPassageLine(animate=true){
 const s=passageState;if(!s)return;
 const input=$('#passage-input'),rail=$('#passage-rail'),line=$('#passage-line');
 const preserved=s.renderedIndex===s.index?input.value:'';
 const nextNode=rail.querySelector('[data-index="'+s.index+'"]');
 const before=nextNode?.getBoundingClientRect();
 const lineBounds=line.getBoundingClientRect();
 s.renderedIndex=s.index;
 passageMotion?.cancel();
 passageMotion=null;

 const first=Math.max(0,s.index-12),last=Math.min(s.tokens.length,s.index+13);
 const nodes=new Map([...rail.children].map(el=>[Number(el.dataset.index),el]));
 for(const [i,node] of nodes){
  if(i<first||i>=last){node.remove();nodes.delete(i);}
 }
 for(let i=first;i<last;i++){
  let word=nodes.get(i);
  if(!word){
   word=document.createElement('span');
   word.className='passage-word';
   word.dataset.index=i;
   word.textContent=s.tokens[i].raw;
   const following=[...rail.children].find(el=>Number(el.dataset.index)>i);
   rail.insertBefore(word,following||null);
  }
  const distance=Math.abs(i-s.index);
  const currentWord=distance===0;
  if(!currentWord&&word.classList.contains('passage-active'))word.textContent=s.tokens[i].raw;
  word.classList.toggle('passage-active',currentWord);
  word.classList.toggle('passage-done',i<s.index);
  word.style.opacity=String(Math.max(.035,.76**distance));
  word.style.filter=distance?'blur('+Math.min(4,(distance-1)*.65+.35)+'px)':'none';
  if(currentWord){
   word.setAttribute('aria-current','true');
   word.removeAttribute('aria-hidden');
  }else{
   word.removeAttribute('aria-current');
   word.setAttribute('aria-hidden','true');
  }
 }

 const active=rail.querySelector('.passage-active');
 if(!active)return;
 if(passageObservedWord!==active){
  if(passageObservedWord)passageSizeObserver?.unobserve(passageObservedWord);
  passageObservedWord=active;
  passageSizeObserver?.observe(active);
 }

 // Fit the whole line before moving it so zoom and long words don't pop on arrival.
 const fontSize=Math.min(parseFloat(getComputedStyle(rail).fontSize)*wordZoom(),line.clientHeight*.65);
 const words=[...rail.children];
 for(const word of words)word.style.fontSize=fontSize+'px';
 input.value=preserved;
 input.maxLength=s.tokens[s.index].target.length+4;
 drawPassageLetters();
 const maxWidth=line.clientWidth*.62;
 const widths=words.map(word=>word.offsetWidth);
 words.forEach((word,i)=>{
  if(widths[i]>maxWidth&&maxWidth>0)word.style.fontSize=Math.max(18,fontSize*maxWidth/widths[i])+'px';
 });
 if(active.offsetWidth!==widths[words.indexOf(active)])drawPassageLetters();
 active.classList.toggle('passage-compact',parseFloat(active.style.fontSize)<24);

 const center=active.offsetLeft+active.offsetWidth/2;
 const target=line.clientWidth/2-center;
 const old=before?before.left+before.width/2-lineBounds.left-center:target;
 rail.style.transform='translateX('+target+'px)';
 if(animate&&before&&effectsAllowed()){
  passageMotion=rail.animate([{transform:'translateX('+old+'px)'},{transform:'translateX('+target+'px)'}],{duration:360,easing:'cubic-bezier(.2,.72,.28,1)'});
 }
 $('#passage-position').textContent=(s.index+1)+' / '+s.tokens.length;
 setSessionProgress(s.index/s.tokens.length);
 updatePassageDetails();
}
function drawPassageLetters(){
 const s=passageState,active=$('.passage-active');if(!s||!active)return;
 const typed=$('#passage-input').value.toLowerCase().replace(/[‘’]/g,"'"),target=s.tokens[s.index].target;
 if(active.dataset.target!==target||!active.querySelector('span')){active.dataset.target=target;active.replaceChildren(...[...target].map(c=>{const span=document.createElement('span');span.textContent=c;return span;}));}
 let caret=active.querySelector('.passage-caret');if(!caret){caret=document.createElement('i');caret.className='passage-caret';caret.setAttribute('aria-hidden','true');active.append(caret);}
 const letters=[...active.children].filter(el=>el.tagName==='SPAN'),length=Math.max(target.length,typed.length);
 while(letters.length<length){const span=document.createElement('span');active.insertBefore(span,caret);letters.push(span);}
 while(letters.length>length)letters.pop().remove();
 for(let i=0;i<letters.length;i++){const letter=letters[i],value=typed[i],nextClass=value?(value===target[i]?'correct':'wrong'):'';const changed=value&&(letter.textContent!==value||letter.className!==nextClass);if(!value&&(letter.classList.contains('correct')||letter.classList.contains('wrong'))){exitGlyph(letter,true);letter.getAnimations().forEach(a=>a.cancel());}letter.textContent=value||target[i]||'';letter.className=nextClass;if(changed)animateLetter(letter);}
 $('#passage-line').classList.toggle('has-error',[...typed].some((c,i)=>c!==target[i]));
 const letter=letters[Math.min(typed.length,letters.length-1)];caret.style.left=(letter?letter.offsetLeft+(typed.length>=target.length?letter.offsetWidth:0):0)+'px';
}

function passageInput(e){if(e.isComposing||!passageState||passageState.finished)return;const s=passageState,input=$('#passage-input'),value=input.value.toLowerCase().replace(/[‘’]/g,"'"),target=s.tokens[s.index].target;const old=s.answer||'';if(value===old)return;input.value=value;s.answer=value;const now=Date.now();if(s.last)s.elapsed+=Math.min(10000,now-s.last);s.last=now;for(let i=0;i<value.length;i++)if(value[i]!==old[i]){s.keys++;if(value[i]===target[i])s.correct++;else s.errors++;}keySound([...value].some((c,i)=>c!==target[i]));drawPassageLetters();if(value===target&&s.index===s.tokens.length-1)finishPassage();}
function advancePassage(){const s=passageState;if(!s||s.finished)return;if($('#passage-input').value.toLowerCase().replace(/[‘’]/g,"'")!==s.tokens[s.index].target){keySound(true);return;}s.index++;s.answer='';s.last=0;db.passageResume={...s.material,index:s.index,keys:s.keys,correct:s.correct,errors:s.errors,elapsed:s.elapsed};persist();renderPassageLine(true);}
function finishPassage(){const s=passageState;if(s.finished)return;s.finished=true;setSessionProgress(1,true);$('#passage-input').disabled=true;$('.passage-caret')?.remove();const rate=s.keys?Math.round(s.correct/s.keys*100):100;$('#passage-result').textContent=s.tokens.length+' 词 · '+rate+'% 准确率';$('#passage-result').hidden=false;$('#passage-next').hidden=false;$('#passage-next span').textContent=db.settings.passageSource==='custom'?'再练一遍':'下一句';$('#passage-next').setAttribute('aria-label',$('#passage-next span').textContent);$('#passage-next').title=$('#passage-next span').textContent+'（空格或回车）';db.passageLogs||=[];db.passageLogs.unshift({id:crypto.randomUUID(),title:s.material.title,source:s.material.source,t:Date.now(),words:s.tokens.length,accuracy:rate,ms:s.elapsed});db.passageLogs=db.passageLogs.slice(0,100);delete db.passageResume;persist();$('#passage-next').focus({preventScroll:true});}
function renderPassageHistory(){if(!db.passageLogs?.length)return;const host=$('#other-view'),section=document.createElement('section');section.className='passage-history';const heading=document.createElement('h2');heading.textContent='长句输入';section.append(heading);for(const record of db.passageLogs.slice(0,10)){const row=document.createElement('p');row.textContent=record.title+' · '+record.words+' 词 · '+record.accuracy+'% · '+new Date(record.t).toLocaleDateString('zh-CN');section.append(row);}host.append(section);}

function mergePassageData(data){if(data.passageCustom&&typeof data.passageCustom.text==='string'&&data.passageCustom.text.length<=20000&&passageTokens(data.passageCustom.text).length>=3&&!db.passageCustom)db.passageCustom={title:String(data.passageCustom.title||'自定义文章').slice(0,100),source:'自定义文章',text:data.passageCustom.text};if(!Array.isArray(data.passageLogs))return;const records=new Map((db.passageLogs||[]).map(r=>[r.id,r]));for(const r of data.passageLogs){if(typeof r.id!=='string'||!Number.isFinite(r.t)||!Number.isFinite(r.words)||r.words<1||!Number.isFinite(r.accuracy)||r.accuracy<0||r.accuracy>100)continue;records.set(r.id,{id:r.id,title:String(r.title||'长句输入').slice(0,100),source:String(r.source||'').slice(0,100),t:r.t,words:r.words,accuracy:r.accuracy,ms:Math.max(0,Number(r.ms)||0)});}db.passageLogs=[...records.values()].sort((a,b)=>b.t-a.t).slice(0,100);}

