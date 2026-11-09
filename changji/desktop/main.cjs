'use strict';
const {app,BrowserWindow,protocol,session,shell}=require('electron');
const fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto');
const root=path.join(__dirname,'..','dist');
app.setName('Changji');
if(process.argv.includes('--qa'))app.setPath('userData',path.join(app.getPath('temp'),'changji-v1-desktop-qa'));
protocol.registerSchemesAsPrivileged([{scheme:'changji',privileges:{standard:true,secure:true,supportFetchAPI:true,corsEnabled:true,stream:true}}]);
const allowedExternal=new Set(['dict.youdao.com','github.com','mymemory.translated.net','cijian-cet6-mc.jonlycc.chatgpt.site']);
function external(raw){try{const url=new URL(raw);if(url.protocol==='https:'&&allowedExternal.has(url.hostname))shell.openExternal(url.href).catch(()=>{});}catch{}}
let window;
async function createWindow(){
 window=new BrowserWindow({width:1280,height:850,minWidth:390,minHeight:600,title:'畅记 v'+app.getVersion().split('.').slice(0,2).join('.'),backgroundColor:'#111413',icon:path.join(__dirname,'..','build','icon.png'),autoHideMenuBar:true,show:false,webPreferences:{nodeIntegration:false,contextIsolation:true,sandbox:true,webSecurity:true,spellcheck:false}});
 window.removeMenu();
 window.webContents.setWindowOpenHandler(({url})=>{external(url);return{action:'deny'};});
 window.webContents.on('will-navigate',(event,url)=>{if(new URL(url).protocol!=='changji:'||new URL(url).hostname!=='app'){event.preventDefault();external(url);}});
 window.once('ready-to-show',()=>window.show());
 await window.loadURL('changji://app/');
}
if(!app.requestSingleInstanceLock()){app.quit();}else{
 app.on('second-instance',()=>{if(window){if(window.isMinimized())window.restore();window.focus();}});
 app.whenReady().then(async()=>{
  session.defaultSession.setPermissionRequestHandler((_contents,_permission,callback)=>callback(false));
  session.defaultSession.setPermissionCheckHandler(()=>false);
  const html=await fs.readFile(path.join(root,'index.html'),'utf8');
  const hashes=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m=>"'sha256-"+crypto.createHash('sha256').update(m[1]).digest('base64')+"'").join(' ');
  const csp="default-src 'self'; script-src 'self' "+hashes+"; style-src 'self' 'unsafe-inline'; font-src 'self'; img-src 'self' data:; media-src 'self' https://dict.youdao.com; connect-src 'self' https://api.mymemory.translated.net; object-src 'none'; base-uri 'none'; frame-src 'none'";
  protocol.handle('changji',async request=>{
   try{
    const url=new URL(request.url);if(url.hostname!=='app'||!['GET','HEAD'].includes(request.method))return new Response('Forbidden',{status:403});
    const relative=decodeURIComponent(url.pathname==='/'?'/index.html':url.pathname);
    const file=path.resolve(root,'.'+relative);
    if(!file.startsWith(root+path.sep)||relative.includes('\\'))return new Response('Forbidden',{status:403});
    const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.ttf':'font/ttf','.txt':'text/plain; charset=utf-8','.zip':'application/zip'}[path.extname(file)]||'application/octet-stream';
    let bytes=await fs.readFile(file);if(path.extname(file)==='.html')bytes=Buffer.from(bytes.toString('utf8').replace('v1.1 网页预览','v1.1').replace('网页先行，桌面版仍为 v1.0。','本版已通过网页检查并同步桌面。'));return new Response(request.method==='HEAD'?null:bytes,{headers:{'Content-Type':mime,'Content-Security-Policy':csp,'X-Content-Type-Options':'nosniff'}});
   }catch{return new Response('Not found',{status:404});}
  });
  await createWindow();
 });
 app.on('window-all-closed',()=>app.quit());
}
