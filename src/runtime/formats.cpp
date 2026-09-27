#include "s/formats.hpp"
#include "s/ecosystem.hpp"
#include "s/error.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#ifndef _WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#endif

namespace s {
namespace {
const char* script=R"SEPY(import datetime
import base64
import email
import email.policy
from email.message import EmailMessage
from email import message_from_string, message_from_bytes
import ftplib
import hashlib
import hmac
import html
import imaplib
import json
import secrets
import smtplib
import ssl
import sys
import time
import urllib.request
import traceback
import xml.etree.ElementTree as ET

with open(sys.argv[1], encoding='utf-8') as f:
    request = json.load(f)
name, op, args = request['name'], request['op'], request['args']
try:
    if name == 'toml':
        import tomllib
        if op != 'parse':
            raise ValueError('Unknown TOML operation')
        value = tomllib.loads(args[0])
    elif name == 'yaml':
        try:
            import yaml
        except ImportError as exc:
            raise RuntimeError('yaml.parse requires PyYAML installed for the selected Python') from exc
        if op == 'parse':
            value = yaml.safe_load(args[0])
        elif op == 'stringify':
            value = yaml.safe_dump(args[0], allow_unicode=True, sort_keys=False)
        else:
            raise ValueError('Unknown YAML operation')
    elif name == 'xml':
        def check(s):
            if len(s) > 1000000 or '<!DOCTYPE' in s.upper() or '<!ENTITY' in s.upper():
                raise ValueError('XML DTD, entities and input above 1MB are not supported')
        def as_map(node):
            return {'tag': node.tag, 'text': node.text or '', 'attributes': dict(node.attrib), 'children': [as_map(child) for child in node]}
        if op == 'parse':
            check(args[0]); value = as_map(ET.fromstring(args[0]))
        elif op == 'escape':
            value = html.escape(args[0], quote=True)
        else:
            raise ValueError('Unknown XML operation')
    elif name == 'markdown':
        if op != 'render':
            raise ValueError('Unknown Markdown operation')
        try:
            import markdown_it
        except ImportError as exc:
            raise RuntimeError('markdown.render requires markdown-it-py installed for the selected Python') from exc
        value = markdown_it.MarkdownIt('commonmark', {'html': False}).render(args[0])
    elif name == 'crypto':
        if op == 'sha256':
            value = hashlib.sha256(args[0].encode('utf-8')).hexdigest()
        elif op == 'hmac_sha256':
            value = hmac.new(args[0].encode('utf-8'), args[1].encode('utf-8'), hashlib.sha256).hexdigest()
        elif op == 'random_hex':
            if not isinstance(args[0], int) or isinstance(args[0], bool) or not 1 <= args[0] <= 4096:
                raise ValueError('random_hex length must be 1..4096 bytes')
            value = secrets.token_hex(args[0])
        elif op == 'constant_time_equal':
            value = hmac.compare_digest(args[0].encode('utf-8'), args[1].encode('utf-8'))
        else:
            raise ValueError('Unknown crypto operation')
    elif name in ('jwt', 'session'):
        def encoded(data):
            raw = json.dumps(data, separators=(',', ':'), ensure_ascii=False).encode('utf-8')
            return base64.urlsafe_b64encode(raw).rstrip(b'=').decode('ascii')
        def decoded(data):
            if len(data) > 100000 or any(c not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_' for c in data):
                raise ValueError('Invalid JWT encoding')
            raw = base64.urlsafe_b64decode(data + '=' * (-len(data) % 4))
            return json.loads(raw)
        if op in ('sign', 'encode'):
            payload, key = args
            if not isinstance(payload, dict) or not isinstance(key, str) or not key:
                raise ValueError('JWT requires a Map payload and a nonempty signing key')
            header = encoded({'alg': 'HS256', 'typ': 'JWT'})
            body = encoded(payload)
            message = header + '.' + body
            signature = base64.urlsafe_b64encode(hmac.new(key.encode('utf-8'), message.encode('ascii'), hashlib.sha256).digest()).rstrip(b'=').decode('ascii')
            value = message + '.' + signature
        elif op in ('verify', 'decode'):
            token, key = args
            if not isinstance(key, str) or not key or not isinstance(token, str) or len(token) > 100000:
                raise ValueError('Invalid JWT or signing key')
            parts = token.split('.')
            if len(parts) != 3 or decoded(parts[0]) != {'alg': 'HS256', 'typ': 'JWT'}:
                raise ValueError('Only HS256 JWT is supported')
            message = parts[0] + '.' + parts[1]
            expected = base64.urlsafe_b64encode(hmac.new(key.encode('utf-8'), message.encode('ascii'), hashlib.sha256).digest()).rstrip(b'=').decode('ascii')
            if not hmac.compare_digest(expected, parts[2]):
                raise ValueError('Invalid JWT signature')
            value = decoded(parts[1])
            if not isinstance(value, dict):
                raise ValueError('JWT payload must be a Map')
            if 'exp' in value and (not isinstance(value['exp'], (int, float)) or isinstance(value['exp'], bool) or time.time() >= value['exp']):
                raise ValueError('JWT has expired')
        else:
            raise ValueError('Unknown JWT operation')
    elif name == 'auth':
        if op == 'hash_password':
            password = args[0]
            if not isinstance(password, str):
                raise ValueError('Password must be Text')
            salt = secrets.token_bytes(16)
            rounds = 250000
            value = 'pbkdf2_sha256$' + str(rounds) + '$' + salt.hex() + '$' + hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, rounds).hex()
        elif op == 'verify_password':
            password, stored = args
            kind, rounds, salt, expected = stored.split('$')
            if kind != 'pbkdf2_sha256' or not 100000 <= int(rounds) <= 1000000 or len(salt) != 32 or len(expected) != 64:
                raise ValueError('Invalid password hash')
            actual = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), bytes.fromhex(salt), int(rounds))
            value = hmac.compare_digest(actual, bytes.fromhex(expected))
        else:
            raise ValueError('Unknown auth operation')
    elif name == 'email':
        if op == 'compose':
            sender, recipient, subject, body = args
            message = EmailMessage()
            message['From'] = sender
            message['To'] = recipient
            message['Subject'] = subject
            message.set_content(body)
            value = message.as_string()
        elif op == 'parse':
            message = message_from_string(args[0], policy=email.policy.default)
            value = {'from': str(message.get('From', '')), 'to': str(message.get('To', '')), 'subject': str(message.get('Subject', '')), 'body': message.get_body(preferencelist=('plain',)).get_content() if message.get_body(preferencelist=('plain',)) else ''}
        else:
            raise ValueError('Unknown email operation')
    elif name == 'smtp':
        if op != 'send':
            raise ValueError('Unknown SMTP operation')
        host, port, username, password, recipient, message_text = args
        if not 1 <= port <= 65535:
            raise ValueError('Invalid SMTP port')
        with smtplib.SMTP_SSL(host, port, context=ssl.create_default_context(), timeout=15) as connection:
            connection.login(username, password)
            connection.sendmail(username, [recipient], message_text)
        value = True
    elif name == 'imap':
        if op != 'subjects':
            raise ValueError('Unknown IMAP operation')
        host, username, password, mailbox, limit = args
        if not 1 <= limit <= 100:
            raise ValueError('IMAP limit must be 1..100')
        with imaplib.IMAP4_SSL(host, ssl_context=ssl.create_default_context(), timeout=15) as connection:
            connection.login(username, password)
            status, _ = connection.select(mailbox, readonly=True)
            if status != 'OK':
                raise RuntimeError('Cannot open mailbox')
            status, data = connection.search(None, 'ALL')
            if status != 'OK':
                raise RuntimeError('IMAP search failed')
            result = []
            for uid in data[0].split()[-limit:]:
                status, fetched = connection.fetch(uid, '(BODY.PEEK[HEADER.FIELDS (SUBJECT FROM DATE)])')
                if status != 'OK' or not fetched or not isinstance(fetched[0], tuple):
                    continue
                headers = message_from_bytes(fetched[0][1], policy=email.policy.default)
                result.append({'subject': str(headers.get('Subject', '')), 'from': str(headers.get('From', '')), 'date': str(headers.get('Date', ''))})
            value = result
    elif name == 'ftp':
        host, username, password, path = args[:4]
        with ftplib.FTP_TLS(host, timeout=15, context=ssl.create_default_context()) as connection:
            connection.login(username, password)
            connection.prot_p()
            if op == 'list':
                value = connection.nlst(path)
            elif op == 'download':
                import io
                buffer = io.BytesIO()
                connection.retrbinary('RETR ' + path, buffer.write)
                value = base64.b64encode(buffer.getvalue()).decode('ascii')
            elif op == 'upload':
                import io
                connection.storbinary('STOR ' + path, io.BytesIO(args[4].encode('utf-8')))
                value = True
            else:
                raise ValueError('Unknown FTPS operation')
    elif name == 'ssh':
        if op != 'run':
            raise ValueError('Unknown SSH operation')
        host, username, command = args
        try:
            import paramiko
        except ImportError as exc:
            raise RuntimeError('ssh.run requires paramiko installed for the selected Python') from exc
        client = paramiko.SSHClient()
        client.load_system_host_keys()
        client.set_missing_host_key_policy(paramiko.RejectPolicy())
        try:
            client.connect(host, username=username, timeout=15, look_for_keys=True, allow_agent=True)
            _, output, errors = client.exec_command(command, timeout=30)
            value = {'stdout': output.read().decode('utf-8', 'replace'), 'stderr': errors.read().decode('utf-8', 'replace'), 'status': output.channel.recv_exit_status()}
        finally:
            client.close()
    elif name == 'websocket':
        if op != 'exchange':
            raise ValueError('Unknown WebSocket operation')
        url, message = args
        if not url.startswith('wss://'):
            raise ValueError('websocket.exchange requires a wss:// URL')
        try:
            from websockets.sync.client import connect
        except ImportError as exc:
            raise RuntimeError('websocket.exchange requires websockets installed for the selected Python') from exc
        with connect(url, open_timeout=15, close_timeout=5) as socket:
            socket.send(message)
            reply = socket.recv(timeout=15)
            value = reply if isinstance(reply, str) else base64.b64encode(reply).decode('ascii')
    elif name in ('ai', 'embedding'):
        endpoint, api_key, model, text = args
        if not endpoint.startswith('https://'):
            raise ValueError('AI requests require an https:// endpoint')
        if name == 'ai' and op == 'chat':
            payload = {'model': model, 'messages': [{'role': 'user', 'content': text}]}
        elif name == 'embedding' and op == 'create':
            payload = {'model': model, 'input': text}
        else:
            raise ValueError('Unknown AI operation')
        request = urllib.request.Request(endpoint, data=json.dumps(payload).encode('utf-8'), headers={'Authorization': 'Bearer ' + api_key, 'Content-Type': 'application/json'}, method='POST')
        with urllib.request.urlopen(request, timeout=30) as response:
            result = json.load(response)
        value = result['choices'][0]['message']['content'] if name == 'ai' else result['data'][0]['embedding']
    elif name == 'ml':
        if op == 'linear_regression':
            xs, ys = args
            if len(xs) != len(ys) or len(xs) < 2 or not all(isinstance(x, (int, float)) and not isinstance(x, bool) for x in xs + ys):
                raise ValueError('Regression needs matching numeric Lists of length >= 2')
            mean_x, mean_y = sum(xs) / len(xs), sum(ys) / len(ys)
            denominator = sum((x-mean_x)**2 for x in xs)
            if denominator == 0:
                raise ValueError('All x values are equal')
            slope = sum((x-mean_x)*(y-mean_y) for x, y in zip(xs, ys)) / denominator
            value = {'slope': slope, 'intercept': mean_y - slope * mean_x}
        elif op == 'predict':
            model, x = args
            value = model['slope'] * x + model['intercept']
        else:
            raise ValueError('Unknown ML operation')
    elif name == 'tensor':
        def shape(a):
            if not isinstance(a, list):
                if not isinstance(a, (int, float)) or isinstance(a, bool):
                    raise ValueError('Tensor values must be numeric')
                return []
            if not a:
                return [0]
            inner = shape(a[0])
            if any(shape(item) != inner for item in a[1:]):
                raise ValueError('Tensor must be rectangular')
            return [len(a)] + inner
        def add(a, b):
            return [add(x, y) for x, y in zip(a, b)] if isinstance(a, list) else a + b
        if op == 'shape':
            value = shape(args[0])
        elif op == 'add':
            if shape(args[0]) != shape(args[1]):
                raise ValueError('Tensor dimensions do not match')
            value = add(*args)
        elif op == 'matmul':
            a, b = args
            ashape, bshape = shape(a), shape(b)
            if len(ashape) != 2 or len(bshape) != 2 or ashape[1] != bshape[0]:
                raise ValueError('matmul needs compatible 2D tensors')
            value = [[sum(a[row][k] * b[k][col] for k in range(ashape[1])) for col in range(bshape[1])] for row in range(ashape[0])]
        else:
            raise ValueError('Unknown tensor operation')
    else:
        raise ValueError('Unknown format module')
    result = {'ok': True, 'value': value}
except Exception as exc:
    result = {'ok': False, 'error': str(exc)}
def normalized(obj):
    if isinstance(obj, (datetime.datetime, datetime.date, datetime.time)):
        return obj.isoformat()
    if isinstance(obj, dict):
        return {str(k): normalized(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [normalized(v) for v in obj]
    return obj
with open(sys.argv[2], 'w', encoding='utf-8') as f:
    json.dump(normalized(result), f, ensure_ascii=False, allow_nan=False)
)SEPY";
std::string shell_quote(const std::string& s){
#ifdef _WIN32
 std::string out="\"";for(char c:s){if(c=='\"')out+="\"\"";else out+=c;}return out+'\"';
#else
 std::string out="'";for(char c:s){if(c=='\'')out+="'\\''";else out+=c;}return out+'\'';
#endif
}
std::string text(const Value& v,SourcePos p){if(auto x=std::get_if<std::string>(&v.data()))return *x;throw Error(p,"Format parser needs Text.");}
Value map_get(const Value& value,const std::string& key,SourcePos p){auto mp=std::get_if<std::shared_ptr<MapData>>(&value.data());if(!mp)throw Error(p,"Invalid format bridge response.");for(auto& [name,item]:(*mp)->items)if(name==key)return item;throw Error(p,"Format bridge response is missing "+key+".");}
struct TemporaryFiles{
 std::filesystem::path directory,source,input,output;
 ~TemporaryFiles(){std::error_code ec;std::filesystem::remove_all(directory,ec);}
};
Value invoke_format(const std::string& name,const std::string& op,const std::vector<Value>& args,Interpreter& vm,SourcePos p){
 auto codec=ecosystem_builtin_module("pickle",vm);
 auto dump=std::get<std::shared_ptr<CallableData>>(codec->exports.at("dumps").data());
 auto load=std::get<std::shared_ptr<CallableData>>(codec->exports.at("loads").data());
 auto sequence=std::make_shared<ListData>();sequence->items=args;
 auto payload=std::make_shared<MapData>();payload->items={{"name",Value(name)},{"op",Value(op)},{"args",Value(sequence)}};
 auto serialized=text(dump->call({Value(payload)},p),p);
 std::random_device rd;auto nonce=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(rd());
 auto directory=std::filesystem::temp_directory_path()/("se-bridge-"+nonce);
#ifdef _WIN32
 if(_mkdir(directory.string().c_str())!=0)throw Error(p,"Cannot create private bridge directory.");
#else
 if(::mkdir(directory.string().c_str(),0700)!=0)throw Error(p,"Cannot create private bridge directory.");
#endif
 TemporaryFiles files{directory,directory/"bridge.py",directory/"input.json",directory/"output.json"};
 {std::ofstream source(files.source),input(files.input);if(!source||!input)throw Error(p,"Cannot create format parser temporary files.");source<<script;input<<serialized;}
#ifdef _WIN32
 const std::string interpreter="python";
#else
 const std::string interpreter="python3";
#endif
 auto command=interpreter+" "+shell_quote(files.source.string())+" "+shell_quote(files.input.string())+" "+shell_quote(files.output.string());
 if(std::system(command.c_str())!=0)throw Error(p,"Python format bridge failed. Install Python 3.11+ and required format packages.");
 std::ifstream output(files.output);if(!output)throw Error(p,"Python format bridge produced no output.");
 std::string response{std::istreambuf_iterator<char>(output),{}};
 auto parsed=load->call({Value(response)},p);auto ok=map_get(parsed,"ok",p);
 if(!std::holds_alternative<bool>(ok.data()))throw Error(p,"Invalid format bridge response status.");
 if(!std::get<bool>(ok.data()))throw Error(p,text(map_get(parsed,"error",p),p));
 return map_get(parsed,"value",p);
}
TypeInfo fn_type(std::vector<TypeInfo> params,TypeInfo output){TypeInfo t(TypeKind::Function);auto signature=std::make_shared<FunctionSig>();signature->params=std::move(params);signature->result=std::move(output);signature->fallible=true;t.callable=signature;return t;}
}
bool is_format_builtin(const std::string& name){return name=="toml"||name=="yaml"||name=="xml"||name=="markdown"||name=="crypto"||name=="jwt"||name=="session"||name=="auth"||name=="email"||name=="smtp"||name=="imap"||name=="ftp"||name=="ssh"||name=="websocket"||name=="ai"||name=="ml"||name=="tensor"||name=="embedding";}
TypeInfo format_builtin_type(const std::string& name){TypeInfo module(TypeKind::Module);module.name=name;TypeInfo text_type(TypeKind::Text),unknown,integer(TypeKind::Int),boolean(TypeKind::Bool);
 if(name=="toml"||name=="yaml"||name=="xml")module.members["parse"]=fn_type({text_type},unknown);
 if(name=="yaml")module.members["stringify"]=fn_type({unknown},text_type);
 if(name=="xml")module.members["escape"]=fn_type({text_type},text_type);
 if(name=="markdown")module.members["render"]=fn_type({text_type},text_type);
 if(name=="crypto"){
  module.members["sha256"]=fn_type({text_type},text_type);
  module.members["hmac_sha256"]=fn_type({text_type,text_type},text_type);
  module.members["random_hex"]=fn_type({integer},text_type);
  module.members["constant_time_equal"]=fn_type({text_type,text_type},boolean);
 }
 if(name=="jwt"||name=="session"){
  module.members[name=="jwt"?"sign":"encode"]=fn_type({unknown,text_type},text_type);
  module.members[name=="jwt"?"verify":"decode"]=fn_type({text_type,text_type},unknown);
 }
 if(name=="auth"){
  module.members["hash_password"]=fn_type({text_type},text_type);
  module.members["verify_password"]=fn_type({text_type,text_type},boolean);
 }
 if(name=="email"){
  module.members["compose"]=fn_type({text_type,text_type,text_type,text_type},text_type);
  module.members["parse"]=fn_type({text_type},unknown);
 }
 if(name=="smtp")module.members["send"]=fn_type({text_type,integer,text_type,text_type,text_type,text_type},boolean);
 if(name=="imap")module.members["subjects"]=fn_type({text_type,text_type,text_type,text_type,integer},unknown);
 if(name=="ftp"){
  module.members["list"]=fn_type({text_type,text_type,text_type,text_type},unknown);
  module.members["download"]=fn_type({text_type,text_type,text_type,text_type},text_type);
  module.members["upload"]=fn_type({text_type,text_type,text_type,text_type,text_type},boolean);
 }
 if(name=="ssh")module.members["run"]=fn_type({text_type,text_type,text_type},unknown);
 if(name=="websocket")module.members["exchange"]=fn_type({text_type,text_type},text_type);
 if(name=="ai")module.members["chat"]=fn_type({text_type,text_type,text_type,text_type},text_type);
 if(name=="embedding")module.members["create"]=fn_type({text_type,text_type,text_type,text_type},unknown);
 if(name=="ml"){
  module.members["linear_regression"]=fn_type({unknown,unknown},unknown);
  module.members["predict"]=fn_type({unknown,TypeInfo(TypeKind::Num)},TypeInfo(TypeKind::Num));
 }
 if(name=="tensor"){
  module.members["shape"]=fn_type({unknown},unknown);
  module.members["add"]=fn_type({unknown,unknown},unknown);
  module.members["matmul"]=fn_type({unknown,unknown},unknown);
 }
 return module;
}
std::shared_ptr<ModuleData> format_builtin_module(const std::string& name,Interpreter& vm){auto module=std::make_shared<ModuleData>();module->name=name;for(auto& [op,type]:format_builtin_type(name).members){auto fn=std::make_shared<CallableData>();fn->name=name+"."+op;fn->min_args=fn->max_args=type.callable->params.size();fn->call=[name,op,&vm](const std::vector<Value>& args,SourcePos p){return invoke_format(name,op,args,vm,p);};module->exports[op]=Value(fn);}return module;}
}
