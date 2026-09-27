#include "s/ecosystem.hpp"
#include "s/error.hpp"
#include "s/interpreter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace s {
namespace {

std::shared_ptr<CallableData> callable(std::string name,std::size_t min,std::size_t max,
    std::function<Value(const std::vector<Value>&,SourcePos)> fn,bool variadic=false){
  auto c=std::make_shared<CallableData>();
  c->name=std::move(name);c->min_args=min;c->max_args=max;c->variadic=variadic;c->call=std::move(fn);return c;
}
std::shared_ptr<FunctionSig> signature(std::vector<TypeInfo> params,TypeInfo result,bool variadic=false,std::size_t min=0,bool fallible=false){
  auto s=std::make_shared<FunctionSig>();s->params=std::move(params);s->result=std::move(result);s->variadic=variadic;s->min_args=min;s->fallible=fallible;return s;
}
TypeInfo fn(std::vector<TypeInfo> params,TypeInfo result,bool variadic=false,std::size_t min=0,bool fallible=false){TypeInfo t(TypeKind::Function);t.callable=signature(std::move(params),std::move(result),variadic,min,fallible);return t;}
TypeInfo module_type(const std::string& name){TypeInfo t(TypeKind::Module);t.name=name;return t;}
TypeInfo list_type(TypeInfo e=TypeInfo{}){TypeInfo t(TypeKind::List);t.element=std::make_shared<TypeInfo>(std::move(e));return t;}
TypeInfo map_type(){TypeInfo t(TypeKind::Map);t.key=std::make_shared<TypeInfo>(TypeInfo(TypeKind::Text));t.value=std::make_shared<TypeInfo>(TypeInfo{});return t;}
std::shared_ptr<ModuleData> make_module(const std::string& name){auto m=std::make_shared<ModuleData>();m->name=name;return m;}

double number(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::int64_t>(&v.data()))return static_cast<double>(*x);if(auto x=std::get_if<double>(&v.data()))return *x;throw Error(p,name+" needs a number.");}
std::int64_t integer(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::int64_t>(&v.data()))return *x;throw Error(p,name+" needs Int.");}
std::string text(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::string>(&v.data()))return *x;throw Error(p,name+" needs Text.");}
bool boolean(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<bool>(&v.data()))return *x;throw Error(p,name+" needs Bool.");}
std::shared_ptr<ListData> list_value(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::shared_ptr<ListData>>(&v.data()))return *x;throw Error(p,name+" needs a List.");}
std::shared_ptr<MapData> map_value(const Value& v,SourcePos p,const std::string& name){if(auto x=std::get_if<std::shared_ptr<MapData>>(&v.data()))return *x;throw Error(p,name+" needs a Map.");}

std::string shell_quote(const std::string& s){
#ifdef _WIN32
  std::string out="\"";for(char c:s){if(c=='\"')out+="\\\"";else out+=c;}return out+"\"";
#else
  std::string out="'";for(char c:s){if(c=='\'')out+="'\\''";else out+=c;}return out+"'";
#endif
}
int normalized_system(const std::string& command){int code=std::system(command.c_str());
#ifndef _WIN32
  if(code!=-1&&WIFEXITED(code))return WEXITSTATUS(code);
#endif
  return code;
}
std::string process_output(const std::string& command,SourcePos p){
#ifdef _WIN32
  FILE* pipe=_popen(command.c_str(),"r");
#else
  FILE* pipe=popen(command.c_str(),"r");
#endif
  if(!pipe)throw Error(p,"Could not start process.");std::string out;char buf[4096];while(std::fgets(buf,sizeof(buf),pipe))out+=buf;
#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif
  return out;
}

struct NetResponse{std::int64_t status=0;std::string type;std::string body;};
NetResponse curl_request(const std::string& method,const std::string& url,const std::string& body,const std::string& content_type,SourcePos p){
  if(url.rfind("http://",0)!=0&&url.rfind("https://",0)!=0)throw Error(p,"net needs an http:// or https:// URL.");
  const std::string marker="__SE_NET_META_7C88__";
  std::string cmd="curl -sS -L --max-time 30 -X "+shell_quote(method)+" -H "+shell_quote("Content-Type: "+content_type);
  if(!body.empty()||method=="POST"||method=="PUT"||method=="PATCH")cmd+=" --data-binary "+shell_quote(body);
  cmd+=" -w "+shell_quote("\\n"+marker+"%{http_code}|%{content_type}")+" "+shell_quote(url)+" 2>&1";
  auto out=process_output(cmd,p);auto pos=out.rfind("\n"+marker);if(pos==std::string::npos)throw Error(p,"Network request failed. Make sure curl is installed and the URL is reachable.");
  NetResponse r;r.body=out.substr(0,pos);auto meta=out.substr(pos+1+marker.size());auto bar=meta.find('|');try{r.status=std::stoll(meta.substr(0,bar));}catch(...){r.status=0;}if(bar!=std::string::npos)r.type=meta.substr(bar+1);while(!r.type.empty()&&(r.type.back()=='\n'||r.type.back()=='\r'))r.type.pop_back();return r;
}
Value response_map(const NetResponse& r){auto m=std::make_shared<MapData>();m->items.emplace_back("status",Value(r.status));m->items.emplace_back("type",Value(r.type));m->items.emplace_back("body",Value(r.body));return Value(m);}

std::vector<double> numbers(const Value& value,SourcePos p,const std::string& name){auto l=list_value(value,p,name);if(l->items.empty())throw Error(p,name+" needs a non-empty List.");std::vector<double> out;for(auto& v:l->items)out.push_back(number(v,p,name));return out;}
std::int64_t factorial_checked(std::int64_t n,SourcePos p){if(n<0)throw Error(p,"math.factorial needs a non-negative Int.");if(n>20)throw Error(p,"math.factorial is limited to 20 for Int safety.");std::int64_t r=1;for(std::int64_t i=2;i<=n;++i)r*=i;return r;}


std::string decimal_text(long double v,int precision=18){
  std::ostringstream o;o<<std::setprecision(std::clamp(precision,1,36))<<std::fixed<<v;
  auto s=o.str();while(s.size()>1&&s.back()=='0')s.pop_back();if(!s.empty()&&s.back()=='.')s.pop_back();if(s=="-0")s="0";return s;
}
long double decimal_number(const Value& v,SourcePos p,const std::string& name){
  auto s=text(v,p,name);try{std::size_t used=0;auto x=std::stold(s,&used);if(used!=s.size())throw std::runtime_error("bad");return x;}catch(...){throw Error(p,name+" needs a decimal Text value.");}
}
std::string csv_escape_field(const std::string& s){
  if(s.find_first_of(",\"\r\n")==std::string::npos)return s;std::string out="\"";for(char ch:s){if(ch=='\"')out+="\"\"";else out+=ch;}return out+"\"";
}
std::shared_ptr<ListData> csv_parse_rows(const std::string& input){
  auto rows=std::make_shared<ListData>();auto row=std::make_shared<ListData>();std::string field;bool quoted=false;
  auto push_field=[&](){row->items.emplace_back(field);field.clear();};
  auto push_row=[&](){push_field();rows->items.emplace_back(row);row=std::make_shared<ListData>();};
  for(std::size_t i=0;i<input.size();++i){char ch=input[i];if(quoted){if(ch=='\"'){if(i+1<input.size()&&input[i+1]=='\"'){field+='\"';++i;}else quoted=false;}else field+=ch;continue;}if(ch=='\"'&&field.empty()){quoted=true;continue;}if(ch==','){push_field();continue;}if(ch=='\n'){if(!field.empty()||!row->items.empty())push_row();continue;}if(ch=='\r'){if(i+1<input.size()&&input[i+1]=='\n')continue;if(!field.empty()||!row->items.empty())push_row();continue;}field+=ch;}
  if(!field.empty()||!row->items.empty())push_row();return rows;
}
std::string csv_stringify_rows(const Value& value,SourcePos p,const std::string& name){
  auto rows=list_value(value,p,name);std::ostringstream out;
  for(std::size_t r=0;r<rows->items.size();++r){auto row=list_value(rows->items[r],p,name);for(std::size_t i=0;i<row->items.size();++i){if(i)out<<',';out<<csv_escape_field(row->items[i].text());}if(r+1<rows->items.size())out<<'\n';}
  return out.str();
}
std::string wildcard_regex(const std::string& pattern){
  std::string out="^";for(std::size_t i=0;i<pattern.size();++i){char ch=pattern[i];if(ch=='*'){if(i+1<pattern.size()&&pattern[i+1]=='*'){out+=".*";++i;}else out+="[^/\\\\]*";}else if(ch=='?')out+=".";else if(std::string(".^$|()[]{}+\\").find(ch)!=std::string::npos){out+='\\';out+=ch;}else if(ch=='\\')out+="[/\\\\]";else if(ch=='/')out+="[/\\\\]";else out+=ch;}return out+"$";
}
std::string sha256_text(const std::string& input,SourcePos p){
  auto stamp=std::chrono::high_resolution_clock::now().time_since_epoch().count();
  auto file=std::filesystem::temp_directory_path()/("se-sha256-"+std::to_string(stamp)+".tmp");
  {std::ofstream f(file,std::ios::binary|std::ios::trunc);if(!f)throw Error(p,"hash.sha256 could not create a temporary file.");f<<input;}
#ifdef _WIN32
  auto output=process_output("certutil -hashfile "+shell_quote(file.string())+" SHA256",p);
#else
  auto output=process_output("(command -v sha256sum >/dev/null 2>&1 && sha256sum "+shell_quote(file.string())+") || shasum -a 256 "+shell_quote(file.string()),p);
#endif
  std::error_code ec;std::filesystem::remove(file,ec);std::smatch m;std::regex re("[0-9A-Fa-f]{64}");
  if(!std::regex_search(output,m,re))throw Error(p,"hash.sha256 could not obtain a SHA-256 digest from the host tools.");auto s=m.str();std::transform(s.begin(),s.end(),s.begin(),[](unsigned char ch){return static_cast<char>(std::tolower(ch));});return s;
}
std::tm utc_tm(std::time_t t){
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm,&t);
#else
  gmtime_r(&t,&tm);
#endif
  return tm;
}
std::string iso_utc(std::time_t t){auto tm=utc_tm(t);std::ostringstream o;o<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%SZ");return o.str();}
struct QueueState{std::deque<Value> items;};
struct SqliteState{std::filesystem::path path;};
template<class T> Value native_handle(const std::string& tag,std::shared_ptr<T> value){auto h=std::make_shared<NativeHandleData>();h->tag=tag;h->resource=std::move(value);return Value(h);}
template<class T> std::shared_ptr<T> native_as(const Value& value,const std::string& tag,SourcePos p,const std::string& name){auto h=std::get_if<std::shared_ptr<NativeHandleData>>(&value.data());if(!h||!(*h)||(*h)->tag!=tag)throw Error(p,name+" needs "+tag+".");return std::static_pointer_cast<T>((*h)->resource);}
int& se_log_level(){static int level=1;return level;}
int parse_log_level(std::string level){level=lower_ascii(level);if(level=="debug")return 0;if(level=="info")return 1;if(level=="warn"||level=="warning")return 2;if(level=="error")return 3;return 1;}
void emit_log(int level,const std::string& label,const std::string& msg){if(level<se_log_level())return;auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::clog<<"["<<iso_utc(now)<<"] ["<<label<<"] "<<msg<<'\n';}

std::string html_escape(const std::string& s){std::string o;for(char c:s){if(c=='&')o+="&amp;";else if(c=='<')o+="&lt;";else if(c=='>')o+="&gt;";else if(c=='\"')o+="&quot;";else o+=c;}return o;}
std::string js_escape(const std::string& s){std::string o;for(char c:s){if(c=='\\')o+="\\\\";else if(c=='\"')o+="\\\"";else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;}
struct GameScene{int width=800;int height=600;std::string title="SE Game";std::string background="#111";std::vector<std::string> draw;std::vector<std::string> scripts;};
std::unordered_map<std::int64_t,GameScene>& scenes(){static std::unordered_map<std::int64_t,GameScene> v;return v;}
std::int64_t& next_scene(){static std::int64_t v=1;return v;}
GameScene& scene_for(std::int64_t id,SourcePos p){auto i=scenes().find(id);if(i==scenes().end())throw Error(p,"Unknown game scene "+std::to_string(id)+".");return i->second;}
std::string scene_html(std::int64_t id,SourcePos p){auto& s=scene_for(id,p);std::ostringstream o;o<<"<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>"<<html_escape(s.title)<<"</title><style>html,body{margin:0;width:100%;height:100%;background:#000;display:grid;place-items:center;overflow:hidden}canvas{max-width:100vw;max-height:100vh}</style></head><body><canvas id=\"game\" width=\""<<s.width<<"\" height=\""<<s.height<<"\"></canvas><script>const canvas=document.getElementById('game');const ctx=canvas.getContext('2d');const SEGame={keys:new Set(),mouse:{x:0,y:0,down:false},canvas,ctx};addEventListener('keydown',e=>SEGame.keys.add(e.key));addEventListener('keyup',e=>SEGame.keys.delete(e.key));canvas.addEventListener('mousemove',e=>{const r=canvas.getBoundingClientRect();SEGame.mouse.x=(e.clientX-r.left)*canvas.width/r.width;SEGame.mouse.y=(e.clientY-r.top)*canvas.height/r.height});canvas.addEventListener('mousedown',()=>SEGame.mouse.down=true);addEventListener('mouseup',()=>SEGame.mouse.down=false);ctx.fillStyle=\""<<js_escape(s.background)<<"\";ctx.fillRect(0,0,canvas.width,canvas.height);";for(auto& d:s.draw)o<<d;for(auto& q:s.scripts)o<<q;o<<"</script></body></html>";return o.str();}
void open_file(const std::filesystem::path& path){
#ifdef _WIN32
  normalized_system("cmd /c start \"\" "+shell_quote(path.string()));
#elif __APPLE__
  normalized_system("open "+shell_quote(path.string()));
#else
  normalized_system("xdg-open "+shell_quote(path.string())+" >/dev/null 2>&1 &");
#endif
}

} // namespace

bool is_ecosystem_builtin(const std::string& name){static const std::set<std::string> names={
  "math","data","net","node","next","game",
  "statistics","regex","re","base64","uuid","iter","itertools","copy","operator",
  "decimal","csv","datetime","hash","hashlib","pickle","args","argparse","log","logging",
  "shutil","glob","zip","zipfile","subprocess","socket","queue","sqlite","sqlite3",
  "functools","enum","typing"
};return names.contains(name);}

TypeInfo ecosystem_builtin_type(const std::string& name){
  auto m=module_type(name);auto& x=m.members;TypeInfo unknown,none(TypeKind::None),num(TypeKind::Num),integer_t(TypeKind::Int),text_t(TypeKind::Text),bool_t(TypeKind::Bool),list_t=list_type(),map_t=map_type(),func_t(TypeKind::Function);
  auto handle_t=[](const std::string& n){TypeInfo t(TypeKind::NativeHandle);t.name=n;return t;};
  if(name=="math"){
    x["pi"]=num;x["e"]=num;x["tau"]=num;x["inf"]=num;
    for(auto n:{"sqrt","cbrt","abs","floor","ceil","round","trunc","sin","cos","tan","asin","acos","atan","sinh","cosh","tanh","asinh","acosh","atanh","exp","exp2","expm1","log","log10","log2","log1p","degrees","radians","gamma","lgamma","erf","erfc"})x[n]=fn({num},num);
    for(auto n:{"atan2","pow","fmod","remainder","copysign","nextafter"})x[n]=fn({num,num},num);
    x["hypot"]=fn({num,num},num,true,2);x["min"]=fn({num,num},num,true,2);x["max"]=fn({num,num},num,true,2);x["clamp"]=fn({num,num,num},num);x["lerp"]=fn({num,num,num},num);x["map_range"]=fn({num,num,num,num,num},num);x["sign"]=fn({num},integer_t);x["isfinite"]=fn({num},bool_t);x["isinf"]=fn({num},bool_t);x["isnan"]=fn({num},bool_t);
    x["gcd"]=fn({integer_t,integer_t},integer_t,true,2);x["lcm"]=fn({integer_t,integer_t},integer_t,true,2);x["factorial"]=fn({integer_t},integer_t);x["comb"]=fn({integer_t,integer_t},integer_t);x["perm"]=fn({integer_t,integer_t},integer_t);x["sum"]=fn({list_t},num);x["mean"]=fn({list_t},num);x["median"]=fn({list_t},num);x["variance"]=fn({list_t},num);x["stddev"]=fn({list_t},num);
  }else if(name=="statistics"){
    x["mean"]=fn({list_t},num);x["median"]=fn({list_t},num);x["variance"]=fn({list_t},num);x["pvariance"]=fn({list_t},num);x["stdev"]=fn({list_t},num);x["pstdev"]=fn({list_t},num);
  }else if(name=="regex"||name=="re"){
    x["match"]=fn({text_t,text_t},bool_t);x["search"]=fn({text_t,text_t},bool_t);x["replace"]=fn({text_t,text_t,text_t},text_t);x["split"]=fn({text_t,text_t},list_type(text_t));
  }else if(name=="base64"){
    x["encode"]=fn({text_t},text_t);x["decode"]=fn({text_t},text_t,false,0,true);
  }else if(name=="uuid"){
    x["v4"]=fn({},text_t);x["valid"]=fn({text_t},bool_t);
  }else if(name=="iter"||name=="itertools"){
    x["range"]=fn({integer_t,integer_t},list_type(integer_t),true,1);x["enumerate"]=fn({list_t},list_t);x["zip"]=fn({list_t,list_t},list_t);x["product"]=fn({list_t,list_t},list_t);x["permutations"]=fn({list_t},list_t,true,1);x["combinations"]=fn({list_t,integer_t},list_t);
  }else if(name=="copy"){
    x["shallow"]=fn({unknown},unknown);x["deep"]=fn({unknown},unknown);
  }else if(name=="operator"){
    for(auto n:{"add","sub","mul","div","mod","eq","ne","lt","le","gt","ge"})x[n]=fn({unknown,unknown},unknown);
  }else if(name=="decimal"){
    x["parse"]=fn({text_t},text_t);x["add"]=fn({text_t,text_t},text_t);x["sub"]=fn({text_t,text_t},text_t);x["mul"]=fn({text_t,text_t},text_t);x["div"]=fn({text_t,text_t},text_t,true,2);x["quantize"]=fn({text_t,integer_t},text_t);
  }else if(name=="csv"){
    x["parse"]=fn({text_t},list_t);x["stringify"]=fn({list_t},text_t);x["read"]=fn({text_t},list_t,false,0,true);x["write"]=fn({text_t,list_t},none,false,0,true);
  }else if(name=="datetime"){
    x["now"]=fn({},text_t);x["timestamp"]=fn({},integer_t);x["from_timestamp"]=fn({integer_t},text_t);x["format"]=fn({integer_t,text_t},text_t);x["add_seconds"]=fn({integer_t,integer_t},integer_t);
  }else if(name=="hash"||name=="hashlib"){
    x["sha256"]=fn({text_t},text_t,false,0,true);x["file_sha256"]=fn({text_t},text_t,false,0,true);
  }else if(name=="pickle"){
    x["dumps"]=fn({unknown},text_t);x["loads"]=fn({text_t},unknown,false,0,true);
  }else if(name=="args"||name=="argparse"){
    x["parse"]=fn({list_t},map_t);x["get"]=fn({map_t,text_t},unknown,true,2);x["flag"]=fn({map_t,text_t},bool_t);
  }else if(name=="log"||name=="logging"){
    x["level"]=fn({text_t},none);x["debug"]=fn({text_t},none);x["info"]=fn({text_t},none);x["warn"]=fn({text_t},none);x["error"]=fn({text_t},none);
  }else if(name=="shutil"){
    x["copy"]=fn({text_t,text_t},none,false,0,true);x["move"]=fn({text_t,text_t},none,false,0,true);x["copytree"]=fn({text_t,text_t},none,false,0,true);x["remove"]=fn({text_t},none,false,0,true);x["mkdir"]=fn({text_t},none,false,0,true);
  }else if(name=="glob"){
    x["match"]=fn({text_t,text_t},bool_t);x["find"]=fn({text_t},list_type(text_t),false,0,true);
  }else if(name=="zip"||name=="zipfile"){
    x["create"]=fn({text_t,list_t},none,false,0,true);x["extract"]=fn({text_t,text_t},none,false,0,true);x["list"]=fn({text_t},list_type(text_t),false,0,true);
  }else if(name=="subprocess"){
    x["run"]=fn({text_t},integer_t,false,0,true);x["output"]=fn({text_t},text_t,false,0,true);
  }else if(name=="socket"){
    x["resolve"]=fn({text_t},text_t,false,0,true);x["tcp"]=fn({text_t,integer_t,text_t},text_t,false,0,true);
  }else if(name=="queue"){
    x["new"]=fn({},handle_t("Queue"));x["put"]=fn({handle_t("Queue"),unknown},none);x["get"]=fn({handle_t("Queue")},unknown,false,0,true);x["empty"]=fn({handle_t("Queue")},bool_t);x["size"]=fn({handle_t("Queue")},integer_t);
  }else if(name=="sqlite"||name=="sqlite3"){
    x["open"]=fn({text_t},handle_t("SQLite"));x["exec"]=fn({handle_t("SQLite"),text_t},integer_t,false,0,true);x["query"]=fn({handle_t("SQLite"),text_t},list_t,false,0,true);
  }else if(name=="functools"){
    x["partial"]=fn({func_t,unknown},func_t,true,1);x["reduce"]=fn({func_t,list_t},unknown,true,2);x["map"]=fn({func_t,list_t},list_t);x["filter"]=fn({func_t,list_t},list_t);
  }else if(name=="enum"){
    x["make"]=fn({list_type(text_t)},map_t);x["name"]=fn({map_t,integer_t},text_t,false,0,true);x["value"]=fn({map_t,text_t},integer_t,false,0,true);x["has"]=fn({map_t,text_t},bool_t);
  }else if(name=="typing"){
    x["type_of"]=fn({unknown},text_t);x["is"]=fn({unknown,text_t},bool_t);x["cast"]=fn({unknown,text_t},unknown);
  }else if(name=="data"){
    x["append"]=fn({list_t,unknown},none);x["extend"]=fn({list_t,list_t},none);x["insert"]=fn({list_t,integer_t,unknown},none);x["pop"]=fn({list_t},unknown,true,1);x["clear"]=fn({unknown},none);x["copy"]=fn({unknown},unknown);x["get"]=fn({map_t,text_t},unknown,true,2);x["set"]=fn({map_t,text_t,unknown},none);x["update"]=fn({map_t,map_t},none);x["delete"]=fn({map_t,text_t},bool_t);x["has"]=fn({unknown,unknown},bool_t);x["keys"]=fn({map_t},list_type(text_t));x["values"]=fn({map_t},list_t);x["items"]=fn({map_t},list_t);
  }else if(name=="net"){
    x["get"]=fn({text_t},text_t,false,0,true);x["post"]=fn({text_t,text_t},text_t,false,0,true);x["post_json"]=fn({text_t,text_t},text_t,false,0,true);x["request"]=fn({text_t,text_t,text_t},unknown,false,0,true);x["download"]=fn({text_t,text_t},none,false,0,true);
  }else if(name=="node"){
    x["version"]=fn({},text_t,false,0,true);x["run"]=fn({text_t},integer_t,false,0,true);x["output"]=fn({text_t},text_t,false,0,true);x["eval"]=fn({text_t},text_t,false,0,true);x["npm"]=fn({text_t},integer_t,false,0,true);x["npx"]=fn({text_t},integer_t,false,0,true);
  }else if(name=="next"){
    x["create"]=fn({text_t},integer_t,false,0,true);x["dev"]=fn({text_t},integer_t,false,0,true);x["build"]=fn({text_t},integer_t,false,0,true);x["start"]=fn({text_t},integer_t,false,0,true);x["lint"]=fn({text_t},integer_t,false,0,true);
  }else if(name=="game"){
    x["new"]=fn({integer_t,integer_t,text_t},integer_t);x["background"]=fn({integer_t,text_t},none);x["clear"]=fn({integer_t},none);x["rect"]=fn({integer_t,num,num,num,num,text_t,bool_t},none);x["circle"]=fn({integer_t,num,num,num,text_t,bool_t},none);x["line"]=fn({integer_t,num,num,num,num,text_t,num},none);x["text"]=fn({integer_t,text_t,num,num,num,text_t},none);x["script"]=fn({integer_t,text_t},none);x["html"]=fn({integer_t},text_t);x["save"]=fn({integer_t,text_t},none,false,0,true);x["show"]=fn({integer_t},none,false,0,true);
  }
  return m;
}

std::shared_ptr<ModuleData> ecosystem_builtin_module(const std::string& name,Interpreter& vm){
  (void)vm;auto m=make_module(name);
  if(name=="math"){
    m->exports["pi"]=Value(3.14159265358979323846);m->exports["e"]=Value(2.71828182845904523536);m->exports["tau"]=Value(6.28318530717958647692);m->exports["inf"]=Value(std::numeric_limits<double>::infinity());
#define SE_MATH1(NAME,FN) m->exports[#NAME]=callable("math." #NAME,1,1,[](const std::vector<Value>&a,SourcePos p){return Value(FN(number(a[0],p,"math." #NAME)));})
    SE_MATH1(cbrt,std::cbrt);SE_MATH1(abs,std::fabs);SE_MATH1(floor,std::floor);SE_MATH1(ceil,std::ceil);SE_MATH1(round,std::round);SE_MATH1(trunc,std::trunc);SE_MATH1(sin,std::sin);SE_MATH1(cos,std::cos);SE_MATH1(tan,std::tan);SE_MATH1(asin,std::asin);SE_MATH1(acos,std::acos);SE_MATH1(atan,std::atan);SE_MATH1(sinh,std::sinh);SE_MATH1(cosh,std::cosh);SE_MATH1(tanh,std::tanh);SE_MATH1(asinh,std::asinh);SE_MATH1(acosh,std::acosh);SE_MATH1(atanh,std::atanh);SE_MATH1(exp,std::exp);SE_MATH1(exp2,std::exp2);SE_MATH1(expm1,std::expm1);SE_MATH1(log,std::log);SE_MATH1(log10,std::log10);SE_MATH1(log2,std::log2);SE_MATH1(log1p,std::log1p);SE_MATH1(gamma,std::tgamma);SE_MATH1(lgamma,std::lgamma);SE_MATH1(erf,std::erf);SE_MATH1(erfc,std::erfc);
#undef SE_MATH1
    m->exports["sqrt"]=callable("math.sqrt",1,1,[](const std::vector<Value>&a,SourcePos p){auto v=number(a[0],p,"math.sqrt");if(v<0)throw Error(p,"math.sqrt needs a non-negative number.");return Value(std::sqrt(v));});
    m->exports["degrees"]=callable("math.degrees",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(number(a[0],p,"math.degrees")*180.0/3.14159265358979323846);});m->exports["radians"]=callable("math.radians",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(number(a[0],p,"math.radians")*3.14159265358979323846/180.0);});
    auto binary=[&](const std::string& n,auto op){m->exports[n]=callable("math."+n,2,2,[n,op](const std::vector<Value>&a,SourcePos p){return Value(op(number(a[0],p,"math."+n),number(a[1],p,"math."+n)));});};
    binary("pow",[](double a,double b){return std::pow(a,b);});binary("atan2",[](double a,double b){return std::atan2(a,b);});binary("fmod",[](double a,double b){return std::fmod(a,b);});binary("remainder",[](double a,double b){return std::remainder(a,b);});binary("copysign",[](double a,double b){return std::copysign(a,b);});binary("nextafter",[](double a,double b){return std::nextafter(a,b);});
    m->exports["hypot"]=callable("math.hypot",2,64,[](const std::vector<Value>&a,SourcePos p){double s=0;for(auto& v:a){auto n=number(v,p,"math.hypot");s+=n*n;}return Value(std::sqrt(s));},true);m->exports["min"]=callable("math.min",2,64,[](const std::vector<Value>&a,SourcePos p){double r=number(a[0],p,"math.min");for(std::size_t i=1;i<a.size();++i)r=std::min(r,number(a[i],p,"math.min"));return Value(r);},true);m->exports["max"]=callable("math.max",2,64,[](const std::vector<Value>&a,SourcePos p){double r=number(a[0],p,"math.max");for(std::size_t i=1;i<a.size();++i)r=std::max(r,number(a[i],p,"math.max"));return Value(r);},true);
    m->exports["clamp"]=callable("math.clamp",3,3,[](const std::vector<Value>&a,SourcePos p){auto v=number(a[0],p,"math.clamp");auto lo=number(a[1],p,"math.clamp");auto hi=number(a[2],p,"math.clamp");if(lo>hi)throw Error(p,"math.clamp needs min <= max.");return Value(std::clamp(v,lo,hi));});m->exports["lerp"]=callable("math.lerp",3,3,[](const std::vector<Value>&a,SourcePos p){auto x=number(a[0],p,"math.lerp"),y=number(a[1],p,"math.lerp"),t=number(a[2],p,"math.lerp");return Value(x+(y-x)*t);});m->exports["map_range"]=callable("math.map_range",5,5,[](const std::vector<Value>&a,SourcePos p){auto x=number(a[0],p,"math.map_range"),a0=number(a[1],p,"math.map_range"),a1=number(a[2],p,"math.map_range"),b0=number(a[3],p,"math.map_range"),b1=number(a[4],p,"math.map_range");if(a0==a1)throw Error(p,"math.map_range input range cannot have zero width.");return Value(b0+(x-a0)*(b1-b0)/(a1-a0));});
    m->exports["sign"]=callable("math.sign",1,1,[](const std::vector<Value>&a,SourcePos p){auto v=number(a[0],p,"math.sign");return Value(static_cast<std::int64_t>((v>0)-(v<0)));});m->exports["isfinite"]=callable("math.isfinite",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::isfinite(number(a[0],p,"math.isfinite")));});m->exports["isinf"]=callable("math.isinf",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::isinf(number(a[0],p,"math.isinf")));});m->exports["isnan"]=callable("math.isnan",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::isnan(number(a[0],p,"math.isnan")));});
    m->exports["gcd"]=callable("math.gcd",2,64,[](const std::vector<Value>&a,SourcePos p){auto r=integer(a[0],p,"math.gcd");for(std::size_t i=1;i<a.size();++i)r=std::gcd(r,integer(a[i],p,"math.gcd"));return Value(r);},true);m->exports["lcm"]=callable("math.lcm",2,64,[](const std::vector<Value>&a,SourcePos p){auto r=integer(a[0],p,"math.lcm");for(std::size_t i=1;i<a.size();++i)r=std::lcm(r,integer(a[i],p,"math.lcm"));return Value(r);},true);m->exports["factorial"]=callable("math.factorial",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(factorial_checked(integer(a[0],p,"math.factorial"),p));});m->exports["comb"]=callable("math.comb",2,2,[](const std::vector<Value>&a,SourcePos p){auto n=integer(a[0],p,"math.comb");auto k=integer(a[1],p,"math.comb");if(k<0||n<0||k>n)throw Error(p,"math.comb needs 0 <= k <= n.");k=std::min(k,n-k);std::int64_t r=1;for(std::int64_t i=1;i<=k;++i)r=r*(n-k+i)/i;return Value(r);});m->exports["perm"]=callable("math.perm",2,2,[](const std::vector<Value>&a,SourcePos p){auto n=integer(a[0],p,"math.perm");auto k=integer(a[1],p,"math.perm");if(k<0||n<0||k>n||n>20)throw Error(p,"math.perm needs 0 <= k <= n <= 20.");std::int64_t r=1;for(std::int64_t i=0;i<k;++i)r*=n-i;return Value(r);});
    auto stat=[&](const std::string& n,auto op){m->exports[n]=callable("math."+n,1,1,[n,op](const std::vector<Value>&a,SourcePos p){return Value(op(numbers(a[0],p,"math."+n)));});};stat("sum",[](std::vector<double> v){return std::accumulate(v.begin(),v.end(),0.0);});stat("mean",[](std::vector<double> v){return std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());});stat("median",[](std::vector<double> v){std::sort(v.begin(),v.end());auto n=v.size();return n%2?v[n/2]:(v[n/2-1]+v[n/2])/2.0;});stat("variance",[](std::vector<double> v){auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return s/static_cast<double>(v.size());});stat("stddev",[](std::vector<double> v){auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return std::sqrt(s/static_cast<double>(v.size()));});
  }else if(name=="statistics"){
    auto stat=[&](const std::string& n,auto op){m->exports[n]=callable("statistics."+n,1,1,[n,op](const std::vector<Value>&a,SourcePos p){return Value(op(numbers(a[0],p,"statistics."+n)));});};
    stat("mean",[](std::vector<double> v){return std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());});
    stat("median",[](std::vector<double> v){std::sort(v.begin(),v.end());auto n=v.size();return n%2?v[n/2]:(v[n/2-1]+v[n/2])/2.0;});
    stat("pvariance",[](std::vector<double> v){auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return s/static_cast<double>(v.size());});
    stat("pstdev",[](std::vector<double> v){auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return std::sqrt(s/static_cast<double>(v.size()));});
    stat("variance",[](std::vector<double> v){if(v.size()<2)throw std::runtime_error("statistics.variance needs at least 2 values.");auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return s/static_cast<double>(v.size()-1);});
    stat("stdev",[](std::vector<double> v){if(v.size()<2)throw std::runtime_error("statistics.stdev needs at least 2 values.");auto mean=std::accumulate(v.begin(),v.end(),0.0)/static_cast<double>(v.size());double s=0;for(auto q:v){auto d=q-mean;s+=d*d;}return std::sqrt(s/static_cast<double>(v.size()-1));});
  }else if(name=="regex"){
    m->exports["match"]=callable("regex.match",2,2,[](const std::vector<Value>&a,SourcePos p){try{return Value(std::regex_match(text(a[1],p,"regex.match"),std::regex(text(a[0],p,"regex.match"))));}catch(const std::regex_error&e){throw Error(p,std::string("Invalid regex: ")+e.what());}});
    m->exports["search"]=callable("regex.search",2,2,[](const std::vector<Value>&a,SourcePos p){try{return Value(std::regex_search(text(a[1],p,"regex.search"),std::regex(text(a[0],p,"regex.search"))));}catch(const std::regex_error&e){throw Error(p,std::string("Invalid regex: ")+e.what());}});
    m->exports["replace"]=callable("regex.replace",3,3,[](const std::vector<Value>&a,SourcePos p){try{return Value(std::regex_replace(text(a[1],p,"regex.replace"),std::regex(text(a[0],p,"regex.replace")),text(a[2],p,"regex.replace")));}catch(const std::regex_error&e){throw Error(p,std::string("Invalid regex: ")+e.what());}});
    m->exports["split"]=callable("regex.split",2,2,[](const std::vector<Value>&a,SourcePos p){try{std::regex re(text(a[0],p,"regex.split"));std::string s=text(a[1],p,"regex.split");std::sregex_token_iterator it(s.begin(),s.end(),re,-1),end;auto out=std::make_shared<ListData>();for(;it!=end;++it)out->items.emplace_back(it->str());return Value(out);}catch(const std::regex_error&e){throw Error(p,std::string("Invalid regex: ")+e.what());}});
  }else if(name=="base64"){
    static const std::string chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    m->exports["encode"]=callable("base64.encode",1,1,[](const std::vector<Value>&a,SourcePos p){auto s=text(a[0],p,"base64.encode");std::string out;int val=0,valb=-6;for(unsigned char c:s){val=(val<<8)+c;valb+=8;while(valb>=0){out.push_back(chars[(val>>valb)&0x3F]);valb-=6;}}if(valb>-6)out.push_back(chars[((val<<8)>>(valb+8))&0x3F]);while(out.size()%4)out.push_back('=');return Value(out);});
    m->exports["decode"]=callable("base64.decode",1,1,[](const std::vector<Value>&a,SourcePos p){auto s=text(a[0],p,"base64.decode");std::vector<int> t(256,-1);for(int i=0;i<64;i++)t[static_cast<unsigned char>(chars[i])]=i;std::string out;int val=0,valb=-8;for(unsigned char c:s){if(std::isspace(c))continue;if(c=='=')break;if(t[c]==-1)throw Error(p,"base64.decode received invalid Base64.");val=(val<<6)+t[c];valb+=6;if(valb>=0){out.push_back(char((val>>valb)&0xFF));valb-=8;}}return Value(out);});
  }else if(name=="uuid"){
    m->exports["v4"]=callable("uuid.v4",0,0,[](const std::vector<Value>&,SourcePos){std::random_device rd;std::mt19937_64 g(rd());std::uniform_int_distribution<unsigned long long>d;auto a=d(g),b=d(g);unsigned char x[16];for(int i=0;i<8;++i)x[i]=static_cast<unsigned char>((a>>(56-8*i))&255);for(int i=0;i<8;++i)x[8+i]=static_cast<unsigned char>((b>>(56-8*i))&255);x[6]=(x[6]&0x0f)|0x40;x[8]=(x[8]&0x3f)|0x80;std::ostringstream o;o<<std::hex<<std::setfill('0');for(int i=0;i<16;++i){o<<std::setw(2)<<static_cast<int>(x[i]);if(i==3||i==5||i==7||i==9)o<<'-';}return Value(o.str());});
    m->exports["valid"]=callable("uuid.valid",1,1,[](const std::vector<Value>&a,SourcePos p){auto s=text(a[0],p,"uuid.valid");static const std::regex re("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}$");return Value(std::regex_match(s,re));});
  }else if(name=="iter"){
    m->exports["range"]=callable("iter.range",1,3,[](const std::vector<Value>&a,SourcePos p){std::int64_t start=0,stop=0,step=1;if(a.size()==1)stop=integer(a[0],p,"iter.range");else{start=integer(a[0],p,"iter.range");stop=integer(a[1],p,"iter.range");if(a.size()==3)step=integer(a[2],p,"iter.range");}if(step==0)throw Error(p,"iter.range step cannot be zero.");auto out=std::make_shared<ListData>();if(step>0)for(auto i=start;i<stop;i+=step)out->items.emplace_back(i);else for(auto i=start;i>stop;i+=step)out->items.emplace_back(i);return Value(out);},true);
    m->exports["enumerate"]=callable("iter.enumerate",1,1,[](const std::vector<Value>&a,SourcePos p){auto l=list_value(a[0],p,"iter.enumerate");auto out=std::make_shared<ListData>();for(std::size_t i=0;i<l->items.size();++i){auto pair=std::make_shared<ListData>();pair->items.emplace_back(static_cast<std::int64_t>(i));pair->items.push_back(l->items[i]);out->items.emplace_back(pair);}return Value(out);});
    m->exports["zip"]=callable("iter.zip",2,2,[](const std::vector<Value>&a,SourcePos p){auto x=list_value(a[0],p,"iter.zip"),y=list_value(a[1],p,"iter.zip");auto out=std::make_shared<ListData>();auto n=std::min(x->items.size(),y->items.size());for(std::size_t i=0;i<n;++i){auto pair=std::make_shared<ListData>();pair->items={x->items[i],y->items[i]};out->items.emplace_back(pair);}return Value(out);});
    m->exports["product"]=callable("iter.product",2,2,[](const std::vector<Value>&a,SourcePos p){auto x=list_value(a[0],p,"iter.product"),y=list_value(a[1],p,"iter.product");auto out=std::make_shared<ListData>();for(auto& i:x->items)for(auto& j:y->items){auto pair=std::make_shared<ListData>();pair->items={i,j};out->items.emplace_back(pair);}return Value(out);});
    m->exports["permutations"]=callable("iter.permutations",1,2,[](const std::vector<Value>&a,SourcePos p){auto l=list_value(a[0],p,"iter.permutations");auto r=a.size()==2?integer(a[1],p,"iter.permutations"):static_cast<std::int64_t>(l->items.size());if(r<0||static_cast<std::size_t>(r)>l->items.size())throw Error(p,"iter.permutations invalid r.");auto out=std::make_shared<ListData>();std::vector<bool> used(l->items.size());std::vector<Value> cur;std::function<void()> dfs=[&]{if(cur.size()==static_cast<std::size_t>(r)){auto q=std::make_shared<ListData>();q->items=cur;out->items.emplace_back(q);return;}for(std::size_t i=0;i<l->items.size();++i)if(!used[i]){used[i]=true;cur.push_back(l->items[i]);dfs();cur.pop_back();used[i]=false;}};dfs();return Value(out);},true);
    m->exports["combinations"]=callable("iter.combinations",2,2,[](const std::vector<Value>&a,SourcePos p){auto l=list_value(a[0],p,"iter.combinations");auto r=integer(a[1],p,"iter.combinations");if(r<0||static_cast<std::size_t>(r)>l->items.size())throw Error(p,"iter.combinations invalid r.");auto out=std::make_shared<ListData>();std::vector<Value> cur;std::function<void(std::size_t)> dfs=[&](std::size_t pos){if(cur.size()==static_cast<std::size_t>(r)){auto q=std::make_shared<ListData>();q->items=cur;out->items.emplace_back(q);return;}for(std::size_t i=pos;i<l->items.size();++i){cur.push_back(l->items[i]);dfs(i+1);cur.pop_back();}};dfs(0);return Value(out);});
  }else if(name=="copy"){
    m->exports["shallow"]=callable("copy.shallow",1,1,[](const std::vector<Value>&a,SourcePos p){if(auto l=std::get_if<std::shared_ptr<ListData>>(&a[0].data())){auto o=std::make_shared<ListData>();o->items=(*l)->items;return Value(o);}if(auto m=std::get_if<std::shared_ptr<MapData>>(&a[0].data())){auto o=std::make_shared<MapData>();o->items=(*m)->items;return Value(o);}if(auto s=std::get_if<std::shared_ptr<SetData>>(&a[0].data())){auto o=std::make_shared<SetData>();o->items=(*s)->items;return Value(o);}return a[0];});
    m->exports["deep"]=callable("copy.deep",1,1,[](const std::vector<Value>&a,SourcePos){std::function<Value(const Value&)> cp=[&](const Value&v)->Value{if(auto l=std::get_if<std::shared_ptr<ListData>>(&v.data())){auto o=std::make_shared<ListData>();for(auto&i:(*l)->items)o->items.push_back(cp(i));return Value(o);}if(auto m=std::get_if<std::shared_ptr<MapData>>(&v.data())){auto o=std::make_shared<MapData>();for(auto&i:(*m)->items)o->items.emplace_back(i.first,cp(i.second));return Value(o);}if(auto s=std::get_if<std::shared_ptr<SetData>>(&v.data())){auto o=std::make_shared<SetData>();for(auto&i:(*s)->items)o->items.push_back(cp(i));return Value(o);}return v;};return cp(a[0]);});
  }else if(name=="operator"){
    auto bin=[&](const std::string& n,auto op){m->exports[n]=callable("operator."+n,2,2,[n,op](const std::vector<Value>&a,SourcePos p){return op(a[0],a[1],p,n);});};
    bin("add",[](const Value&a,const Value&b,SourcePos p,const std::string&){if(auto x=std::get_if<std::int64_t>(&a.data())){if(auto y=std::get_if<std::int64_t>(&b.data()))return Value(*x+*y);}if((std::holds_alternative<std::int64_t>(a.data())||std::holds_alternative<double>(a.data()))&&(std::holds_alternative<std::int64_t>(b.data())||std::holds_alternative<double>(b.data())))return Value(number(a,p,"operator.add")+number(b,p,"operator.add"));if(auto x=std::get_if<std::string>(&a.data()))if(auto y=std::get_if<std::string>(&b.data()))return Value(*x+*y);throw Error(p,"operator.add unsupported operands.");});
    bin("sub",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.sub")-number(b,p,"operator.sub"));});
    bin("mul",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.mul")*number(b,p,"operator.mul"));});
    bin("div",[](const Value&a,const Value&b,SourcePos p,const std::string&){auto d=number(b,p,"operator.div");if(d==0)throw Error(p,"operator.div division by zero.");return Value(number(a,p,"operator.div")/d);});
    bin("mod",[](const Value&a,const Value&b,SourcePos p,const std::string&){auto x=integer(a,p,"operator.mod"),y=integer(b,p,"operator.mod");if(y==0)throw Error(p,"operator.mod division by zero.");return Value(x%y);});
    bin("eq",[](const Value&a,const Value&b,SourcePos,const std::string&){return Value(value_equal(a,b));});
    bin("ne",[](const Value&a,const Value&b,SourcePos,const std::string&){return Value(!value_equal(a,b));});
    bin("lt",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.lt")<number(b,p,"operator.lt"));});
    bin("le",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.le")<=number(b,p,"operator.le"));});
    bin("gt",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.gt")>number(b,p,"operator.gt"));});
    bin("ge",[](const Value&a,const Value&b,SourcePos p,const std::string&){return Value(number(a,p,"operator.ge")>=number(b,p,"operator.ge"));});
  }else if(name=="data"){
    m->exports["append"]=callable("data.append",2,2,[](const std::vector<Value>&a,SourcePos p){list_value(a[0],p,"data.append")->items.push_back(a[1]);return Value{};});
    m->exports["extend"]=callable("data.extend",2,2,[](const std::vector<Value>&a,SourcePos p){auto dst=list_value(a[0],p,"data.extend");auto src=list_value(a[1],p,"data.extend");dst->items.insert(dst->items.end(),src->items.begin(),src->items.end());return Value{};});
    m->exports["insert"]=callable("data.insert",3,3,[](const std::vector<Value>&a,SourcePos p){auto l=list_value(a[0],p,"data.insert");auto i=integer(a[1],p,"data.insert");if(i<0)i+=static_cast<std::int64_t>(l->items.size());i=std::clamp<std::int64_t>(i,0,static_cast<std::int64_t>(l->items.size()));l->items.insert(l->items.begin()+i,a[2]);return Value{};});
    m->exports["pop"]=callable("data.pop",1,2,[](const std::vector<Value>&a,SourcePos p){auto l=list_value(a[0],p,"data.pop");if(l->items.empty())throw Error(p,"data.pop cannot pop an empty List.");auto i=a.size()==2?integer(a[1],p,"data.pop"):static_cast<std::int64_t>(l->items.size()-1);if(i<0)i+=static_cast<std::int64_t>(l->items.size());if(i<0||static_cast<std::size_t>(i)>=l->items.size())throw Error(p,"data.pop index is out of bounds.");auto v=l->items[static_cast<std::size_t>(i)];l->items.erase(l->items.begin()+i);return v;},true);
    m->exports["clear"]=callable("data.clear",1,1,[](const std::vector<Value>&a,SourcePos p){if(auto l=std::get_if<std::shared_ptr<ListData>>(&a[0].data())){(*l)->items.clear();return Value{};}if(auto mp=std::get_if<std::shared_ptr<MapData>>(&a[0].data())){(*mp)->items.clear();return Value{};}if(auto s=std::get_if<std::shared_ptr<SetData>>(&a[0].data())){(*s)->items.clear();return Value{};}throw Error(p,"data.clear needs List, Map, or Set.");});
    m->exports["copy"]=callable("data.copy",1,1,[](const std::vector<Value>&a,SourcePos p){if(auto l=std::get_if<std::shared_ptr<ListData>>(&a[0].data())){auto out=std::make_shared<ListData>();out->items=(*l)->items;return Value(out);}if(auto mp=std::get_if<std::shared_ptr<MapData>>(&a[0].data())){auto out=std::make_shared<MapData>();out->items=(*mp)->items;return Value(out);}if(auto s=std::get_if<std::shared_ptr<SetData>>(&a[0].data())){auto out=std::make_shared<SetData>();out->items=(*s)->items;return Value(out);}throw Error(p,"data.copy needs List, Map, or Set.");});
    m->exports["get"]=callable("data.get",2,3,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.get");auto k=text(a[1],p,"data.get");for(auto& q:mp->items)if(q.first==k)return q.second;if(a.size()==3)return a[2];return Value{};},true);
    m->exports["set"]=callable("data.set",3,3,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.set");auto k=text(a[1],p,"data.set");for(auto& q:mp->items)if(q.first==k){q.second=a[2];return Value{};}mp->items.emplace_back(k,a[2]);return Value{};});
    m->exports["update"]=callable("data.update",2,2,[](const std::vector<Value>&a,SourcePos p){auto dst=map_value(a[0],p,"data.update");auto src=map_value(a[1],p,"data.update");for(auto& q:src->items){bool found=false;for(auto& d:dst->items)if(d.first==q.first){d.second=q.second;found=true;break;}if(!found)dst->items.push_back(q);}return Value{};});
    m->exports["delete"]=callable("data.delete",2,2,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.delete");auto k=text(a[1],p,"data.delete");auto i=std::find_if(mp->items.begin(),mp->items.end(),[&](const auto& q){return q.first==k;});if(i==mp->items.end())return Value(false);mp->items.erase(i);return Value(true);});
    m->exports["has"]=callable("data.has",2,2,[](const std::vector<Value>&a,SourcePos p){if(auto l=std::get_if<std::shared_ptr<ListData>>(&a[0].data())){for(auto& v:(*l)->items)if(value_equal(v,a[1]))return Value(true);return Value(false);}if(auto mp=std::get_if<std::shared_ptr<MapData>>(&a[0].data())){auto k=text(a[1],p,"data.has");for(auto& q:(*mp)->items)if(q.first==k)return Value(true);return Value(false);}throw Error(p,"data.has needs List or Map.");});
    m->exports["keys"]=callable("data.keys",1,1,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.keys");auto out=std::make_shared<ListData>();for(auto& q:mp->items)out->items.emplace_back(q.first);return Value(out);});
    m->exports["values"]=callable("data.values",1,1,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.values");auto out=std::make_shared<ListData>();for(auto& q:mp->items)out->items.push_back(q.second);return Value(out);});
    m->exports["items"]=callable("data.items",1,1,[](const std::vector<Value>&a,SourcePos p){auto mp=map_value(a[0],p,"data.items");auto out=std::make_shared<ListData>();for(auto& q:mp->items){auto pair=std::make_shared<ListData>();pair->items.emplace_back(q.first);pair->items.push_back(q.second);out->items.emplace_back(pair);}return Value(out);});
  }else if(name=="net"){
    m->exports["get"]=callable("net.get",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(curl_request("GET",text(a[0],p,"net.get"),"","text/plain",p).body);});m->exports["post"]=callable("net.post",2,2,[](const std::vector<Value>&a,SourcePos p){return Value(curl_request("POST",text(a[0],p,"net.post"),text(a[1],p,"net.post"),"text/plain; charset=utf-8",p).body);});m->exports["post_json"]=callable("net.post_json",2,2,[](const std::vector<Value>&a,SourcePos p){return Value(curl_request("POST",text(a[0],p,"net.post_json"),text(a[1],p,"net.post_json"),"application/json",p).body);});m->exports["request"]=callable("net.request",3,3,[](const std::vector<Value>&a,SourcePos p){return response_map(curl_request(text(a[0],p,"net.request"),text(a[1],p,"net.request"),text(a[2],p,"net.request"),"text/plain; charset=utf-8",p));});m->exports["download"]=callable("net.download",2,2,[](const std::vector<Value>&a,SourcePos p){auto url=text(a[0],p,"net.download");auto path=text(a[1],p,"net.download");if(normalized_system("curl -fL --max-time 60 -o "+shell_quote(path)+" "+shell_quote(url))!=0)throw Error(p,"net.download failed.");return Value{};});
  }else if(name=="node"){
    m->exports["version"]=callable("node.version",0,0,[](const std::vector<Value>&,SourcePos p){return Value(process_output("node --version",p));});m->exports["run"]=callable("node.run",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system("node "+shell_quote(text(a[0],p,"node.run")))));});m->exports["output"]=callable("node.output",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(process_output("node "+shell_quote(text(a[0],p,"node.output")),p));});m->exports["eval"]=callable("node.eval",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(process_output("node -e "+shell_quote(text(a[0],p,"node.eval")),p));});m->exports["npm"]=callable("node.npm",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system("npm "+text(a[0],p,"node.npm"))));});m->exports["npx"]=callable("node.npx",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system("npx "+text(a[0],p,"node.npx"))));});
  }else if(name=="next"){
    auto in_project=[](const std::string& project,const std::string& command){return "cd "+shell_quote(project)+" && "+command;};m->exports["create"]=callable("next.create",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system("npx create-next-app@latest "+shell_quote(text(a[0],p,"next.create")))));});m->exports["dev"]=callable("next.dev",1,1,[in_project](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system(in_project(text(a[0],p,"next.dev"),"npm run dev"))));});m->exports["build"]=callable("next.build",1,1,[in_project](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system(in_project(text(a[0],p,"next.build"),"npm run build"))));});m->exports["start"]=callable("next.start",1,1,[in_project](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system(in_project(text(a[0],p,"next.start"),"npm run start"))));});m->exports["lint"]=callable("next.lint",1,1,[in_project](const std::vector<Value>&a,SourcePos p){return Value(static_cast<std::int64_t>(normalized_system(in_project(text(a[0],p,"next.lint"),"npm run lint"))));});
  }else if(name=="game"){
    m->exports["new"]=callable("game.new",3,3,[](const std::vector<Value>&a,SourcePos p){auto w=integer(a[0],p,"game.new");auto h=integer(a[1],p,"game.new");if(w<1||h<1||w>8192||h>8192)throw Error(p,"game.new size must be between 1 and 8192.");GameScene scene;scene.width=static_cast<int>(w);scene.height=static_cast<int>(h);scene.title=text(a[2],p,"game.new");auto id=next_scene()++;scenes()[id]=std::move(scene);return Value(id);});m->exports["background"]=callable("game.background",2,2,[](const std::vector<Value>&a,SourcePos p){scene_for(integer(a[0],p,"game.background"),p).background=text(a[1],p,"game.background");return Value{};});m->exports["clear"]=callable("game.clear",1,1,[](const std::vector<Value>&a,SourcePos p){scene_for(integer(a[0],p,"game.clear"),p).draw.clear();return Value{};});
    m->exports["rect"]=callable("game.rect",7,7,[](const std::vector<Value>&a,SourcePos p){auto& s=scene_for(integer(a[0],p,"game.rect"),p);std::ostringstream q;auto fill=boolean(a[6],p,"game.rect");q<<"ctx."<<(fill?"fillStyle":"strokeStyle")<<"=\""<<js_escape(text(a[5],p,"game.rect"))<<"\";ctx."<<(fill?"fillRect":"strokeRect")<<"("<<number(a[1],p,"game.rect")<<","<<number(a[2],p,"game.rect")<<","<<number(a[3],p,"game.rect")<<","<<number(a[4],p,"game.rect")<<");";s.draw.push_back(q.str());return Value{};});
    m->exports["circle"]=callable("game.circle",6,6,[](const std::vector<Value>&a,SourcePos p){auto& s=scene_for(integer(a[0],p,"game.circle"),p);auto fill=boolean(a[5],p,"game.circle");std::ostringstream q;q<<"ctx.beginPath();ctx.arc("<<number(a[1],p,"game.circle")<<","<<number(a[2],p,"game.circle")<<","<<number(a[3],p,"game.circle")<<",0,Math.PI*2);ctx."<<(fill?"fillStyle":"strokeStyle")<<"=\""<<js_escape(text(a[4],p,"game.circle"))<<"\";ctx."<<(fill?"fill()":"stroke()")<<";";s.draw.push_back(q.str());return Value{};});
    m->exports["line"]=callable("game.line",7,7,[](const std::vector<Value>&a,SourcePos p){auto& s=scene_for(integer(a[0],p,"game.line"),p);std::ostringstream q;q<<"ctx.beginPath();ctx.moveTo("<<number(a[1],p,"game.line")<<","<<number(a[2],p,"game.line")<<");ctx.lineTo("<<number(a[3],p,"game.line")<<","<<number(a[4],p,"game.line")<<");ctx.strokeStyle=\""<<js_escape(text(a[5],p,"game.line"))<<"\";ctx.lineWidth="<<number(a[6],p,"game.line")<<";ctx.stroke();";s.draw.push_back(q.str());return Value{};});
    m->exports["text"]=callable("game.text",6,6,[](const std::vector<Value>&a,SourcePos p){auto& s=scene_for(integer(a[0],p,"game.text"),p);std::ostringstream q;q<<"ctx.fillStyle=\""<<js_escape(text(a[5],p,"game.text"))<<"\";ctx.font=\""<<number(a[4],p,"game.text")<<"px system-ui\";ctx.fillText(\""<<js_escape(text(a[1],p,"game.text"))<<"\","<<number(a[2],p,"game.text")<<","<<number(a[3],p,"game.text")<<");";s.draw.push_back(q.str());return Value{};});
    m->exports["script"]=callable("game.script",2,2,[](const std::vector<Value>&a,SourcePos p){scene_for(integer(a[0],p,"game.script"),p).scripts.push_back(text(a[1],p,"game.script"));return Value{};});m->exports["html"]=callable("game.html",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(scene_html(integer(a[0],p,"game.html"),p));});m->exports["save"]=callable("game.save",2,2,[](const std::vector<Value>&a,SourcePos p){auto path=std::filesystem::path(text(a[1],p,"game.save"));if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out)throw Error(p,"game.save could not write '"+path.string()+"'.");out<<scene_html(integer(a[0],p,"game.save"),p);return Value{};});m->exports["show"]=callable("game.show",1,1,[](const std::vector<Value>&a,SourcePos p){auto id=integer(a[0],p,"game.show");auto path=std::filesystem::temp_directory_path()/("se-game-"+std::to_string(id)+".html");std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out)throw Error(p,"game.show could not create a display file.");out<<scene_html(id,p);out.close();open_file(path);return Value{};});
  }
  return m;
}

} // namespace s
