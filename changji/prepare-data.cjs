const fs=require('fs');const old=JSON.parse(fs.readFileSync('dist/words.js','utf8').replace('window.WORDS=','').replace(/;$/,''));const map=new Map();
const get=w=>{if(!map.has(w))map.set(w,{word:w,phonetic:'',senses:new Map(),definitions:new Set(),examples:[],phrases:[]});return map.get(w)};
const valid=w=>/^[a-z]+(?:[-'][a-z]+)*$/.test(w);
for(const name of ['CET6_2.json','CET6_3.json'])for(const line of fs.readFileSync('dictionary/'+name,'utf8').trim().split(/\r?\n/)){
 const r=JSON.parse(line),w=r.headWord.toLowerCase();if(!valid(w))continue;const o=get(w),c=r.content.word.content;o.phonetic||=c.ukphone||c.usphone||'';
 for(const t of c.trans||[]){const pos=t.pos||'';const arr=o.senses.get(pos)||[];o.senses.set(pos,[...new Set([...arr,...t.tranCn.split(/[；;，,]/).map(s=>s.trim()).filter(Boolean)])]);if(t.tranOther)o.definitions.add(t.tranOther);}
 for(const s of c.sentence?.sentences||[])if(!o.examples.some(e=>e.en===s.sContent))o.examples.push({en:s.sContent,zh:s.sCn});
 for(const p of c.phrase?.phrases||[])if(!o.phrases.some(e=>e.en===p.pContent))o.phrases.push({en:p.pContent,zh:p.pCn});
}
for(const r of JSON.parse(fs.readFileSync('vocab-source.json','utf8'))){const w=r.word.toLowerCase();if(!valid(w))continue;const o=get(w);for(const t of r.translations){const pos=t.type||'';o.senses.set(pos,[...new Set([...(o.senses.get(pos)||[]),...t.translation.split(/[；;，,]/).map(s=>s.trim()).filter(Boolean)])]);}for(const p of r.phrases||[])if(!o.phrases.some(e=>e.en===p.phrase))o.phrases.push({en:p.phrase,zh:p.translation});}
const order=new Map(old.map((w,i)=>[w.word,i]));
const words=[...map.values()].map(o=>{const rx=new RegExp('\\b'+o.word+'\\b','i');const ex=o.examples.find(s=>rx.test(s.en)&&s.en.trim().split(/\s+/).length>=5&&/[.!?]["”']?$/.test(s.en.trim()));const meanings=[...o.senses].map(([pos,senses])=>({pos,text:senses.join('；')}));return {word:o.word,phonetic:o.phonetic,zh:meanings.map(m=>(m.pos?m.pos+'. ':'')+m.text).join('\n'),meanings,en:[...o.definitions].join(' / '),definitions:[...o.definitions],sentence:ex?.en||'',translation:ex?.zh||'',examples:o.examples.slice(0,4),phrases:o.phrases.slice(0,5),phrase:o.phrases.length?{pContent:o.phrases[0].en,pCn:o.phrases[0].zh}:null};}).sort((a,b)=>(order.get(a.word)??100000)-(order.get(b.word)??100000));
fs.writeFileSync('dist/words.js','window.WORDS='+JSON.stringify(words)+';');
console.log(JSON.stringify({total:words.length,english:words.filter(w=>w.en).length,cloze:words.filter(w=>w.sentence).length}));
