/* Translations load only for the sentence opened with Tab, never the whole article. */
var passageDetailRequest=0,passageDetailAnimations=[],passageTranslationPending=new Map();
var passageWordDictionary=null,passageSentenceDictionary=null;
function buildPassageDictionary(){
 if(passageWordDictionary)return;
 passageWordDictionary=new Map();passageSentenceDictionary=new Map();
 const normalize=t=>t.replace(/\s+/g,' ').trim().toLowerCase();
 for(const w of allWords){
  if(!passageWordDictionary.has(w.word))passageWordDictionary.set(w.word,w);
  for(const example of w.examples||[])if(example.en&&example.zh)passageSentenceDictionary.set(normalize(example.en),example.zh);
  if(w.sentence&&w.translation)passageSentenceDictionary.set(normalize(w.sentence),w.translation);
 }
}
function passageCurrentSentence(state){
 if(!state.segments){
  const parts=typeof Intl.Segmenter==='function'?[...new Intl.Segmenter('en',{granularity:'sentence'}).segment(state.material.text)].map(p=>p.segment):state.material.text.match(/[^.!?]+[.!?]*(?:\s+|$)/g)||[state.material.text];
  let count=0;state.segments=[];
  for(const text of parts){const length=passageTokens(text).length;if(length){state.segments.push({text:text.trim(),start:count,end:count+length});count+=length;}}
 }
 return state.segments.find(p=>state.index>=p.start&&state.index<p.end)?.text||state.material.text;
}
function decodePassageTranslation(text){
 return text.replace(/&(?:amp|quot|apos|lt|gt);|&#(?:x[0-9a-f]+|\d+);/gi,entity=>{
  const named={'&amp;':'&','&quot;':'"','&apos;':"'",'&lt;':'<','&gt;':'>'};if(named[entity])return named[entity];
  const n=entity[2].toLowerCase()==='x'?parseInt(entity.slice(3,-1),16):Number(entity.slice(2,-1));
  return n>0&&n<=0x10ffff?String.fromCodePoint(n):entity;
 });
}
function translationParts(text){
 const encoder=new TextEncoder(),parts=[];let part='';
 for(const word of text.split(/\s+/)){if(encoder.encode(word).length>480)throw Error('句中存在过长字符，请检查文章中的空格。');if(encoder.encode(part+' '+word).length>480){parts.push(part);part=word;}else part+=(part?' ':'')+word;}
 if(part)parts.push(part);if(parts.length>12)throw Error('当前句子过长，请在文章中补充句号后重试。');return parts;
}
async function translatePassageText(text,retry=false){
 const key=text.replace(/\s+/g,' ').trim(),cache=db.passageTranslations||{};
 if(!retry&&typeof cache[key]==='string')return{translation:cache[key],provider:true};
 if(passageTranslationPending.has(key))return passageTranslationPending.get(key);
 const request=(async()=>{
  const translated=[];
  for(const part of translationParts(key)){
   const abort=new AbortController(),timer=setTimeout(()=>abort.abort(),10000);
   try{
    const url='https://api.mymemory.translated.net/get?q='+encodeURIComponent(part)+'&langpair=en%7Czh-CN';
    const response=await fetch(url,{signal:abort.signal,credentials:'omit',referrerPolicy:'no-referrer'});
    if(!response.ok)throw Error('翻译暂时无法连接，请重试。');const data=await response.json();
    if(data.quotaFinished||Number(data.responseStatus)===403||Number(data.responseStatus)===429)throw Error('今日翻译额度已用完，可稍后重试。');
    if(Number(data.responseStatus)!==200||typeof data.responseData?.translatedText!=='string')throw Error('暂未获得译文，请重试。');
    const value=decodePassageTranslation(data.responseData.translatedText).trim();
    if(!value||/MYMEMORY WARNING|QUERY LENGTH LIMIT|INVALID LANGUAGE/i.test(value))throw Error('暂未获得译文，请重试。');
    translated.push(value);
   }catch(error){if(error.name==='AbortError')throw Error('翻译连接超时，请重试。');if(['TypeError','SyntaxError'].includes(error.name))throw Error('翻译连接失败，请检查网络后重试。');throw error;}finally{clearTimeout(timer);}
  }
  const translation=translated.join(' ');db.passageTranslations||={};db.passageTranslations[key]=translation;
  const keys=Object.keys(db.passageTranslations);for(const k of keys.slice(0,Math.max(0,keys.length-250)))delete db.passageTranslations[k];
  setTimeout(persist,0);return{translation,provider:true};
 })();
 passageTranslationPending.set(key,request);try{return await request;}finally{passageTranslationPending.delete(key);}
}
async function updatePassageDetails(retry=false){
 const request=++passageDetailRequest,panel=$('#passage-details'),state=passageState;
 if(!state||panel.hidden||!db.settings.passageDetailsOpen||mode!=='cloze')return;
 buildPassageDictionary();const word=state.tokens[state.index].target,sentence=passageCurrentSentence(state);
 const candidates=[word,word.replace(/'s$/,''),word.replace(/ies$/,'y'),word.length>3?word.replace(/s$/,''):word];
 const entry=candidates.map(w=>passageWordDictionary.get(w)).find(Boolean);const localWord=entry?((entry.meanings||[]).filter(m=>m.text).slice(0,2).map(m=>(m.pos?m.pos+'. ':'')+m.text.split(/[；;]/).slice(0,3).join('；')).join('；')||entry.zh||''):'';
 const localSentence=passageSentenceDictionary.get(sentence.replace(/\s+/g,' ').trim().toLowerCase());const cached=db.passageTranslations?.[sentence.replace(/\s+/g,' ').trim()];const knownSentence=localSentence||(typeof cached==='string'?cached:'');
 $('#passage-detail-word').textContent=word;
 $('#passage-detail-meaning').textContent=localWord||'查询中…';
 $('#passage-detail-source').textContent=state.material.title+(state.material.source==='自定义文章'?' · 自定义文章':'');
 const sentenceElement=$('#passage-sentence-translation'),initial=knownSentence||'翻译中…';if(sentenceElement.textContent!==initial)sentenceElement.textContent=initial;sentenceElement.dataset.loading=String(!knownSentence||retry);
 $('#passage-translation-provider').hidden=!!localSentence;$('#passage-translation-retry').hidden=true;panel.setAttribute('aria-busy','true');
 const valid=()=>request===passageDetailRequest&&passageState===state&&!panel.hidden&&mode==='cloze';
 const sentenceTask=knownSentence&&!retry?Promise.resolve({translation:knownSentence,provider:!localSentence}):translatePassageText(sentence,retry);
 const wordTask=localWord?Promise.resolve({translation:localWord,provider:false}):translatePassageText(word,retry);
 const [sentenceResult,wordResult]=await Promise.allSettled([sentenceTask,wordTask]);if(!valid())return;
 $('#passage-detail-meaning').textContent=wordResult.status==='fulfilled'?wordResult.value.translation:'暂无词义';
 if(sentenceResult.status==='fulfilled'){if(sentenceElement.textContent!==sentenceResult.value.translation)sentenceElement.textContent=sentenceResult.value.translation;$('#passage-translation-provider').hidden=!sentenceResult.value.provider;}
 else{$('#passage-sentence-translation').textContent=sentenceResult.reason?.message||'翻译暂不可用，请重试。';$('#passage-translation-retry').hidden=false;}
 if(wordResult.status==='rejected')$('#passage-translation-retry').hidden=false;
 sentenceElement.dataset.loading='false';panel.setAttribute('aria-busy','false');
}
function togglePassageDetails(){
 const panel=$('#passage-details');for(const animation of passageDetailAnimations)animation.cancel();passageDetailAnimations=[];
 const open=!db.settings.passageDetailsOpen;db.settings.passageDetailsOpen=open;persist();
 if(open){panel.hidden=false;updatePassageDetails();if(effectsAllowed())passageDetailAnimations.push(panel.animate([{opacity:.1,transform:'translateY(8px)'},{opacity:1,transform:'none'}],{duration:300,easing:'cubic-bezier(.2,.8,.2,1)'}));}
 else{passageDetailRequest++;if(effectsAllowed()){const animation=panel.animate([{opacity:1,transform:'none'},{opacity:0,transform:'translateY(4px)'}],{duration:160,easing:'ease-out'});passageDetailAnimations.push(animation);animation.finished.then(()=>{if(!db.settings.passageDetailsOpen)panel.hidden=true;},()=>{});}else panel.hidden=true;}
}
