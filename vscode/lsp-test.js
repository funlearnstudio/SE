'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { pathToFileURL } = require('node:url');

async function main() {
  const dir=fs.mkdtempSync(path.join(os.tmpdir(),'se-lsp-test-'));
  const helper=path.join(dir,'helper.se'),mainFile=path.join(dir,'main.se'),messy=path.join(dir,'messy.se');
  fs.writeFileSync(helper,'make inc x:Int -> Int\n    give x + 1\n');
  fs.writeFileSync(mainFile,'use helper\nsay inc 41\n');
  fs.writeFileSync(messy,'if true\n\tsay 1  \n');
  const uri=pathToFileURL(mainFile).toString(),helperUri=pathToFileURL(helper).toString();
  const messyUri=pathToFileURL(messy).toString();
  const server=spawn(process.execPath,[path.join(__dirname,'lsp-server.js')],{stdio:['pipe','pipe','pipe']});
  let buffer=Buffer.alloc(0),id=0,errors='';
  const waiting=new Map();
  const timeout=setTimeout(()=>server.kill(),12000);
  // On Windows, a child may close stdin as it handles the LSP exit notification.
  // Prevent a late EPIPE from crashing the parent after a successful test.
  server.stdin.on('error',error=>{
    if(error.code!=='EPIPE')process.stderr.write('LSP stdin: '+String(error)+'\n');
  });
  const childExited=new Promise(resolve=>server.once('exit',resolve));
  server.stderr.on('data',x=>{errors+=x.toString();});
  server.stdout.on('data',chunk=>{
    buffer=Buffer.concat([buffer,chunk]);
    while(true) {
      const end=buffer.indexOf('\r\n\r\n');
      if(end<0)break;
      const header=buffer.subarray(0,end).toString('ascii');
      const match=header.match(/Content-Length:\s*(\d+)/i);
      if(!match)throw new Error('LSP missing Content-Length');
      const length=Number(match[1]);
      if(buffer.length<end+4+length)break;
      const message=JSON.parse(buffer.subarray(end+4,end+4+length).toString('utf8'));
      buffer=buffer.subarray(end+4+length);
      if(message.id!==undefined&&waiting.has(message.id)){
        const done=waiting.get(message.id);waiting.delete(message.id);done(message.result);
      }
    }
  });
  function send(method,params,id){
    const body=Buffer.from(JSON.stringify({jsonrpc:'2.0',method,params,...(id===undefined?{}:{id})}));
    server.stdin.write(Buffer.concat([Buffer.from('Content-Length: '+body.length+'\r\n\r\n'),body]));
  }
  function request(method,params){
    const requestId=++id;
    return new Promise(resolve=>{waiting.set(requestId,resolve);send(method,params,requestId);});
  }
  try {
    const init=await request('initialize',{workspaceFolders:[{uri:pathToFileURL(dir).toString(),name:'test'}]});
    assert.equal(init.serverInfo.name,'SE Language Server');
    send('initialized',{});
    send('textDocument/didOpen',{textDocument:{uri,languageId:'se',version:1,text:fs.readFileSync(mainFile,'utf8')}});
    send('textDocument/didOpen',{textDocument:{uri:helperUri,languageId:'se',version:1,text:fs.readFileSync(helper,'utf8')}});
    const completion=await request('textDocument/completion',{textDocument:{uri},position:{line:1,character:5}});
    assert(completion.some(x=>x.label==='inc'),'Cross-file completion missing');
    const definition=await request('textDocument/definition',{textDocument:{uri},position:{line:1,character:5}});
    assert.equal(definition.uri,helperUri);
    const references=await request('textDocument/references',{textDocument:{uri},position:{line:1,character:5},context:{includeDeclaration:true}});
    assert(references.length>=2,'Cross-file references missing');
    const renamed=await request('textDocument/rename',{textDocument:{uri},position:{line:1,character:5},newName:'increment'});
    assert(renamed.changes[helperUri].length>=1&&renamed.changes[uri].length>=1,'Cross-file rename missing');
    const symbols=await request('textDocument/documentSymbol',{textDocument:{uri:helperUri}});
    assert(symbols.some(x=>x.name==='inc'));
    send('textDocument/didOpen',{textDocument:{uri:messyUri,languageId:'se',version:1,text:fs.readFileSync(messy,'utf8')}});
    const formatting=await request('textDocument/formatting',{textDocument:{uri:messyUri},options:{tabSize:4,insertSpaces:true}});
    assert.equal(formatting[0].newText,'if true\n    say 1\n');
    await request('shutdown',{});
    send('exit',{});
    // Give the child time to acknowledge and close its pipe before cleanup.
    await Promise.race([childExited,new Promise(resolve=>setTimeout(resolve,2000))]);
    console.log('SE LSP protocol smoke test passed.');
  } finally {
    clearTimeout(timeout);
    server.kill();
    fs.rmSync(dir,{recursive:true,force:true});
    if(errors&&process.env.SE_LSP_DEBUG)process.stderr.write(errors);
  }
}
main().catch(error=>{process.stderr.write(String(error.stack||error)+'\n');process.exitCode=1;});
