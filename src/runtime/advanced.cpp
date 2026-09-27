#include "s/advanced.hpp"
#include "s/interpreter.hpp"
#include "s/error.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace s {
namespace {

std::shared_ptr<CallableData> callable(std::string name,std::size_t min,std::size_t max,
    std::function<Value(const std::vector<Value>&,SourcePos)> fn,bool variadic=false){
  auto c=std::make_shared<CallableData>();
  c->name=std::move(name);c->min_args=min;c->max_args=max;c->variadic=variadic;c->call=std::move(fn);
  return c;
}
std::shared_ptr<FunctionSig> sig(std::vector<TypeInfo> params,TypeInfo result,bool variadic=false,std::size_t min=0,bool fallible=false){
  auto s=std::make_shared<FunctionSig>();s->params=std::move(params);s->result=std::move(result);s->variadic=variadic;s->min_args=min;s->fallible=fallible;return s;
}
TypeInfo fn(std::vector<TypeInfo> p,TypeInfo r,bool variadic=false,std::size_t min=0,bool fallible=false){TypeInfo t(TypeKind::Function);t.callable=sig(std::move(p),std::move(r),variadic,min,fallible);return t;}
TypeInfo module_type(const std::string& n){TypeInfo t(TypeKind::Module);t.name=n;return t;}
TypeInfo list_type(){TypeInfo t(TypeKind::List);t.element=std::make_shared<TypeInfo>();return t;}
TypeInfo handle_type(std::string n){TypeInfo t(TypeKind::NativeHandle);t.name=std::move(n);return t;}

std::string text(const Value& v,SourcePos p,const std::string& name){if(auto s=std::get_if<std::string>(&v.data()))return *s;throw Error(p,name+" needs Text.");}
std::int64_t integer(const Value& v,SourcePos p,const std::string& name){if(auto n=std::get_if<std::int64_t>(&v.data()))return *n;throw Error(p,name+" needs Int.");}
std::shared_ptr<ListData> list(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::shared_ptr<ListData>>(&v.data()))return *x;throw Error(p,name+" needs a List.");}
std::shared_ptr<CallableData> function(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::shared_ptr<CallableData>>(&v.data()))return *x;throw Error(p,name+" needs a function.");}

template<class T> Value handle(std::string tag,std::shared_ptr<T> p){auto h=std::make_shared<NativeHandleData>();h->tag=std::move(tag);h->resource=std::move(p);return Value(h);}
template<class T> std::shared_ptr<T> as_handle(const Value& v,const std::string& tag,SourcePos p,const std::string& name){auto h=std::get_if<std::shared_ptr<NativeHandleData>>(&v.data());if(!h||!(*h)||(*h)->tag!=tag||!(*h)->resource)throw Error(p,name+" needs "+tag+".");return std::static_pointer_cast<T>((*h)->resource);}

Value key_value(const Value& v,const std::string& key,SourcePos p){
  if(auto m=std::get_if<std::shared_ptr<MapData>>(&v.data())){for(auto& x:(*m)->items)if(x.first==key)return x.second;throw Error(p,"sort_by key '"+key+"' is missing.");}
  if(auto o=std::get_if<std::shared_ptr<ObjectData>>(&v.data())){auto i=(*o)->fields.find(key);if(i!=(*o)->fields.end())return i->second;throw Error(p,"sort_by field '"+key+"' is missing.");}
  throw Error(p,"collections.sort_by needs a List of Map or object values.");
}
bool less_value(const Value& a,const Value& b,SourcePos p){
  if(auto x=std::get_if<std::int64_t>(&a.data())){if(auto y=std::get_if<std::int64_t>(&b.data()))return *x<*y;}
  if(auto x=std::get_if<double>(&a.data())){if(auto y=std::get_if<double>(&b.data()))return *x<*y;}
  if(auto x=std::get_if<std::string>(&a.data())){if(auto y=std::get_if<std::string>(&b.data()))return *x<*y;}
  throw Error(p,"Sort keys must all be Int, Num, or Text of one type.");
}

struct OptionData {bool some=false;Value value;};
struct ResultData {bool ok=false;Value value;std::string error;};
struct TaskData {std::shared_future<Value> future;};
struct DbData {std::filesystem::path path;std::vector<std::pair<std::string,std::string>> values;bool dirty=false;};

std::string escape_field(const std::string& s){std::ostringstream o;for(unsigned char c:s){if(c=='%'||c=='\t'||c=='\n'||c=='\r')o<<'%'<<std::hex<<static_cast<int>(c)<<';'<<std::dec;else o<<static_cast<char>(c);}return o.str();}
std::string unescape_field(const std::string& s){std::string o;for(std::size_t i=0;i<s.size();){if(s[i]=='%'){auto e=s.find(';',i+1);if(e==std::string::npos){o+=s[i++];continue;}try{o+=static_cast<char>(std::stoi(s.substr(i+1,e-i-1),nullptr,16));i=e+1;}catch(...){o+=s[i++];}}else o+=s[i++];}return o;}
void db_load(DbData& db){std::ifstream in(db.path,std::ios::binary);if(!in)return;std::string line;while(std::getline(in,line)){auto t=line.find('\t');if(t!=std::string::npos)db.values.emplace_back(unescape_field(line.substr(0,t)),unescape_field(line.substr(t+1)));}}
void db_flush(DbData& db,SourcePos p){if(!db.dirty)return;std::ofstream out(db.path,std::ios::binary|std::ios::trunc);if(!out)throw Error(p,"db could not write '"+db.path.string()+"'.");for(auto& [k,v]:db.values)out<<escape_field(k)<<'\t'<<escape_field(v)<<'\n';db.dirty=false;}

std::string shell_quote(const std::string& s){
#ifdef _WIN32
  std::string out="\"";for(char c:s){if(c=='\"')out+="\\\"";else out+=c;}return out+"\"";
#else
  std::string out="'";for(char c:s){if(c=='\'')out+="'\\''";else out+=c;}return out+"'";
#endif
}
std::string command_output(const std::string& command,SourcePos p){
#ifdef _WIN32
  FILE* pipe=_popen(command.c_str(),"r");
#else
  FILE* pipe=popen(command.c_str(),"r");
#endif
  if(!pipe)throw Error(p,"Could not start external HTTPS transport.");std::string out;char b[4096];while(std::fgets(b,sizeof(b),pipe))out+=b;
#ifdef _WIN32
  int code=_pclose(pipe);
#else
  int code=pclose(pipe);
#endif
  if(code!=0)throw RuntimeFailure({"HTTPS request failed. Install curl and check the URL.","",p.line,"HttpError"});return out;
}
std::string curl_request(const std::string& method,const std::string& url,const std::string& body,const std::string& type,SourcePos p){
  if(url.rfind("https://",0)!=0)throw Error(p,"HTTPS transport needs an https:// URL.");
  std::string command="curl --fail --silent --show-error --location --request "+shell_quote(method);
  if(!body.empty())command+=" --header "+shell_quote("Content-Type: "+type)+" --data-binary "+shell_quote(body);
  command+=" "+shell_quote(url);return command_output(command,p);
}

std::mutex async_vm_mutex;

} // namespace

bool is_advanced_builtin(const std::string& name){
  return name=="function"||name=="async"||name=="threading"||name=="option"||name=="result"||name=="match"||name=="db"||name=="https";
}

void extend_collections_type(TypeInfo& m){
  TypeInfo unknown,integer(TypeKind::Int),text_t(TypeKind::Text),function_t(TypeKind::Function);
  m.members["filter"]=fn({unknown,function_t},list_type());
  m.members["map"]=fn({unknown,function_t},list_type());
  m.members["reduce"]=fn({unknown,unknown,function_t},unknown);
  m.members["sort_by"]=fn({unknown,text_t},list_type());
  m.members["sort_by_desc"]=fn({unknown,text_t},list_type());
  m.members["sort_with"]=fn({unknown,function_t},list_type());
  m.members["slice"]=fn({unknown,integer,integer},list_type());
  m.members["take"]=fn({unknown,integer},list_type());
  m.members["drop"]=fn({unknown,integer},list_type());
}

void extend_collections_module(const std::shared_ptr<ModuleData>& m,Interpreter& vm){
  m->exports["filter"]=callable("collections.filter",2,2,[&vm](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.filter");function(a[1],p,"collections.filter");auto out=std::make_shared<ListData>();for(auto& v:in->items)if(vm.invoke(a[1],{v},p).truth(p))out->items.push_back(v);return Value(out);});
  m->exports["map"]=callable("collections.map",2,2,[&vm](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.map");function(a[1],p,"collections.map");auto out=std::make_shared<ListData>();for(auto& v:in->items)out->items.push_back(vm.invoke(a[1],{v},p));return Value(out);});
  m->exports["reduce"]=callable("collections.reduce",3,3,[&vm](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.reduce");function(a[2],p,"collections.reduce");Value acc=a[1];for(auto& v:in->items)acc=vm.invoke(a[2],{acc,v},p);return acc;});
  auto sort_by=[&](bool desc){return callable(desc?"collections.sort_by_desc":"collections.sort_by",2,2,[desc](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.sort_by");auto key=text(a[1],p,"collections.sort_by");auto out=std::make_shared<ListData>();out->items=in->items;std::stable_sort(out->items.begin(),out->items.end(),[&](const Value& x,const Value& y){auto akey=key_value(x,key,p),bkey=key_value(y,key,p);return desc?less_value(bkey,akey,p):less_value(akey,bkey,p);});return Value(out);});};
  m->exports["sort_by"]=sort_by(false);m->exports["sort_by_desc"]=sort_by(true);
  m->exports["sort_with"]=callable("collections.sort_with",2,2,[&vm](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.sort_with");function(a[1],p,"collections.sort_with");auto out=std::make_shared<ListData>();out->items=in->items;std::stable_sort(out->items.begin(),out->items.end(),[&](const Value& x,const Value& y){return vm.invoke(a[1],{x,y},p).truth(p);});return Value(out);});
  m->exports["slice"]=callable("collections.slice",3,3,[](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.slice");auto start=integer(a[1],p,"collections.slice"),end=integer(a[2],p,"collections.slice");auto n=static_cast<std::int64_t>(in->items.size());if(start<0)start=n+start;if(end<0)end=n+end;start=std::clamp<std::int64_t>(start,0,n);end=std::clamp<std::int64_t>(end,0,n);auto out=std::make_shared<ListData>();if(end>start)out->items.assign(in->items.begin()+start,in->items.begin()+end);return Value(out);});
  m->exports["take"]=callable("collections.take",2,2,[](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.take");auto n=integer(a[1],p,"collections.take");if(n<0)throw Error(p,"collections.take count cannot be negative.");auto out=std::make_shared<ListData>();auto count=std::min<std::size_t>(static_cast<std::size_t>(n),in->items.size());out->items.assign(in->items.begin(),in->items.begin()+static_cast<std::ptrdiff_t>(count));return Value(out);});
  m->exports["drop"]=callable("collections.drop",2,2,[](const std::vector<Value>& a,SourcePos p){auto in=list(a[0],p,"collections.drop");auto n=integer(a[1],p,"collections.drop");if(n<0)throw Error(p,"collections.drop count cannot be negative.");auto out=std::make_shared<ListData>();auto count=std::min<std::size_t>(static_cast<std::size_t>(n),in->items.size());out->items.assign(in->items.begin()+static_cast<std::ptrdiff_t>(count),in->items.end());return Value(out);});
}

TypeInfo advanced_builtin_type(const std::string& name){
  TypeInfo m=module_type(name),unknown,text_t(TypeKind::Text),bool_t(TypeKind::Bool),int_t(TypeKind::Int),none(TypeKind::None),func(TypeKind::Function);
  auto& x=m.members;
  if(name=="function"){
    x["bind"]=fn({func,unknown},func,true,2);x["partial"]=fn({func,unknown},func,true,1);x["call"]=fn({func,unknown},unknown,true,1);x["pipe"]=fn({unknown,func},unknown,true,2);x["reduce"]=fn({func,list_type(),unknown},unknown,true,2);x["map"]=fn({func,list_type()},list_type());x["filter"]=fn({func,list_type()},list_type());
  }else if(name=="async"){
    x["run"]=fn({func,unknown},handle_type("Task"),true,1);x["await"]=fn({handle_type("Task")},unknown,false,0,true);x["ready"]=fn({handle_type("Task")},bool_t);
  }else if(name=="threading"){
    x["run"]=fn({func,unknown},handle_type("Task"),true,1);x["join"]=fn({handle_type("Task")},unknown,false,0,true);x["ready"]=fn({handle_type("Task")},bool_t);
  }else if(name=="option"){
    x["some"]=fn({unknown},handle_type("Option"));x["none"]=fn({},handle_type("Option"));x["is_some"]=fn({handle_type("Option")},bool_t);x["is_none"]=fn({handle_type("Option")},bool_t);x["value"]=fn({handle_type("Option")},unknown,false,0,true);x["or"]=fn({handle_type("Option"),unknown},unknown);
  }else if(name=="result"){
    x["ok"]=fn({unknown},handle_type("Result"));x["err"]=fn({text_t},handle_type("Result"));x["is_ok"]=fn({handle_type("Result")},bool_t);x["is_err"]=fn({handle_type("Result")},bool_t);x["value"]=fn({handle_type("Result")},unknown,false,0,true);x["error"]=fn({handle_type("Result")},text_t);x["or"]=fn({handle_type("Result"),unknown},unknown);
  }else if(name=="match"){
    x["value"]=fn({unknown,unknown,func},unknown,true,3);x["option"]=fn({handle_type("Option"),func,func},unknown);x["result"]=fn({handle_type("Result"),func,func},unknown);
  }else if(name=="db"){
    x["open"]=fn({text_t},handle_type("Database"),false,0,true);x["set"]=fn({handle_type("Database"),text_t,text_t},none,false,0,true);x["get"]=fn({handle_type("Database"),text_t},handle_type("Option"));x["has"]=fn({handle_type("Database"),text_t},bool_t);x["remove"]=fn({handle_type("Database"),text_t},bool_t,false,0,true);x["keys"]=fn({handle_type("Database")},list_type());x["save"]=fn({handle_type("Database")},none,false,0,true);
  }else if(name=="https"){
    x["get"]=fn({text_t},text_t,false,0,true);x["post"]=fn({text_t,text_t},text_t,false,0,true);x["post_json"]=fn({text_t,text_t},text_t,false,0,true);
  }
  return m;
}

std::shared_ptr<ModuleData> advanced_builtin_module(const std::string& name,Interpreter& vm){
  auto m=std::make_shared<ModuleData>();m->name=name;
  if(name=="function"){
    m->exports["bind"]=callable("function.bind",2,64,[](const std::vector<Value>& a,SourcePos p){auto f=function(a[0],p,"function.bind");std::vector<Value> bound(a.begin()+1,a.end());auto out=std::make_shared<CallableData>();out->name="bound "+f->name;out->min_args=f->min_args>bound.size()?f->min_args-bound.size():0;out->max_args=f->max_args>bound.size()?f->max_args-bound.size():0;out->variadic=f->variadic;out->call=[f,bound](const std::vector<Value>& rest,SourcePos q){auto all=bound;all.insert(all.end(),rest.begin(),rest.end());return f->call(all,q);};return Value(out);},true);
    m->exports["call"]=callable("function.call",1,64,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"function.call");return vm.invoke(a[0],std::vector<Value>(a.begin()+1,a.end()),p);},true);
    m->exports["pipe"]=callable("function.pipe",2,64,[&vm](const std::vector<Value>& a,SourcePos p){Value v=a[0];for(std::size_t i=1;i<a.size();++i){function(a[i],p,"function.pipe");v=vm.invoke(a[i],{v},p);}return v;},true);
    m->exports["partial"]=callable("function.partial",1,64,[](const std::vector<Value>& a,SourcePos p){auto f=function(a[0],p,"function.partial");std::vector<Value> bound(a.begin()+1,a.end());auto out=std::make_shared<CallableData>();out->name="partial "+f->name;out->min_args=f->min_args>bound.size()?f->min_args-bound.size():0;out->max_args=f->max_args>bound.size()?f->max_args-bound.size():0;out->variadic=f->variadic;out->call=[f,bound](const std::vector<Value>& rest,SourcePos q){auto all=bound;all.insert(all.end(),rest.begin(),rest.end());return f->call(all,q);};return Value(out);},true);
    m->exports["reduce"]=callable("function.reduce",2,3,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"function.reduce");auto in=list(a[1],p,"function.reduce");if(in->items.empty()&&a.size()<3)throw Error(p,"function.reduce needs a non-empty List or an initial value.");std::size_t i=0;Value acc;if(a.size()==3)acc=a[2];else{acc=in->items[0];i=1;}for(;i<in->items.size();++i)acc=vm.invoke(a[0],{acc,in->items[i]},p);return acc;},true);
    m->exports["map"]=callable("function.map",2,2,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"function.map");auto in=list(a[1],p,"function.map");auto out=std::make_shared<ListData>();for(auto&v:in->items)out->items.push_back(vm.invoke(a[0],{v},p));return Value(out);});
    m->exports["filter"]=callable("function.filter",2,2,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"function.filter");auto in=list(a[1],p,"function.filter");auto out=std::make_shared<ListData>();for(auto&v:in->items)if(vm.invoke(a[0],{v},p).truth(p))out->items.push_back(v);return Value(out);});
  }else if(name=="async"){
    m->exports["run"]=callable("async.run",1,64,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"async.run");Value f=a[0];std::vector<Value> args(a.begin()+1,a.end());auto task=std::make_shared<TaskData>();task->future=std::async(std::launch::async,[&vm,f,args,p](){std::lock_guard<std::mutex> lock(async_vm_mutex);return vm.invoke(f,args,p);}).share();return handle("Task",task);},true);
    m->exports["await"]=callable("async.await",1,1,[](const std::vector<Value>& a,SourcePos p){auto t=as_handle<TaskData>(a[0],"Task",p,"async.await");try{return t->future.get();}catch(const RuntimeFailure&){throw;}catch(const std::exception& e){throw RuntimeFailure({e.what(),"",p.line,"AsyncError"});}});
    m->exports["ready"]=callable("async.ready",1,1,[](const std::vector<Value>& a,SourcePos p){auto t=as_handle<TaskData>(a[0],"Task",p,"async.ready");return Value(t->future.wait_for(std::chrono::milliseconds(0))==std::future_status::ready);});
  }else if(name=="threading"){
    m->exports["run"]=callable("threading.run",1,64,[&vm](const std::vector<Value>& a,SourcePos p){function(a[0],p,"threading.run");Value f=a[0];std::vector<Value> args(a.begin()+1,a.end());auto task=std::make_shared<TaskData>();task->future=std::async(std::launch::async,[&vm,f,args,p](){std::lock_guard<std::mutex> lock(async_vm_mutex);return vm.invoke(f,args,p);}).share();return handle("Task",task);},true);
    m->exports["join"]=callable("threading.join",1,1,[](const std::vector<Value>& a,SourcePos p){auto t=as_handle<TaskData>(a[0],"Task",p,"threading.join");try{return t->future.get();}catch(const RuntimeFailure&){throw;}catch(const std::exception& e){throw RuntimeFailure({e.what(),"",p.line,"ThreadError"});}});
    m->exports["ready"]=callable("threading.ready",1,1,[](const std::vector<Value>& a,SourcePos p){auto t=as_handle<TaskData>(a[0],"Task",p,"threading.ready");return Value(t->future.wait_for(std::chrono::milliseconds(0))==std::future_status::ready);});
  }else if(name=="option"){
    m->exports["some"]=callable("option.some",1,1,[](const std::vector<Value>& a,SourcePos){auto o=std::make_shared<OptionData>();o->some=true;o->value=a[0];return handle("Option",o);});
    m->exports["none"]=callable("option.none",0,0,[](const std::vector<Value>&,SourcePos){return handle("Option",std::make_shared<OptionData>());});
    m->exports["is_some"]=callable("option.is_some",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(as_handle<OptionData>(a[0],"Option",p,"option.is_some")->some);});
    m->exports["is_none"]=callable("option.is_none",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(!as_handle<OptionData>(a[0],"Option",p,"option.is_none")->some);});
    m->exports["value"]=callable("option.value",1,1,[](const std::vector<Value>& a,SourcePos p){auto o=as_handle<OptionData>(a[0],"Option",p,"option.value");if(!o->some)throw RuntimeFailure({"Option has no value.","",p.line,"OptionError"});return o->value;});
    m->exports["or"]=callable("option.or",2,2,[](const std::vector<Value>& a,SourcePos p){auto o=as_handle<OptionData>(a[0],"Option",p,"option.or");return o->some?o->value:a[1];});
  }else if(name=="result"){
    m->exports["ok"]=callable("result.ok",1,1,[](const std::vector<Value>& a,SourcePos){auto r=std::make_shared<ResultData>();r->ok=true;r->value=a[0];return handle("Result",r);});
    m->exports["err"]=callable("result.err",1,1,[](const std::vector<Value>& a,SourcePos p){auto r=std::make_shared<ResultData>();r->error=text(a[0],p,"result.err");return handle("Result",r);});
    m->exports["is_ok"]=callable("result.is_ok",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(as_handle<ResultData>(a[0],"Result",p,"result.is_ok")->ok);});
    m->exports["is_err"]=callable("result.is_err",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(!as_handle<ResultData>(a[0],"Result",p,"result.is_err")->ok);});
    m->exports["value"]=callable("result.value",1,1,[](const std::vector<Value>& a,SourcePos p){auto r=as_handle<ResultData>(a[0],"Result",p,"result.value");if(!r->ok)throw RuntimeFailure({r->error,"",p.line,"ResultError"});return r->value;});
    m->exports["error"]=callable("result.error",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(as_handle<ResultData>(a[0],"Result",p,"result.error")->error);});
    m->exports["or"]=callable("result.or",2,2,[](const std::vector<Value>& a,SourcePos p){auto r=as_handle<ResultData>(a[0],"Result",p,"result.or");return r->ok?r->value:a[1];});
  }else if(name=="match"){
    m->exports["value"]=callable("match.value",3,64,[&vm](const std::vector<Value>& a,SourcePos p){if((a.size()-1)%2==0)throw Error(p,"match.value needs value, pattern/function pairs, then a fallback function.");Value value=a[0];for(std::size_t i=1;i+1<a.size()-1;i+=2){function(a[i+1],p,"match.value");if(value_equal(value,a[i]))return vm.invoke(a[i+1],{value},p);}function(a.back(),p,"match.value");return vm.invoke(a.back(),{value},p);},true);
    m->exports["option"]=callable("match.option",3,3,[&vm](const std::vector<Value>& a,SourcePos p){auto o=as_handle<OptionData>(a[0],"Option",p,"match.option");function(a[1],p,"match.option");function(a[2],p,"match.option");return o->some?vm.invoke(a[1],{o->value},p):vm.invoke(a[2],{},p);});
    m->exports["result"]=callable("match.result",3,3,[&vm](const std::vector<Value>& a,SourcePos p){auto r=as_handle<ResultData>(a[0],"Result",p,"match.result");function(a[1],p,"match.result");function(a[2],p,"match.result");return r->ok?vm.invoke(a[1],{r->value},p):vm.invoke(a[2],{Value(r->error)},p);});
  }else if(name=="db"){
    m->exports["open"]=callable("db.open",1,1,[](const std::vector<Value>& a,SourcePos p){auto d=std::make_shared<DbData>();d->path=text(a[0],p,"db.open");db_load(*d);return handle("Database",d);});
    m->exports["set"]=callable("db.set",3,3,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.set");auto k=text(a[1],p,"db.set"),v=text(a[2],p,"db.set");for(auto& x:d->values)if(x.first==k){x.second=v;d->dirty=true;db_flush(*d,p);return Value{};}d->values.emplace_back(k,v);d->dirty=true;db_flush(*d,p);return Value{};});
    m->exports["get"]=callable("db.get",2,2,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.get");auto k=text(a[1],p,"db.get");auto o=std::make_shared<OptionData>();for(auto& x:d->values)if(x.first==k){o->some=true;o->value=Value(x.second);break;}return handle("Option",o);});
    m->exports["has"]=callable("db.has",2,2,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.has");auto k=text(a[1],p,"db.has");for(auto& x:d->values)if(x.first==k)return Value(true);return Value(false);});
    m->exports["remove"]=callable("db.remove",2,2,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.remove");auto k=text(a[1],p,"db.remove");auto old=d->values.size();std::erase_if(d->values,[&](const auto& x){return x.first==k;});if(d->values.size()!=old){d->dirty=true;db_flush(*d,p);return Value(true);}return Value(false);});
    m->exports["keys"]=callable("db.keys",1,1,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.keys");auto out=std::make_shared<ListData>();for(auto& x:d->values)out->items.emplace_back(x.first);return Value(out);});
    m->exports["save"]=callable("db.save",1,1,[](const std::vector<Value>& a,SourcePos p){auto d=as_handle<DbData>(a[0],"Database",p,"db.save");db_flush(*d,p);return Value{};});
  }else if(name=="https"){
    m->exports["get"]=callable("https.get",1,1,[](const std::vector<Value>& a,SourcePos p){return Value(curl_request("GET",text(a[0],p,"https.get"),"","text/plain",p));});
    m->exports["post"]=callable("https.post",2,2,[](const std::vector<Value>& a,SourcePos p){return Value(curl_request("POST",text(a[0],p,"https.post"),text(a[1],p,"https.post"),"text/plain; charset=utf-8",p));});
    m->exports["post_json"]=callable("https.post_json",2,2,[](const std::vector<Value>& a,SourcePos p){return Value(curl_request("POST",text(a[0],p,"https.post_json"),text(a[1],p,"https.post_json"),"application/json",p));});
  }
  return m;
}

} // namespace s
