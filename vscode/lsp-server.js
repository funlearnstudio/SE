'use strict';
const fs = require('node:fs');
const path = require('node:path');
const { execFile } = require('node:child_process');
const { fileURLToPath, pathToFileURL } = require('node:url');
const { parseSeCheckOutput } = require('./diagnostics');

// SE Language Server: dependency-free LSP over stdio, Node.js 20+.
// Semantic error messages are supplied by the real SE compiler.
const documents = new Map();
const pending = new Map();
const roots = [];
const ignored = new Set(['.git', '.se', 'node_modules', 'build', 'dist', '.next']);
const keywords = ['say','ask','read','make','give','if','else','repeat','for','while',
  'in','use','type','match','case','try','fail','true','false','and','or','not'];
let stopped = false;
function send(msg) {
  const body = Buffer.from(JSON.stringify(Object.assign({ jsonrpc:'2.0' }, msg)), 'utf8');
  process.stdout.write('Content-Length: ' + body.length + '\r\n\r\n');
  process.stdout.write(body);
}
function respond(id,result) { send({ id, result }); }
function uriPath(uri) {
  try { return uri.startsWith('file:') ? fileURLToPath(uri) : null; }
  catch (_) { return null; }
}
function toUri(file) { return pathToFileURL(file).toString(); }
function getText(uri) {
  if (documents.has(uri)) return documents.get(uri);
  const file = uriPath(uri);
  if (!file) return '';
  try { return fs.readFileSync(file,'utf8'); } catch (_) { return ''; }
}
function starts(text) {
  const a=[0];
  for(let i=0;i<text.length;i++) if(text[i]==='\n') a.push(i+1);
  return a;
}
function pos(a,at) {
  let lo=0,hi=a.length-1;
  while(lo<hi){const mid=Math.ceil((lo+hi)/2);if(a[mid]<=at)lo=mid;else hi=mid-1;}
  return {line:lo,character:at-a[lo]};
}
function at(text,p) {
  const a=starts(text),n=Math.max(0,Math.min(a.length-1,p.line));
  return Math.max(a[n],Math.min(text.length,a[n]+p.character));
}
function rng(a,from,to){return {start:pos(a,from),end:pos(a,to)};}
function selected(text,p){
  const n=at(text,p);let a=n,b=n;
  while(a>0&&/[A-Za-z0-9_]/.test(text[a-1]))a--;
  while(b<text.length&&/[A-Za-z0-9_]/.test(text[b]))b++;
  const name=text.slice(a,b);
  return /^[A-Za-z_][A-Za-z0-9_]*$/.test(name)?{name,a,b}:null;
}
function symbols(uri) {
  const text=getText(uri),lines=text.split('\n'),start=starts(text),out=[];
  let at=0;
  for(const line of lines){
    const code=line.replace(/#.*$/,'');
    const match=code.match(/^\s*(?:make|type)\s+([A-Za-z_]\w*)\b/) ||
      code.match(/^\s*read\s+([A-Za-z_]\w*)\s*(?:\[[^\]]+\])?\s*:/) ||
      code.match(/^\s*([A-Za-z_]\w*)(?:\s*\[[^\]]+\])*\s*(?::\s*(?:Int|Num|Bool|Text))?\s*=/);
    if(match){
      const name=match[1],index=code.indexOf(name);
      const kind=/^\s*make\b/.test(code)?12:/^\s*type\b/.test(code)?5:13;
      out.push({uri,name,kind,range:rng(start,at+index,at+index+name.length)});
    }
    at+=line.length+1;
  }
  return out;
}
function workspaceFiles(){
  const found=new Set(documents.keys()),seen=new Set(),stack=roots.slice();
  let count=0;
  while(stack.length&&count<2000&&found.size<350){
    const dir=stack.pop();if(!dir||seen.has(dir))continue;
    seen.add(dir);count++;
    let entries;try{entries=fs.readdirSync(dir,{withFileTypes:true});}catch(_){continue;}
    for(const e of entries){
      if(e.isSymbolicLink()||ignored.has(e.name))continue;
      const full=path.join(dir,e.name);
      if(e.isDirectory())stack.push(full);
      else if(e.isFile()&&e.name.endsWith('.se'))found.add(toUri(full));
    }
  }
  return [...found];
}
function allSymbols(){return workspaceFiles().flatMap(symbols);}
function occurrences(uri,name) {
  const text=getText(uri),a=starts(text),found=[];
  let quote=false,escaped=false,comment=false;
  for(let i=0;i<text.length;){
    const ch=text[i];
    if(ch==='\n'){comment=false;i++;continue;}
    if(comment){i++;continue;}
    if(quote){if(escaped)escaped=false;else if(ch==='\\')escaped=true;else if(ch==='"')quote=false;i++;continue;}
    if(ch==='#'){comment=true;i++;continue;}
    if(ch==='"'){quote=true;i++;continue;}
    if(/[A-Za-z_]/.test(ch)){
      const begin=i++;
      while(i<text.length&&/[A-Za-z0-9_]/.test(text[i]))i++;
      if(text.slice(begin,i)===name)found.push({uri,range:rng(a,begin,i)});
    }else i++;
  }
  return found;
}
function normalize(source){
  const result=source.replace(/\r\n?/g,'\n').split('\n').map(line=>{
    const clean=line.replace(/[ \t]+$/g,'');
    const indent=clean.match(/^[ \t]*/)[0];
    const spaces=[...indent].reduce((n,c)=>n+(c==='\t'?4:1),0);
    return ' '.repeat(spaces)+clean.slice(indent.length);
  }).join('\n');
  return result.endsWith('\n')?result:result+'\n';
}
function schedule(uri){
  if(pending.has(uri))clearTimeout(pending.get(uri));
  pending.set(uri,setTimeout(()=>{pending.delete(uri);diagnose(uri);},300));
}
function diagnose(uri){
  const file=uriPath(uri);
  if(!file||!fs.existsSync(file)||!documents.has(uri))return;
  const temp=path.join(path.dirname(file),'.se-lsp-'+process.pid+'-'+
    Math.random().toString(16).slice(2)+'.se');
  try{fs.writeFileSync(temp,getText(uri),{flag:'wx'});}catch(_){return;}
  execFile(process.env.SE_EXE||'se',['check',temp],{timeout:15000,maxBuffer:1048576},(error,out,err)=>{
    try{fs.unlinkSync(temp);}catch(_){}
    if(!documents.has(uri))return;
    const parsed=error?parseSeCheckOutput(err||out):null;
    if(error&&!parsed)return;
    const diagnostics=parsed?[{severity:1,source:'SE',
      range:{start:{line:parsed.line,character:parsed.column},
        end:{line:parsed.line,character:parsed.column+1}},
      message:parsed.message+(parsed.hint?'\nHint: '+parsed.hint:'')}]:[];
    send({method:'textDocument/publishDiagnostics',params:{uri,diagnostics}});
  });
}
function handle(msg){
  const p=msg.params||{},method=msg.method,id=msg.id;
  if(method==='initialize'){
    roots.length=0;
    for(const folder of p.workspaceFolders||[]){const f=uriPath(folder.uri);if(f)roots.push(f);}
    if(!roots.length&&p.rootUri){const f=uriPath(p.rootUri);if(f)roots.push(f);}
    respond(id,{capabilities:{
      positionEncoding:'utf-16',textDocumentSync:{openClose:true,change:1},
      hoverProvider:true,definitionProvider:true,referencesProvider:true,
      renameProvider:{prepareProvider:true},documentSymbolProvider:true,
      completionProvider:{triggerCharacters:['.']},documentFormattingProvider:true
    },serverInfo:{name:'SE Language Server',version:'0.1.0'}});
    return;
  }
  if(method==='shutdown'){stopped=true;respond(id,null);return;}
  if(method==='exit'){process.exit(stopped?0:1);return;}
  if(method==='textDocument/didOpen'){
    documents.set(p.textDocument.uri,p.textDocument.text);schedule(p.textDocument.uri);return;
  }
  if(method==='textDocument/didChange'){
    if(p.contentChanges&&p.contentChanges.length){
      documents.set(p.textDocument.uri,p.contentChanges[p.contentChanges.length-1].text);
      schedule(p.textDocument.uri);
    }
    return;
  }
  if(method==='textDocument/didSave'){schedule(p.textDocument.uri);return;}
  if(method==='textDocument/didClose'){
    documents.delete(p.textDocument.uri);
    if(pending.has(p.textDocument.uri))clearTimeout(pending.get(p.textDocument.uri));
    pending.delete(p.textDocument.uri);
    send({method:'textDocument/publishDiagnostics',params:{uri:p.textDocument.uri,diagnostics:[]}});
    return;
  }
  if(id===undefined)return;
  const uri=p.textDocument&&p.textDocument.uri,text=uri?getText(uri):'';
  const token=uri?selected(text,p.position||{line:0,character:0}):null;
  if(method==='textDocument/completion'){
    const map=new Map(keywords.map(x=>[x,14]));
    for(const item of allSymbols())map.set(item.name,item.kind===12?3:item.kind===5?7:6);
    respond(id,[...map].map(([label,kind])=>({label,kind})));
  }else if(method==='textDocument/hover'){
    const def=token&&allSymbols().find(x=>x.name===token.name);
    respond(id,def?{contents:{kind:'markdown',value:'SE '+(def.kind===12?'function':def.kind===5?'type':'variable')+' '+token.name}}:null);
  }else if(method==='textDocument/definition'){
    const defs=token?allSymbols().filter(x=>x.name===token.name):[];
    respond(id,defs[0]||null);
  }else if(method==='textDocument/references'){
    const defs=token?allSymbols().filter(x=>x.name===token.name):[];
    const matches=token?workspaceFiles().flatMap(u=>occurrences(u,token.name)):[];
    respond(id,p.context&&p.context.includeDeclaration?matches:matches.filter(x=>
      !defs.some(y=>y.uri===x.uri&&JSON.stringify(y.range)===JSON.stringify(x.range))));
  }else if(method==='textDocument/prepareRename'){
    respond(id,token&&!keywords.includes(token.name)?rng(starts(text),token.a,token.b):null);
  }else if(method==='textDocument/rename'){
    const name=p.newName;
    if(!token||keywords.includes(token.name)||!/^[A-Za-z_]\w*$/.test(name)||keywords.includes(name)){
      respond(id,null);return;
    }
    const changes={};
    for(const u of workspaceFiles()){
      const edits=occurrences(u,token.name).map(item=>({range:item.range,newText:name}));
      if(edits.length)changes[u]=edits;
    }
    respond(id,{changes});
  }else if(method==='textDocument/documentSymbol'){
    respond(id,symbols(uri).map(x=>({name:x.name,kind:x.kind,location:{uri:x.uri,range:x.range}})));
  }else if(method==='textDocument/formatting'){
    const formatted=normalize(text);
    respond(id,formatted===text?[]:[{range:rng(starts(text),0,text.length),newText:formatted}]);
  }else respond(id,null);
}
let buffer=Buffer.alloc(0);
process.stdin.on('data',chunk=>{
  buffer=Buffer.concat([buffer,chunk]);
  while(true){
    const end=buffer.indexOf('\r\n\r\n');
    if(end<0)break;
    const header=buffer.subarray(0,end).toString('ascii');
    const match=header.match(/(?:^|\r\n)Content-Length:\s*(\d+)/i);
    if(!match){buffer=buffer.subarray(end+4);continue;}
    const length=Number(match[1]);
    if(!Number.isSafeInteger(length)||length>8*1024*1024){
      process.stderr.write('Invalid SE LSP message length\n');process.exit(1);
    }
    if(buffer.length<end+4+length)break;
    const text=buffer.subarray(end+4,end+4+length).toString('utf8');
    buffer=buffer.subarray(end+4+length);
    try{handle(JSON.parse(text));}catch(error){
      process.stderr.write('SE LSP: '+String(error)+'\n');
    }
  }
});
process.stdin.resume();
