#include "s/interpreter.hpp"
#include "s/ffi.hpp"
#include "s/platform.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <random>
#include <sstream>
#include <thread>

namespace s {
namespace {
struct ReturnSignal { Value value; };
std::shared_ptr<CallableData> callable(std::string name,std::size_t min,std::size_t max,std::function<Value(const std::vector<Value>&,SourcePos)> f,bool variadic=false){
  auto c=std::make_shared<CallableData>();
  c->name=std::move(name);c->min_args=min;c->max_args=max;c->variadic=variadic;c->call=std::move(f);
  return c;
}
double number_value(const Value&v,SourcePos p,const std::string&name){
  if(auto n=std::get_if<std::int64_t>(&v.data()))return static_cast<double>(*n);
  if(auto n=std::get_if<double>(&v.data()))return *n;
  throw Error(p,name+" needs a number.");
}
std::int64_t int_value(const Value&v,SourcePos p,const std::string&name){
  if(auto n=std::get_if<std::int64_t>(&v.data()))return *n;
  throw Error(p,name+" needs an Int.");
}
std::string trim_conversion_text(std::string text){
  auto ws=[](unsigned char c){return std::isspace(c)!=0;};
  while(!text.empty()&&ws(static_cast<unsigned char>(text.front())))text.erase(text.begin());
  while(!text.empty()&&ws(static_cast<unsigned char>(text.back())))text.pop_back();
  return text;
}
bool one_utf8_character(const std::string& text){
  if(text.empty())return false;
  const auto first=static_cast<unsigned char>(text[0]);
  std::size_t size=0;
  if(first<=0x7f)size=1;
  else if(first>=0xc2&&first<=0xdf)size=2;
  else if(first>=0xe0&&first<=0xef)size=3;
  else if(first>=0xf0&&first<=0xf4)size=4;
  else return false;
  if(text.size()!=size)return false;
  for(std::size_t i=1;i<size;++i)if((static_cast<unsigned char>(text[i])&0xc0)!=0x80)return false;
  if(size==3){
    const auto second=static_cast<unsigned char>(text[1]);
    if(first==0xe0&&second<0xa0)return false;
    if(first==0xed&&second>=0xa0)return false;
  }
  if(size==4){
    const auto second=static_cast<unsigned char>(text[1]);
    if(first==0xf0&&second<0x90)return false;
    if(first==0xf4&&second>0x8f)return false;
  }
  return true;
}
std::string utf8_from_codepoint(std::int64_t code,SourcePos p){
  if(code<0||code>0x10ffff||(code>=0xd800&&code<=0xdfff))throw Error(p,"char needs a valid Unicode code point or one-character Text.");
  std::string out;
  if(code<=0x7f)out.push_back(static_cast<char>(code));
  else if(code<=0x7ff){out.push_back(static_cast<char>(0xc0|(code>>6)));out.push_back(static_cast<char>(0x80|(code&0x3f)));}
  else if(code<=0xffff){out.push_back(static_cast<char>(0xe0|(code>>12)));out.push_back(static_cast<char>(0x80|((code>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(code&0x3f)));}
  else{out.push_back(static_cast<char>(0xf0|(code>>18)));out.push_back(static_cast<char>(0x80|((code>>12)&0x3f)));out.push_back(static_cast<char>(0x80|((code>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(code&0x3f)));}
  return out;
}
Value convert_int_value(const Value& value,SourcePos p){
  if(auto n=std::get_if<std::int64_t>(&value.data()))return Value(*n);
  if(auto n=std::get_if<double>(&value.data())){
    if(!std::isfinite(*n)||std::trunc(*n)!=*n||*n<static_cast<double>(std::numeric_limits<std::int64_t>::min())||*n>static_cast<double>(std::numeric_limits<std::int64_t>::max()))throw Error(p,"int needs a whole number in Int range.");
    return Value(static_cast<std::int64_t>(*n));
  }
  auto text=std::get_if<std::string>(&value.data());
  if(!text)throw Error(p,"int needs Text, Int, or a whole Num.");
  auto cleaned=trim_conversion_text(*text);
  try{
    std::size_t used=0;
    auto parsed=std::stoll(cleaned,&used,10);
    if(used!=cleaned.size())throw Error(p,"Could not convert '"+*text+"' to Int.");
    return Value(static_cast<std::int64_t>(parsed));
  }catch(const Error&){throw;}catch(...){throw Error(p,"Could not convert '"+*text+"' to Int.");}
}
Value convert_num_value(const Value& value,SourcePos p){
  if(auto n=std::get_if<std::int64_t>(&value.data()))return Value(static_cast<double>(*n));
  if(auto n=std::get_if<double>(&value.data()))return Value(*n);
  auto text=std::get_if<std::string>(&value.data());
  if(!text)throw Error(p,"num needs Text, Int, or Num.");
  auto cleaned=trim_conversion_text(*text);
  try{
    std::size_t used=0;
    auto parsed=std::stod(cleaned,&used);
    if(used!=cleaned.size()||!std::isfinite(parsed))throw Error(p,"Could not convert '"+*text+"' to Num.");
    return Value(parsed);
  }catch(const Error&){throw;}catch(...){throw Error(p,"Could not convert '"+*text+"' to Num.");}
}
Value convert_bool_value(const Value& value,SourcePos p){
  if(auto b=std::get_if<bool>(&value.data()))return Value(*b);
  auto text=std::get_if<std::string>(&value.data());
  if(!text)throw Error(p,"bool needs Bool or Text 'true'/'false'.");
  auto cleaned=trim_conversion_text(*text);
  std::transform(cleaned.begin(),cleaned.end(),cleaned.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
  if(cleaned=="true")return Value(true);
  if(cleaned=="false")return Value(false);
  throw Error(p,"Could not convert '"+*text+"' to Bool. Use true or false.");
}
Value convert_char_value(const Value& value,SourcePos p){
  if(auto text=std::get_if<std::string>(&value.data())){
    if(!one_utf8_character(*text))throw Error(p,"char needs exactly one Unicode character.");
    return Value(*text);
  }
  if(auto code=std::get_if<std::int64_t>(&value.data()))return Value(utf8_from_codepoint(*code,p));
  throw Error(p,"char needs one-character Text or an Int Unicode code point.");
}
std::shared_ptr<CallableData> conversion_callable(const std::string& name){
  if(name=="text"||name=="string")return callable(name,1,1,[](const std::vector<Value>&a,SourcePos){return Value(a[0].text());});
  if(name=="int"||name=="integer")return callable(name,1,1,[](const std::vector<Value>&a,SourcePos p){return convert_int_value(a[0],p);});
  if(name=="num"||name=="double"||name=="float")return callable(name,1,1,[](const std::vector<Value>&a,SourcePos p){return convert_num_value(a[0],p);});
  if(name=="bool"||name=="boolean")return callable(name,1,1,[](const std::vector<Value>&a,SourcePos p){return convert_bool_value(a[0],p);});
  if(name=="char")return callable(name,1,1,[](const std::vector<Value>&a,SourcePos p){return convert_char_value(a[0],p);});
  return {};
}
std::mt19937_64& random_engine(){static std::mt19937_64 engine{std::random_device{}()};return engine;}
std::string platform_name(){
#ifdef _WIN32
  return "windows";
#elif __APPLE__
  return "macos";
#elif __linux__
  return "linux";
#else
  return "unknown";
#endif
}
std::string help_text(const Value&v){
  std::ostringstream out;out<<v.type_name()<<"\n";
  if(std::holds_alternative<std::string>(v.data()))out<<"members: help, len, upper, lower\nexample: text.upper";
  else if(std::holds_alternative<std::shared_ptr<ByteBufferData>>(v.data()))out<<"members: help, len";
  else if(std::holds_alternative<std::shared_ptr<ListData>>(v.data()))out<<"members: help, len, add value, remove value\nexample: nums.add 4";
  else if(std::holds_alternative<std::shared_ptr<MapData>>(v.data()))out<<"members: help, len\nindex with map[\"key\"]";
  else if(std::holds_alternative<std::shared_ptr<SetData>>(v.data()))out<<"members: help, len, add value, remove value";
  else if(auto o=std::get_if<std::shared_ptr<ObjectData>>(&v.data())){out<<"members: help";if((*o)->type){for(auto&[name,value]:(*o)->fields){(void)value;out<<", "<<name;}for(auto&[name,method]:(*o)->type->methods){(void)method;out<<", "<<name;}}}
  else if(auto m=std::get_if<std::shared_ptr<ModuleData>>(&v.data())){out<<"members: help";for(auto&[name,value]:(*m)->exports){(void)value;out<<", "<<name;}}
  else if(std::holds_alternative<std::shared_ptr<FileData>>(v.data()))out<<"members: help, read, write text, close";
  else if(std::holds_alternative<PathData>(v.data()))out<<"members: help, name, ext, parent, exists, is_file, is_dir";
  else if(std::holds_alternative<std::shared_ptr<ErrorData>>(v.data()))out<<"members: help, message, source, line, kind";
  else if(std::holds_alternative<DurationData>(v.data()))out<<"members: help\nuse with wait duration";
  else if(std::holds_alternative<TimeData>(v.data()))out<<"members: help";
  else out<<"members: help";
  return out.str();
}
}

Interpreter::Interpreter(std::istream& i,std::ostream& o):in_(i),out_(o),env_(std::make_shared<Environment>()){install_builtins(env_);}
[[noreturn]] void Interpreter::runtime_fail(SourcePos p,const std::string&m,const std::string&kind) const{throw RuntimeFailure({m,source_,p.line,kind});}
void Interpreter::execute_block(const ast::Block& b,std::shared_ptr<Environment> e){auto old=env_;env_=std::move(e);try{for(auto&s:b)execute(s);}catch(...){env_=old;throw;}env_=old;}

void Interpreter::install_builtins(const std::shared_ptr<Environment>&e){
  e->define("read",callable("read",1,1,[this](const std::vector<Value>&a,SourcePos p){auto path=path_text(a[0],p);std::ifstream f(path,std::ios::binary);if(!f)runtime_fail(p,"Could not read '"+path+"'.","FileError");return Value(std::string(std::istreambuf_iterator<char>(f),{}));}));
  e->define("write",callable("write",2,2,[this](const std::vector<Value>&a,SourcePos p){auto path=path_text(a[0],p);if(!std::holds_alternative<std::string>(a[1].data()))runtime_fail(p,"write needs Text.","FileError");std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)runtime_fail(p,"Could not write '"+path+"'.","FileError");f<<std::get<std::string>(a[1].data());if(!f)runtime_fail(p,"Writing '"+path+"' failed.","FileError");return Value{};}));
  e->define("append",callable("append",2,2,[this](const std::vector<Value>&a,SourcePos p){auto path=path_text(a[0],p);if(!std::holds_alternative<std::string>(a[1].data()))runtime_fail(p,"append needs Text.","FileError");std::ofstream f(path,std::ios::binary|std::ios::app);if(!f)runtime_fail(p,"Could not append to '"+path+"'.","FileError");f<<std::get<std::string>(a[1].data());if(!f)runtime_fail(p,"Appending to '"+path+"' failed.","FileError");return Value{};}));
  e->define("open",callable("open",1,1,[this](const std::vector<Value>&a,SourcePos p){auto path=path_text(a[0],p);auto file=std::make_shared<FileData>();file->path=path;file->stream=std::make_shared<std::fstream>(path,std::ios::in|std::ios::out|std::ios::binary);if(!file->stream->is_open())runtime_fail(p,"Could not open '"+path+"'.","FileError");return Value(file);}));
  e->define("wait",callable("wait",1,1,[this](const std::vector<Value>&a,SourcePos p){auto d=std::get_if<DurationData>(&a[0].data());if(!d)runtime_fail(p,"wait needs a Duration.","TimeError");if(d->milliseconds<0)runtime_fail(p,"wait duration cannot be negative.","TimeError");std::this_thread::sleep_for(std::chrono::milliseconds(d->milliseconds));return Value{};}));
  e->define("bytes",callable("bytes",1,1,[](const std::vector<Value>&a,SourcePos p){auto text=std::get_if<std::string>(&a[0].data());if(!text)throw Error(p,"bytes needs Text.");auto out=std::make_shared<ByteBufferData>();out->bytes.assign(text->begin(),text->end());return Value(out);}));
}

std::shared_ptr<ModuleData> Interpreter::builtin_module(const std::string&name){
  if(combined_platform_builtin(name))return combined_platform_builtin_module(name,*this);
  auto m=std::make_shared<ModuleData>();m->name=name;
  if(name=="path"){
    m->exports["join"]=callable("path.join",1,64,[](const std::vector<Value>&a,SourcePos p){std::filesystem::path r;for(auto&v:a)r/=path_text(v,p);return Value(PathData{r});},true);
    m->exports["name"]=callable("path.name",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::filesystem::path(path_text(a[0],p)).filename().string());});
    m->exports["ext"]=callable("path.ext",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::filesystem::path(path_text(a[0],p)).extension().string());});
    m->exports["parent"]=callable("path.parent",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(PathData{std::filesystem::path(path_text(a[0],p)).parent_path()});});
    m->exports["exists"]=callable("path.exists",1,1,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;auto r=std::filesystem::exists(path_text(a[0],p),ec);return Value(!ec&&r);});
    m->exports["is_file"]=callable("path.is_file",1,1,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;auto r=std::filesystem::is_regular_file(path_text(a[0],p),ec);return Value(!ec&&r);});
    m->exports["is_dir"]=callable("path.is_dir",1,1,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;auto r=std::filesystem::is_directory(path_text(a[0],p),ec);return Value(!ec&&r);});
  }else if(name=="time"){
    m->exports["now"]=callable("time.now",0,0,[](const std::vector<Value>&,SourcePos){return Value(TimeData{std::chrono::system_clock::now()});});
    m->exports["unix"]=callable("time.unix",1,1,[](const std::vector<Value>&a,SourcePos p){auto t=std::get_if<TimeData>(&a[0].data());if(!t)throw Error(p,"time.unix needs Time.");return Value(static_cast<std::int64_t>(std::chrono::system_clock::to_time_t(t->point)));});
    m->exports["from_unix"]=callable("time.from_unix",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(TimeData{std::chrono::system_clock::from_time_t(static_cast<std::time_t>(int_value(a[0],p,"time.from_unix")))});});
    m->exports["iso"]=callable("time.iso",1,1,[](const std::vector<Value>&a,SourcePos p){auto t=std::get_if<TimeData>(&a[0].data());if(!t)throw Error(p,"time.iso needs Time.");auto raw=std::chrono::system_clock::to_time_t(t->point);std::tm tm{};
#ifdef _WIN32
      gmtime_s(&tm,&raw);
#else
      gmtime_r(&raw,&tm);
#endif
      std::ostringstream out;out<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%SZ");return Value(out.str());});
  }
  else if(name=="file"){
    m->exports["read"]=env_->get("read",{});m->exports["write"]=env_->get("write",{});m->exports["append"]=env_->get("append",{});m->exports["open"]=env_->get("open",{});
    m->exports["copy"]=callable("file.copy",2,2,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;std::filesystem::copy_file(path_text(a[0],p),path_text(a[1],p),std::filesystem::copy_options::overwrite_existing,ec);if(ec)throw Error(p,"file.copy: "+ec.message());return Value{};});
    m->exports["move"]=callable("file.move",2,2,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;std::filesystem::rename(path_text(a[0],p),path_text(a[1],p),ec);if(ec)throw Error(p,"file.move: "+ec.message());return Value{};});
    m->exports["copytree"]=callable("file.copytree",2,2,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;std::filesystem::copy(path_text(a[0],p),path_text(a[1],p),std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing,ec);if(ec)throw Error(p,"file.copytree: "+ec.message());return Value{};});
    m->exports["remove"]=callable("file.remove",1,1,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;std::filesystem::remove_all(path_text(a[0],p),ec);if(ec)throw Error(p,"file.remove: "+ec.message());return Value{};});
    m->exports["mkdir"]=callable("file.mkdir",1,1,[](const std::vector<Value>&a,SourcePos p){std::error_code ec;std::filesystem::create_directories(path_text(a[0],p),ec);if(ec)throw Error(p,"file.mkdir: "+ec.message());return Value{};});
  }
  else if(name=="math"){
    m->exports["pi"]=Value(3.14159265358979323846);m->exports["sqrt"]=callable("math.sqrt",1,1,[](const std::vector<Value>&a,SourcePos p){auto n=number_value(a[0],p,"math.sqrt");if(n<0)throw Error(p,"math.sqrt needs a non-negative number.");return Value(std::sqrt(n));});m->exports["abs"]=callable("math.abs",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::fabs(number_value(a[0],p,"math.abs")));});m->exports["floor"]=callable("math.floor",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::floor(number_value(a[0],p,"math.floor")));});m->exports["ceil"]=callable("math.ceil",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::ceil(number_value(a[0],p,"math.ceil")));});m->exports["round"]=callable("math.round",1,1,[](const std::vector<Value>&a,SourcePos p){return Value(std::round(number_value(a[0],p,"math.round")));});m->exports["pow"]=callable("math.pow",2,2,[](const std::vector<Value>&a,SourcePos p){return Value(std::pow(number_value(a[0],p,"math.pow"),number_value(a[1],p,"math.pow")));});m->exports["min"]=callable("math.min",2,2,[](const std::vector<Value>&a,SourcePos p){return Value(std::min(number_value(a[0],p,"math.min"),number_value(a[1],p,"math.min")));});m->exports["max"]=callable("math.max",2,2,[](const std::vector<Value>&a,SourcePos p){return Value(std::max(number_value(a[0],p,"math.max"),number_value(a[1],p,"math.max")));});
  }else if(name=="random"){
    m->exports["int"]=callable("random.int",2,2,[](const std::vector<Value>&a,SourcePos p){auto lo=int_value(a[0],p,"random.int"),hi=int_value(a[1],p,"random.int");if(lo>hi)throw Error(p,"random.int needs min <= max.");std::uniform_int_distribution<std::int64_t> d(lo,hi);return Value(d(random_engine()));});m->exports["num"]=callable("random.num",0,0,[](const std::vector<Value>&,SourcePos){std::uniform_real_distribution<double> d(0.0,1.0);return Value(d(random_engine()));});
  }else if(name=="os"){
    m->exports["platform"]=Value(platform_name());m->exports["cwd"]=callable("os.cwd",0,0,[](const std::vector<Value>&,SourcePos){return Value(PathData{std::filesystem::current_path()});});m->exports["getenv"]=callable("os.getenv",1,1,[](const std::vector<Value>&a,SourcePos p){auto key=std::get_if<std::string>(&a[0].data());if(!key)throw Error(p,"os.getenv needs Text.");const char*v=std::getenv(key->c_str());return Value(v?std::string(v):std::string{});});m->exports["has_env"]=callable("os.has_env",1,1,[](const std::vector<Value>&a,SourcePos p){auto key=std::get_if<std::string>(&a[0].data());if(!key)throw Error(p,"os.has_env needs Text.");return Value(std::getenv(key->c_str())!=nullptr);});
  }
  return m;
}

Value Interpreter::call(Value c,const std::vector<Value>&args,SourcePos p){auto f=std::get_if<std::shared_ptr<CallableData>>(&c.data());if(!f)throw Error(p,"Only a function or method can be called.");if((!(*f)->variadic&&args.size()!=(*f)->min_args)||((*f)->variadic&&args.size()<(*f)->min_args))throw Error(p,"'"+(*f)->name+"' needs "+std::to_string((*f)->min_args)+((*f)->variadic?" or more":"")+" values, but got "+std::to_string(args.size())+".");return (*f)->call(args,p);}

Value Interpreter::instantiate(const std::shared_ptr<TypeData>&type,SourcePos p){auto object=std::make_shared<ObjectData>();object->type=type;auto local=std::make_shared<Environment>(type->closure,object);auto old=env_;env_=local;try{for(auto&f:type->fields)object->fields[f.name]=evaluate(f.value);}catch(...){env_=old;throw;}env_=old;(void)p;return object;}

Value Interpreter::member(Value v,const std::string&name,SourcePos p,bool auto_call){
  if(name=="help")return help_text(v);
  if(name=="len"){if(auto t=std::get_if<std::string>(&v.data()))return static_cast<std::int64_t>(t->size());if(auto b=std::get_if<std::shared_ptr<ByteBufferData>>(&v.data()))return static_cast<std::int64_t>((*b)->bytes.size());if(auto l=std::get_if<std::shared_ptr<ListData>>(&v.data()))return static_cast<std::int64_t>((*l)->items.size());if(auto m=std::get_if<std::shared_ptr<MapData>>(&v.data()))return static_cast<std::int64_t>((*m)->items.size());if(auto s=std::get_if<std::shared_ptr<SetData>>(&v.data()))return static_cast<std::int64_t>((*s)->items.size());}
  if((name=="upper"||name=="lower")&&std::holds_alternative<std::string>(v.data())){auto t=std::get<std::string>(v.data());std::transform(t.begin(),t.end(),t.begin(),[&](unsigned char c){return static_cast<char>(name=="upper"?std::toupper(c):std::tolower(c));});return t;}
  if(auto l=std::get_if<std::shared_ptr<ListData>>(&v.data())){if(name=="add")return callable("List.add",1,1,[list=*l](const std::vector<Value>&a,SourcePos){list->items.push_back(a[0]);return Value{};});if(name=="remove")return callable("List.remove",1,1,[list=*l](const std::vector<Value>&a,SourcePos){auto i=std::find_if(list->items.begin(),list->items.end(),[&](const Value&x){return value_equal(x,a[0]);});if(i==list->items.end())return Value(false);list->items.erase(i);return Value(true);});}
  if(auto s=std::get_if<std::shared_ptr<SetData>>(&v.data())){if(name=="add")return callable("Set.add",1,1,[set=*s](const std::vector<Value>&a,SourcePos){for(auto&x:set->items)if(value_equal(x,a[0]))return Value{};set->items.push_back(a[0]);return Value{};});if(name=="remove")return callable("Set.remove",1,1,[set=*s](const std::vector<Value>&a,SourcePos){auto i=std::find_if(set->items.begin(),set->items.end(),[&](const Value&x){return value_equal(x,a[0]);});if(i==set->items.end())return Value(false);set->items.erase(i);return Value(true);});}
  if(auto o=std::get_if<std::shared_ptr<ObjectData>>(&v.data())){if(auto f=(*o)->fields.find(name);f!=(*o)->fields.end())return f->second;auto type=(*o)->type;if(type){if(auto m=type->methods.find(name);m!=type->methods.end()){auto method=m->second;auto object=*o;auto c=callable(type->name+"."+name,method->params.size(),method->params.size(),[this,object,method,type](const std::vector<Value>&a,SourcePos)->Value{auto local=std::make_shared<Environment>(type->closure,object);for(std::size_t i=0;i<a.size();++i)local->define(method->params[i],a[i]);try{execute_block(method->body,local);}catch(ReturnSignal&r){return r.value;}return {};});if(auto_call&&method->params.empty())return call(c,{},p);return c;}}}
  if(auto m=std::get_if<std::shared_ptr<ModuleData>>(&v.data())){if(auto x=(*m)->exports.find(name);x!=(*m)->exports.end()){auto r=x->second;if(auto_call)if(auto c=std::get_if<std::shared_ptr<CallableData>>(&r.data());c&&(*c)->min_args==0&&!(*c)->variadic)return call(r,{},p);return r;}}
  if(auto f=std::get_if<std::shared_ptr<FileData>>(&v.data())){auto file=*f;if(name=="read"){auto c=callable("File.read",0,0,[this,file](const std::vector<Value>&,SourcePos q){if(file->closed||!file->stream||!file->stream->is_open())runtime_fail(q,"This file is closed.","FileError");file->stream->clear();file->stream->seekg(0);return Value(std::string(std::istreambuf_iterator<char>(*file->stream),{}));});return auto_call?call(c,{},p):Value(c);}if(name=="close"){auto c=callable("File.close",0,0,[file](const std::vector<Value>&,SourcePos){if(!file->closed&&file->stream&&file->stream->is_open())file->stream->close();file->closed=true;return Value{};});return auto_call?call(c,{},p):Value(c);}if(name=="write")return callable("File.write",1,1,[this,file](const std::vector<Value>&a,SourcePos q){if(file->closed||!file->stream||!file->stream->is_open())runtime_fail(q,"This file is closed.","FileError");if(!std::holds_alternative<std::string>(a[0].data()))runtime_fail(q,"File.write needs Text.","FileError");file->stream->clear();file->stream->seekp(0,std::ios::end);*file->stream<<std::get<std::string>(a[0].data());file->stream->flush();if(!*file->stream)runtime_fail(q,"Writing the file failed.","FileError");return Value{};});}
  if(auto q=std::get_if<PathData>(&v.data())){if(name=="name")return q->path.filename().string();if(name=="ext")return q->path.extension().string();if(name=="parent")return PathData{q->path.parent_path()};if(name=="exists")return std::filesystem::exists(q->path);if(name=="is_file")return std::filesystem::is_regular_file(q->path);if(name=="is_dir")return std::filesystem::is_directory(q->path);}
  if(auto e=std::get_if<std::shared_ptr<ErrorData>>(&v.data())){if(name=="message")return (*e)->message;if(name=="source")return (*e)->source;if(name=="line")return static_cast<std::int64_t>((*e)->line);if(name=="kind")return (*e)->kind;}
  throw Error(p,v.type_name()+" has no member named '"+name+"'.","Use value.help to see the members available on this value.");
}

Value Interpreter::evaluate(const ast::ExprPtr& e){
  if(auto x=std::dynamic_pointer_cast<ast::Literal>(e))return std::visit([](auto v){return Value(v);},x->value);if(auto x=std::dynamic_pointer_cast<ast::Duration>(e))return DurationData{x->milliseconds};if(auto x=std::dynamic_pointer_cast<ast::Variable>(e)){Value v;if(env_->has(x->name))v=env_->get(x->name,x->pos);else if(auto builtin=conversion_callable(x->name))v=Value(builtin);else v=env_->get(x->name,x->pos);if(auto t=std::get_if<std::shared_ptr<TypeData>>(&v.data()))return instantiate(*t,x->pos);if(auto f=std::get_if<std::shared_ptr<CallableData>>(&v.data());f&&(*f)->min_args==0&&!(*f)->variadic)return call(v,{},x->pos);return v;}if(auto x=std::dynamic_pointer_cast<ast::Unary>(e)){auto v=evaluate(x->value);if(x->op==TokenKind::Not)return !v.truth(x->pos);if(auto n=std::get_if<std::int64_t>(&v.data()))return -*n;if(auto n=std::get_if<double>(&v.data()))return -*n;throw Error(x->pos,"Only a number can follow '-'.");}if(auto x=std::dynamic_pointer_cast<ast::Binary>(e)){auto a=evaluate(x->left);if(x->op==TokenKind::And&&!a.truth(x->pos))return false;if(x->op==TokenKind::Or&&a.truth(x->pos))return true;return binary(x->op,a,evaluate(x->right),x->pos);}
  if(auto x=std::dynamic_pointer_cast<ast::List>(e)){auto l=std::make_shared<ListData>();for(auto&i:x->items)l->items.push_back(evaluate(i));return l;}if(auto x=std::dynamic_pointer_cast<ast::Set>(e)){auto s=std::make_shared<SetData>();for(auto&i:x->items){auto v=evaluate(i);bool found=false;for(auto&old:s->items)if(value_equal(old,v)){found=true;break;}if(!found)s->items.push_back(std::move(v));}return s;}if(auto x=std::dynamic_pointer_cast<ast::Map>(e)){auto m=std::make_shared<MapData>();for(auto&i:x->items){auto k=evaluate(i.first);if(!std::holds_alternative<std::string>(k.data()))throw Error(i.first->pos,"Map keys are Text in SE.");auto key=std::get<std::string>(k.data());auto v=evaluate(i.second);auto old=std::find_if(m->items.begin(),m->items.end(),[&](auto&q){return q.first==key;});if(old==m->items.end())m->items.emplace_back(std::move(key),std::move(v));else old->second=std::move(v);}return m;}if(auto x=std::dynamic_pointer_cast<ast::Range>(e)){auto a=evaluate(x->start),b=evaluate(x->end);if(!std::holds_alternative<std::int64_t>(a.data())||!std::holds_alternative<std::int64_t>(b.data()))throw Error(x->pos,"A range needs two Int values.");auto l=std::make_shared<ListData>();auto from=std::get<std::int64_t>(a.data()),to=std::get<std::int64_t>(b.data());auto step=from<=to?1:-1;for(auto n=from;;n+=step){l->items.emplace_back(n);if(n==to)break;}return l;}
  if(auto x=std::dynamic_pointer_cast<ast::Index>(e)){auto v=evaluate(x->value),i=evaluate(x->index);if(auto l=std::get_if<std::shared_ptr<ListData>>(&v.data())){if(!std::holds_alternative<std::int64_t>(i.data()))throw Error(x->pos,"A List index needs Int.");auto n=std::get<std::int64_t>(i.data());if(n<0||static_cast<std::size_t>(n)>=(*l)->items.size())throw Error(x->pos,"List index "+std::to_string(n)+" is out of bounds. This list has "+std::to_string((*l)->items.size())+" items.");return (*l)->items[static_cast<std::size_t>(n)];}if(auto m=std::get_if<std::shared_ptr<MapData>>(&v.data())){if(!std::holds_alternative<std::string>(i.data()))throw Error(x->pos,"A Map key needs Text.");auto key=std::get<std::string>(i.data());for(auto&q:(*m)->items)if(q.first==key)return q.second;throw Error(x->pos,"Map has no key '"+key+"'.");}throw Error(x->pos,"Only List and Map can use [index].");}
  if(auto x=std::dynamic_pointer_cast<ast::Member>(e))return member(evaluate(x->value),x->name,x->pos,true);if(auto x=std::dynamic_pointer_cast<ast::Ask>(e)){auto q=evaluate(x->question);if(!std::holds_alternative<std::string>(q.data()))throw Error(x->pos,"ask needs Text.");out_<<q.text()<<": ";std::string answer;std::getline(in_,answer);return answer;}if(auto x=std::dynamic_pointer_cast<ast::TryExpr>(e))return evaluate(x->value);if(auto x=std::dynamic_pointer_cast<ast::Call>(e)){Value c;if(auto m=std::dynamic_pointer_cast<ast::Member>(x->callee))c=member(evaluate(m->value),m->name,m->pos,false);else if(auto v=std::dynamic_pointer_cast<ast::Variable>(x->callee)){if(env_->has(v->name))c=env_->get(v->name,x->pos);else if(auto builtin=conversion_callable(v->name))c=Value(builtin);else c=env_->get(v->name,x->pos);}else c=evaluate(x->callee);std::vector<Value>a;for(auto&i:x->args)a.push_back(evaluate(i));return call(c,a,x->pos);}throw Error(e->pos,"This expression is not implemented.");
}

void Interpreter::execute(const ast::StmtPtr& s){
  if(std::dynamic_pointer_cast<ast::Use>(s))return;if(auto x=std::dynamic_pointer_cast<ast::Say>(s)){out_<<evaluate(x->value).text()<<'\n';return;}if(auto x=std::dynamic_pointer_cast<ast::ExprStmt>(s)){auto value=evaluate(x->value);if(auto m=std::dynamic_pointer_cast<ast::Member>(x->value);m&&m->name=="help")out_<<value.text()<<'\n';return;}
  if(auto x=std::dynamic_pointer_cast<ast::Assign>(s)){auto v=evaluate(x->value);if(auto n=std::dynamic_pointer_cast<ast::Variable>(x->target)){env_->set(n->name,v);if(!x->init.empty()){auto o=std::get_if<std::shared_ptr<ObjectData>>(&v.data());if(!o)throw Error(x->pos,"Indented initialization needs an object.");execute_block(x->init,std::make_shared<Environment>(env_,*o));}return;}if(auto m=std::dynamic_pointer_cast<ast::Member>(x->target)){auto owner=evaluate(m->value);auto o=std::get_if<std::shared_ptr<ObjectData>>(&owner.data());if(!o||!(*o)->fields.contains(m->name))throw Error(x->pos,"Unknown object field '"+m->name+"'.");(*o)->fields[m->name]=v;return;}if(auto i=std::dynamic_pointer_cast<ast::Index>(x->target)){auto c=evaluate(i->value),k=evaluate(i->index);if(auto l=std::get_if<std::shared_ptr<ListData>>(&c.data())){if(!std::holds_alternative<std::int64_t>(k.data()))throw Error(x->pos,"A List index needs Int.");auto n=std::get<std::int64_t>(k.data());if(n<0||static_cast<std::size_t>(n)>=(*l)->items.size())throw Error(x->pos,"List index is out of bounds.");(*l)->items[static_cast<std::size_t>(n)]=v;return;}if(auto mp=std::get_if<std::shared_ptr<MapData>>(&c.data())){if(!std::holds_alternative<std::string>(k.data()))throw Error(x->pos,"A Map key needs Text.");auto key=std::get<std::string>(k.data());for(auto&q:(*mp)->items)if(q.first==key){q.second=v;return;}(*mp)->items.emplace_back(key,v);return;}throw Error(x->pos,"Only List or Map items can be assigned by index.");}throw Error(x->pos,"You can only assign to a name, field, or collection item.");}
  if(auto x=std::dynamic_pointer_cast<ast::If>(s)){execute_block(evaluate(x->condition).truth(x->pos)?x->then_block:x->else_block,std::make_shared<Environment>(env_));return;}
  if(auto x=std::dynamic_pointer_cast<ast::Match>(s)){auto subject=evaluate(x->value);for(auto& c:x->cases){if(value_equal(subject,evaluate(c.pattern))){execute_block(c.body,std::make_shared<Environment>(env_));return;}}execute_block(x->else_block,std::make_shared<Environment>(env_));return;}
  if(auto x=std::dynamic_pointer_cast<ast::Repeat>(s)){auto n=evaluate(x->count);if(!std::holds_alternative<std::int64_t>(n.data()))throw Error(x->pos,"repeat needs an Int count.");auto count=std::get<std::int64_t>(n.data());if(count<0)throw Error(x->pos,"repeat count cannot be negative.");for(std::int64_t i=0;i<count;++i)execute_block(x->body,std::make_shared<Environment>(env_));return;}
  if(auto x=std::dynamic_pointer_cast<ast::For>(s)){auto v=evaluate(x->values);if(auto l=std::get_if<std::shared_ptr<ListData>>(&v.data())){for(auto&item:(*l)->items){auto local=std::make_shared<Environment>(env_);local->define(x->names[0],item);execute_block(x->body,local);}return;}if(auto set=std::get_if<std::shared_ptr<SetData>>(&v.data())){for(auto&item:(*set)->items){auto local=std::make_shared<Environment>(env_);local->define(x->names[0],item);execute_block(x->body,local);}return;}if(auto map=std::get_if<std::shared_ptr<MapData>>(&v.data())){for(auto&item:(*map)->items){auto local=std::make_shared<Environment>(env_);local->define(x->names[0],item.first);if(x->names.size()>1)local->define(x->names[1],item.second);execute_block(x->body,local);}return;}throw Error(x->pos,"for needs a List, Set, Map, or range after 'in'.");}
  if(auto x=std::dynamic_pointer_cast<ast::While>(s)){while(evaluate(x->condition).truth(x->pos)){if(++loop_steps_>10000000)throw Error(x->pos,"This loop ran too long.","Check that its condition can become false.");execute_block(x->body,std::make_shared<Environment>(env_));}return;}
  if(auto x=std::dynamic_pointer_cast<ast::Function>(s)){auto closure=env_;auto f=callable(x->name,x->params.size(),x->params.size(),[this,closure,x](const std::vector<Value>&a,SourcePos)->Value{auto local=std::make_shared<Environment>(closure);for(std::size_t i=0;i<a.size();++i)local->define(x->params[i],a[i]);try{execute_block(x->body,local);}catch(ReturnSignal&r){return r.value;}return {};});env_->set(x->name,f);return;}
  if(auto x=std::dynamic_pointer_cast<ast::Type>(s)){auto t=std::make_shared<TypeData>();t->name=x->name;t->fields=x->fields;t->closure=env_;for(auto&m:x->methods)t->methods[m->name]=m;env_->set(x->name,t);return;}if(auto x=std::dynamic_pointer_cast<ast::Give>(s))throw ReturnSignal{evaluate(x->value)};if(auto x=std::dynamic_pointer_cast<ast::Try>(s)){try{execute_block(x->body,std::make_shared<Environment>(env_));}catch(const RuntimeFailure&e){auto local=std::make_shared<Environment>(env_);local->define(x->error_name,std::make_shared<ErrorData>(e.error()));execute_block(x->else_block,local);}return;}if(auto x=std::dynamic_pointer_cast<ast::Fail>(s)){auto v=evaluate(x->value);runtime_fail(x->pos,v.text(),"UserError");}throw Error(s->pos,"This statement is not implemented.");
}

std::shared_ptr<ModuleData> Interpreter::run_module(const ast::Module&m){
  if(m.builtin)return builtin_module(m.name);if(m.native)return load_native_module(m);auto old=env_;auto old_source=source_;auto local=std::make_shared<Environment>();env_=local;source_=m.path;install_builtins(local);
  for(auto&s:m.statements)if(auto u=std::dynamic_pointer_cast<ast::Use>(s)){auto d=modules_.find(u->name);if(d==modules_.end())throw Error(u->pos,"Module '"+u->name+"' was not loaded.");if(u->name=="file"||u->name=="path"||u->name=="time"||u->name=="math"||u->name=="random"||u->name=="os"||combined_platform_builtin(u->name))local->define(u->name,d->second);else for(auto&[n,v]:d->second->exports){if(local->has(n))throw Error(u->pos,"The name '"+n+"' is provided by more than one module.");local->define(n,v);}}
  try{for(auto&s:m.statements)execute(s);}catch(...){env_=old;source_=old_source;throw;}auto out=std::make_shared<ModuleData>();out->name=m.name;for(auto&s:m.statements){std::string n;if(auto t=std::dynamic_pointer_cast<ast::Type>(s))n=t->name;else if(auto f=std::dynamic_pointer_cast<ast::Function>(s))n=f->name;else if(auto a=std::dynamic_pointer_cast<ast::Assign>(s))if(auto v=std::dynamic_pointer_cast<ast::Variable>(a->target))n=v->name;if(!n.empty()&&n[0]!='_')out->exports[n]=local->get(n,s->pos);}env_=old;source_=old_source;return out;
}

void Interpreter::run_project(const ast::Program&p){modules_.clear();for(auto&m:p.modules){auto value=run_module(m);modules_[m.name]=value;if(m.name==p.entry){env_=std::make_shared<Environment>();install_builtins(env_);for(auto&[n,v]:value->exports)env_->define(n,v);}}}
void Interpreter::run(const ast::Program& p){if(!p.modules.empty()){run_project(p);return;}for(auto&s:p.statements)execute(s);}

} // namespace s