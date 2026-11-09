const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const calls=[];
const context={db:{},TextEncoder,AbortController,setTimeout,clearTimeout,persist(){},fetch:async url=>{calls.push(new URL(url));return{ok:true,json:async()=>({responseStatus:200,responseData:{translatedText:'测试译文 &amp; 缓存'}})};}};
vm.createContext(context);
vm.runInContext(fs.readFileSync('dist/passage-details.js','utf8'),context);
(async()=>{
 const sentence=Array(50).fill('Today’s careful practice and café notes').join(' ');
 const result=await context.translatePassageText(sentence);
 assert(result.translation.includes('测试译文 & 缓存'));
 assert(calls.length>1);
 for(const url of calls){assert.equal(url.pathname,'/get');assert(Buffer.byteLength(url.searchParams.get('q'),'utf8')<=500);assert.equal(url.searchParams.get('langpair'),'en|zh-CN');}
 const count=calls.length;await context.translatePassageText(sentence);assert.equal(calls.length,count,'cached text must not be sent again');
 context.fetch=async()=>({ok:true,json:async()=>({quotaFinished:true,responseStatus:403})});
 await assert.rejects(context.translatePassageText('Quota check.'),/额度/);
 context.fetch=async()=>{throw new TypeError('network failed')};
 await assert.rejects(context.translatePassageText('Network check.'),/连接失败/);
 assert.equal(context.passageTranslationPending.size,0,'failed requests must not remain pending');
 console.log('Translation checks passed: UTF-8 request bounds, cache, entity decoding, quota and network recovery.');
})().catch(error=>{console.error(error);process.exitCode=1;});
