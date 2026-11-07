#pragma once
// Small C++20 runtime for the SE-written bootstrap compiler.
// This header is NOT the production SE runtime. See selfhost/README.md.
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <numbers>
#include <numeric>
#include <regex>
#include <random>
#include <thread>
#include <system_error>
#include <cstdint>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <functional>
#include <memory>
#include <future>
#include <tuple>
#include <variant>
#include <optional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>
#include "se_list.hpp"

// Sized collection allocation mirrors the production SE 0.8 bounded-memory
// rule. Check both dimensions and the product before allocating storage.
inline std::size_t se_sized_extent(std::int64_t size) {
  if (size < 0 || size > 1000000)
    throw std::runtime_error("Sized array length must be from 0 to 1000000.");
  return static_cast<std::size_t>(size);
}
template<class T> SEList<T> se_sized_list(std::int64_t size) {
  return SEList<T>(se_sized_extent(size));
}
template<class T> SEList<SEList<T>> se_sized_matrix(std::int64_t rows,
                                                      std::int64_t columns) {
  const auto r = se_sized_extent(rows);
  const auto c = se_sized_extent(columns);
  if (r != 0 && c > 1000000 / r)
    throw std::runtime_error("Sized array exceeds 1000000 elements.");
  SEList<SEList<T>> result(r);
  for (auto& row : result) row = SEList<T>(c);
  return result;
}

// Global binding storage defers initialization until its original statement.
// Functions can read and mutate existing top-level bindings without changing
// startup ordering (input and I/O still execute inside main's error handler).
template<class T> struct SEGlobal {
  std::optional<T> storage;
  template<class U> SEGlobal& operator=(U&& value) {
    storage = std::forward<U>(value);
    return *this;
  }
  T& get() {
    if (!storage) throw std::runtime_error("Global variable used before initialization.");
    return *storage;
  }
};
template<class T> T& se_implicit_call(SEGlobal<T>& value) { return value.get(); }

// Production SE automatically evaluates zero-parameter function values.
// Keep non-callable values (including mutable collections) as references, and
// preserve function values that require one or more explicit arguments.
template<class T> decltype(auto) se_implicit_call(T&& value) {
  if constexpr (std::is_invocable_v<T&&>)
    return std::forward<T>(value)();
  else
    return std::forward<T>(value);
}

// Recursive callable with value ownership; the body receives the current
// callable rather than retaining a reference to a local function variable.
template<class F> struct SEClosure {
  F body;
  template<class... A> requires std::is_invocable_v<F&, SEClosure&, A...>
  decltype(auto) operator()(A&&... args) {
    return body(*this, std::forward<A>(args)...);
  }
};
template<class F> auto se_closure(F body) { return SEClosure<F>{std::move(body)}; }
template<class T> auto se_cell(T&& value) {
  return std::make_shared<std::decay_t<T>>(std::forward<T>(value));
}

struct SEError {
  std::string message;
  std::string kind = "Error";
};
using SEAny = std::variant<std::monostate,std::int64_t,double,bool,std::string,SEBytes>;
inline std::ostream& operator<<(std::ostream& out,const SEError& error){
  return out<<error.message;
}
struct SEDuration {
  std::int64_t milliseconds;
  bool operator==(const SEDuration&) const = default;
};
struct SETime {
  std::chrono::system_clock::time_point point;
};
struct SEFile {
  std::fstream stream;
  bool closed=false;
  std::string read(){
    if(closed||!stream.is_open())throw std::runtime_error("This file is closed.");
    stream.clear();stream.seekg(0);
    return std::string(std::istreambuf_iterator<char>(stream),{});
  }
  void write(const std::string& value){
    if(closed||!stream.is_open())throw std::runtime_error("This file is closed.");
    stream.clear();stream.seekp(0,std::ios::end);stream<<value;stream.flush();
    if(!stream)throw std::runtime_error("Writing the file failed.");
  }
  void close(){if(!closed&&stream.is_open())stream.close();closed=true;}
};
inline SEFile se_file_open(const std::string& path){
  SEFile file;
  file.stream.open(path,std::ios::in|std::ios::out|std::ios::binary);
  if(!file.stream.is_open())throw std::runtime_error("Could not open '"+path+"'.");
  return file;
}
inline std::string se_time_iso(const SETime& value){
  const auto raw=std::chrono::system_clock::to_time_t(value.point);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm,&raw);
#else
  gmtime_r(&raw,&tm);
#endif
  std::ostringstream out;
  out<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}
inline std::ostream& operator<<(std::ostream& out,const SETime& value){
  return out<<se_time_iso(value);
}
inline std::ostream& operator<<(std::ostream& out,const SEFile&){return out<<"<file>";}
inline SETime se_time_now(){return SETime{std::chrono::system_clock::now()};}
inline SETime se_time_from_unix(std::int64_t value){
  return SETime{std::chrono::system_clock::from_time_t(static_cast<std::time_t>(value))};
}
inline std::int64_t se_time_unix(const SETime& value){
  return static_cast<std::int64_t>(std::chrono::system_clock::to_time_t(value.point));
}
inline std::ostream& operator<<(std::ostream& out,const SEDuration& d){
  return out<<d.milliseconds<<"ms";
}
inline SEDuration se_duration(const std::string& source){
  std::size_t i=0;
  if(source.empty() || !std::isdigit(static_cast<unsigned char>(source[0])))
    throw std::runtime_error("Invalid SE Duration literal");
  while(i<source.size() && std::isdigit(static_cast<unsigned char>(source[i])))++i;
  std::int64_t n=0;
  try{n=std::stoll(source.substr(0,i));}
  catch(const std::exception&){throw std::runtime_error("SE Duration out of range");}
  const auto unit=source.substr(i);
  std::int64_t factor=0;
  if(unit=="ms")factor=1;
  else if(unit=="s")factor=1000;
  else if(unit=="min")factor=60000;
  else throw std::runtime_error("Unsupported SE Duration unit: "+unit);
  if(n>std::numeric_limits<std::int64_t>::max()/factor)
    throw std::runtime_error("SE Duration overflow");
  return SEDuration{n*factor};
}
inline void se_wait(SEDuration d){
  if(d.milliseconds<0)throw std::runtime_error("wait requires nonnegative Duration");
  std::this_thread::sleep_for(std::chrono::milliseconds(d.milliseconds));
}
inline std::string se_platform(){
#ifdef _WIN32
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#else
  return "linux";
#endif
}
inline std::string se_cwd(){return std::filesystem::current_path().string();}
template<class... Parts> inline std::string se_path_join(const Parts&... parts){
  std::filesystem::path joined;
  ((joined /= std::filesystem::path(parts)),...);
  return joined.string();
}
inline std::string se_path_name(const std::string& path){
  return std::filesystem::path(path).filename().string();
}
inline std::string se_path_ext(const std::string& path){
  return std::filesystem::path(path).extension().string();
}
inline std::string se_path_parent(const std::string& path){
  return std::filesystem::path(path).parent_path().string();
}
inline bool se_path_exists(const std::string& path){
  std::error_code error; return std::filesystem::exists(path,error);
}
inline bool se_path_is_file(const std::string& path){
  std::error_code error; return std::filesystem::is_regular_file(path,error);
}
inline bool se_path_is_dir(const std::string& path){
  std::error_code error; return std::filesystem::is_directory(path,error);
}
inline std::mt19937_64& se_random_engine(){
  static thread_local std::mt19937_64 engine{std::random_device{}()};
  return engine;
}
inline std::int64_t se_random_int(std::int64_t first,std::int64_t last){
  if(first>last)throw std::runtime_error("random.int range is reversed");
  return std::uniform_int_distribution<std::int64_t>(first,last)(se_random_engine());
}
inline double se_random_num(){
  return std::generate_canonical<double,53>(se_random_engine());
}

inline std::string se_string(const std::string& value) { return value; }
inline std::string se_string(const char* value) { return std::string(value); }
inline std::string se_string(bool value) { return value ? "true" : "false"; }
inline bool se_bool(bool value) { return value; }
inline bool se_bool(std::string value) {
  auto begin=value.find_first_not_of(" \t\n\r\f\v");
  if(begin==std::string::npos)throw std::runtime_error("Could not convert empty Text to Bool. Use true or false.");
  auto end=value.find_last_not_of(" \t\n\r\f\v");
  value=value.substr(begin,end-begin+1);
  std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
  if(value=="true")return true;
  if(value=="false")return false;
  throw std::runtime_error("Could not convert Text to Bool. Use true or false.");
}
template<class N> inline std::string se_string(N value) {
  std::ostringstream out;
  out << value;
  return out.str();
}
inline std::string se_string(const SEAny& value){
  return std::visit([](const auto& item)->std::string{using T=std::decay_t<decltype(item)>;if constexpr(std::is_same_v<T,std::monostate>)return "None";else if constexpr(std::is_same_v<T,SEBytes>)return "<bytes "+std::to_string(item.size())+">";else return se_string(item);},value);
}
template<class T> inline void se_print_value(std::ostream& stream, const T& value) {
  stream << std::boolalpha << value;
}
inline void se_print_value(std::ostream& stream,double value){
  const auto precision=stream.precision();
  stream<<std::setprecision(12)<<std::defaultfloat<<value;
  stream.precision(precision);
}
template<class T> inline void se_print_value(std::ostream& stream, const SEList<T>& values) {
  stream << '[';
  for (std::size_t i=0;i<values.size();++i) {
    if(i)stream<<", ";
    se_print_value(stream, values[i]);
  }
  stream << ']';
}
inline void se_print_value(std::ostream& stream,const SEBytes& values){stream<<"<bytes "<<values.size()<<">";}
inline void se_print_value(std::ostream& stream,const SEAny& value){
  std::visit([&](const auto& item){using T=std::decay_t<decltype(item)>;if constexpr(std::is_same_v<T,std::monostate>)stream<<"None";else se_print_value(stream,item);},value);
}
inline std::string se_dynamic_type(const SEAny& value){
  return std::visit([](const auto& item)->std::string{using T=std::decay_t<decltype(item)>;if constexpr(std::is_same_v<T,std::monostate>)return "None";else if constexpr(std::is_same_v<T,std::int64_t>)return "Int";else if constexpr(std::is_same_v<T,double>)return "Num";else if constexpr(std::is_same_v<T,bool>)return "Bool";else if constexpr(std::is_same_v<T,std::string>)return "Text";else return "Bytes";},value);
}
template<class T> inline std::string se_dynamic_type(const T&){using V=std::decay_t<T>;if constexpr(std::is_same_v<V,bool>)return "Bool";else if constexpr(std::is_integral_v<V>)return "Int";else if constexpr(std::is_floating_point_v<V>)return "Num";else if constexpr(std::is_same_v<V,std::string>||std::is_convertible_v<V,const char*>)return "Text";else if constexpr(std::is_same_v<V,SEBytes>)return "Bytes";else return "Unknown";}
inline bool se_dynamic_is(const SEAny& value,const std::string& type){return se_dynamic_type(value)==type;}
template<class T> inline bool se_dynamic_is(const T& value,const std::string& type){return se_dynamic_type(value)==type;}
template<class T> inline void se_say(const T& value) {
  se_print_value(std::cout, value);
  std::cout << '\n';
}
template<class A,class B> auto se_add(const A& a,const B& b) {
  if constexpr(std::is_constructible_v<std::string, A> || std::is_constructible_v<std::string, B>)
    return se_string(a)+se_string(b);
  else return a+b;
}
template<class A,class B> auto se_operator_add(const A& a,const B& b){return se_add(a,b);}
template<class A,class B> auto se_operator_sub(const A& a,const B& b){return a-b;}
template<class A,class B> auto se_operator_mul(const A& a,const B& b){return a*b;}
template<class A,class B> auto se_operator_div(const A& a,const B& b){if(b==B{})throw std::runtime_error("Division by zero.");return static_cast<double>(a)/static_cast<double>(b);}
template<class A,class B> auto se_operator_mod(const A& a,const B& b){if(b==B{})throw std::runtime_error("Modulo by zero.");if constexpr(std::is_floating_point_v<A>||std::is_floating_point_v<B>)return std::fmod(static_cast<double>(a),static_cast<double>(b));else return a%b;}
template<class A,class B> bool se_operator_eq(const A& a,const B& b){return a==b;}
template<class A,class B> bool se_operator_ne(const A& a,const B& b){return a!=b;}
template<class A,class B> bool se_operator_lt(const A& a,const B& b){return a<b;}
template<class A,class B> bool se_operator_le(const A& a,const B& b){return a<=b;}
template<class A,class B> bool se_operator_gt(const A& a,const B& b){return a>b;}
template<class A,class B> bool se_operator_ge(const A& a,const B& b){return a>=b;}
inline std::int64_t se_int(const std::string& v) {
  std::size_t end=0;
  auto n=std::stoll(v,&end);
  if(end!=v.size())throw std::runtime_error("Invalid Int: "+v);
  return static_cast<std::int64_t>(n);
}
template<class T> inline std::int64_t se_int(const T& v) { return static_cast<std::int64_t>(v); }
inline std::int64_t se_int(const SEAny& v){if(auto p=std::get_if<std::int64_t>(&v))return *p;if(auto p=std::get_if<double>(&v))return static_cast<std::int64_t>(*p);if(auto p=std::get_if<bool>(&v))return *p?1:0;if(auto p=std::get_if<std::string>(&v))return se_int(*p);throw std::runtime_error("Cannot convert dynamic value to Int.");}
inline double se_num(const std::string& v) {
  std::size_t end=0;
  double n=std::stod(v,&end);
  if(end!=v.size()||!std::isfinite(n))throw std::runtime_error("Invalid Num: "+v);
  return n;
}
template<class T> inline double se_num(const T& v) {return static_cast<double>(v);}
inline double se_num(const SEAny& v){if(auto p=std::get_if<std::int64_t>(&v))return static_cast<double>(*p);if(auto p=std::get_if<double>(&v))return *p;if(auto p=std::get_if<bool>(&v))return *p?1.0:0.0;if(auto p=std::get_if<std::string>(&v))return se_num(*p);throw std::runtime_error("Cannot convert dynamic value to Num.");}
// SE char(codepoint): encode a Unicode scalar as UTF-8.
inline std::string se_char(std::int64_t value) {
  if(value<0 || value>0x10ffff || (value>=0xd800 && value<=0xdfff))
    throw std::runtime_error("Invalid Unicode scalar for char");
  const auto cp=static_cast<std::uint32_t>(value);
  std::string out;
  if(cp<=0x7f)out+=static_cast<char>(cp);
  else if(cp<=0x7ff){
    out+=static_cast<char>(0xc0|(cp>>6));
    out+=static_cast<char>(0x80|(cp&0x3f));
  }else if(cp<=0xffff){
    out+=static_cast<char>(0xe0|(cp>>12));
    out+=static_cast<char>(0x80|((cp>>6)&0x3f));
    out+=static_cast<char>(0x80|(cp&0x3f));
  }else{
    out+=static_cast<char>(0xf0|(cp>>18));
    out+=static_cast<char>(0x80|((cp>>12)&0x3f));
    out+=static_cast<char>(0x80|((cp>>6)&0x3f));
    out+=static_cast<char>(0x80|(cp&0x3f));
  }
  return out;
}
inline std::string se_char(const std::string& value) {
  if(value.empty())throw std::runtime_error("char needs exactly one Unicode character.");
  const auto first=static_cast<unsigned char>(value[0]);
  std::size_t width=first<=0x7f?1:first>=0xc2&&first<=0xdf?2:first>=0xe0&&first<=0xef?3:first>=0xf0&&first<=0xf4?4:0;
  if(width==0||value.size()!=width)throw std::runtime_error("char needs exactly one Unicode character.");
  for(std::size_t i=1;i<width;++i)if((static_cast<unsigned char>(value[i])&0xc0)!=0x80)throw std::runtime_error("char needs valid UTF-8.");
  if(width==3){auto second=static_cast<unsigned char>(value[1]);if((first==0xe0&&second<0xa0)||(first==0xed&&second>=0xa0))throw std::runtime_error("char needs valid UTF-8.");}
  if(width==4){auto second=static_cast<unsigned char>(value[1]);if((first==0xf0&&second<0x90)||(first==0xf4&&second>0x8f))throw std::runtime_error("char needs valid UTF-8.");}
  return value;
}

// Standalone JSON facilities for the SE compiler's own String/List[String]
// AST wire format. These intentionally reject other JSON value shapes.
inline std::string se_json_stringify(const std::string& value){
  constexpr char hex[]="0123456789abcdef";
  std::string result="\"";
  for(unsigned char c:value){
    switch(c){
      case '"':result+="\\\"";break;
      case '\\':result+="\\\\";break;
      case '\n':result+="\\n";break;
      case '\r':result+="\\r";break;
      case '\t':result+="\\t";break;
      case '\b':result+="\\b";break;
      case '\f':result+="\\f";break;
      default:
        if(c<0x20){
          result+="\\u00";
          result+=hex[c>>4];
          result+=hex[c&0xf];
        }else result+=static_cast<char>(c);
    }
  }
  return result+"\"";
}
inline std::string se_json_stringify(const SEList<std::string>& values){
  std::string result="[";
  for(const auto& value:values){
    if(result.size()!=1)result+=",";
    result+=se_json_stringify(value);
  }
  return result+"]";
}
// Pretty-print the subset of JSON values supported by the independent
// bootstrap runtime, using the production JSON formatter's two-space indent.
inline std::string se_json_pretty(const std::string& value) {
  return se_json_stringify(value);
}
inline std::string se_json_pretty(const SEList<std::string>& values) {
  if (values.empty()) return "[]";
  std::string out="[\n";
  for (std::size_t i=0;i<values.size();++i) {
    out+="  "+se_json_stringify(values[i]);
    out+=(i+1==values.size()?"\n":",\n");
  }
  return out+"]";
}
inline std::string se_json_pretty(bool value) {
  return value ? "true" : "false";
}
inline std::string se_json_pretty(std::int64_t value) {
  return std::to_string(value);
}
inline std::string se_json_pretty(double value) {
  if (!std::isfinite(value))
    throw std::runtime_error("Non-finite numbers cannot be encoded as JSON.");
  std::ostringstream out;
  out << std::setprecision(15) << value;
  return out.str();
}
template<class T> requires (!std::is_same_v<T,std::string>)
std::string se_json_pretty(const SEList<T>& values) {
  if (values.empty()) return "[]";
  std::string out="[\n";
  for(std::size_t i=0;i<values.size();++i) {
    out+="  "+se_json_pretty(values[i]);
    out+=(i+1==values.size()?"\n":",\n");
  }
  return out+"]";
}
inline SEList<std::string> se_json_parse(const std::string& json){
  std::size_t pos=0;
  auto whitespace=[&](){
    while(pos<json.size() &&
          (json[pos]==' ' || json[pos]=='\n' || json[pos]=='\t' || json[pos]=='\r'))
      ++pos;
  };
  auto digit=[&]()->std::uint32_t{
    if(pos==json.size())throw std::runtime_error("Invalid JSON Unicode escape");
    auto c=json[pos++];
    if(c>='0'&&c<='9')return static_cast<std::uint32_t>(c-'0');
    if(c>='a'&&c<='f')return static_cast<std::uint32_t>(c-'a'+10);
    if(c>='A'&&c<='F')return static_cast<std::uint32_t>(c-'A'+10);
    throw std::runtime_error("Invalid JSON Unicode escape");
  };
  auto scalar=[&]()->std::uint32_t{
    std::uint32_t cp=0;
    for(int i=0;i<4;i++)cp=(cp<<4)|digit();
    return cp;
  };
  auto string_value=[&]()->std::string{
    if(pos>=json.size()||json[pos++]!='"')
      throw std::runtime_error("Expected JSON string");
    std::string out;
    while(pos<json.size()){
      unsigned char c=static_cast<unsigned char>(json[pos++]);
      if(c=='"')return out;
      if(c<0x20)throw std::runtime_error("Control character in JSON string");
      if(c!='\\'){out+=static_cast<char>(c);continue;}
      if(pos==json.size())throw std::runtime_error("Unterminated JSON escape");
      const auto escape=json[pos++];
      switch(escape){
        case '"':out+='"';break;
        case '\\':out+='\\';break;
        case '/':out+='/';break;
        case 'n':out+='\n';break;
        case 'r':out+='\r';break;
        case 't':out+='\t';break;
        case 'b':out+='\b';break;
        case 'f':out+='\f';break;
        case 'u':{
          std::uint32_t cp=scalar();
          if(cp>=0xd800&&cp<=0xdbff){
            if(pos+2>json.size()||json[pos++]!='\\'||json[pos++]!='u')
              throw std::runtime_error("Unpaired JSON high surrogate");
            const auto low=scalar();
            if(low<0xdc00||low>0xdfff)
              throw std::runtime_error("Invalid JSON surrogate pair");
            cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);
          }else if(cp>=0xdc00&&cp<=0xdfff)
            throw std::runtime_error("Unpaired JSON low surrogate");
          out+=se_char(static_cast<std::int64_t>(cp));
          break;
        }
        default:throw std::runtime_error("Unknown JSON escape");
      }
    }
    throw std::runtime_error("Unterminated JSON string");
  };
  whitespace();
  if(pos>=json.size()||json[pos++]!='[')
    throw std::runtime_error("Only JSON arrays of strings are supported");
  whitespace();
  SEList<std::string> out;
  if(pos<json.size()&&json[pos]==']')++pos;
  else{
    while(true){
      whitespace();
      out.push_back(string_value());
      whitespace();
      if(pos>=json.size())throw std::runtime_error("Unterminated JSON array");
      if(json[pos]==']'){++pos;break;}
      if(json[pos++]!=',')throw std::runtime_error("Expected comma in JSON array");
    }
  }
  whitespace();
  if(pos!=json.size())throw std::runtime_error("Unexpected trailing JSON text");
  return out;
}

inline SEList<SEList<std::string>> se_csv_parse(const std::string& source){
  SEList<SEList<std::string>> rows;
  if(source.empty())return rows;
  SEList<std::string> row;
  std::string field;
  bool quoted=false,closed=false,field_present=false;
  for(std::size_t i=0;i<source.size();++i){
    const char c=source[i];
    if(quoted){
      if(c=='"'){
        if(i+1<source.size()&&source[i+1]=='"'){field+='"';++i;}
        else{quoted=false;closed=true;}
      }else field+=c;
      continue;
    }
    if(closed&&c!=','&&c!='\r'&&c!='\n')throw std::runtime_error("Unexpected character after quoted CSV field.");
    if(c=='"'){
      if(!field.empty()||closed)throw std::runtime_error("Quote inside unquoted CSV field.");
      quoted=true;field_present=true;continue;
    }
    if(c==','){row.push_back(field);field.clear();closed=false;field_present=false;continue;}
    if(c=='\n'||c=='\r'){
      row.push_back(field);field.clear();closed=false;field_present=false;rows.push_back(row);row=SEList<std::string>();
      if(c=='\r'&&i+1<source.size()&&source[i+1]=='\n')++i;
      continue;
    }
    field+=c;field_present=true;
  }
  if(quoted)throw std::runtime_error("Unterminated quoted CSV field.");
  if(field_present||!row.empty()||source.back()==','){
    if(source.back()=='\n'||source.back()=='\r')return rows;
    row.push_back(field);rows.push_back(row);
  }
  return rows;
}
inline std::string se_csv_stringify(const SEList<SEList<std::string>>& rows){
  std::string output;
  for(std::size_t r=0;r<rows.size();++r){
    if(r)output+='\n';
    for(std::size_t c=0;c<rows[r].size();++c){
      if(c)output+=',';
      const auto& field=rows[r][c];
      if(field.find_first_of(",\"\r\n")==std::string::npos){output+=field;continue;}
      output+='"';for(char ch:field){if(ch=='"')output+='"';output+=ch;}output+='"';
    }
  }
  return output;
}

inline std::string se_getenv(const std::string& key) {
  const char* raw=std::getenv(key.c_str());
  return raw ? std::string(raw) : std::string();
}
inline bool se_has_env(const std::string& key){return std::getenv(key.c_str())!=nullptr;}

inline std::string se_ask(const std::string& question) {
  std::cout<<question;
  std::cout.flush();
  std::string out;
  if(!std::getline(std::cin,out))throw std::runtime_error("Unable to read input");
  return out;
}
inline SEList<std::int64_t> se_range(std::int64_t start,std::int64_t finish) {
  SEList<std::int64_t> out;
  auto step=start<=finish?std::int64_t{1}:std::int64_t{-1};
  for(auto n=start;;n+=step) {
    out.push_back(n);
    if(n==finish)break;
  }
  return out;
}
template<class T> bool se_contains(const SEList<T>& items,const T& target) {
  return std::find(items.begin(),items.end(),target)!=items.end();
}
template<class T> T se_first(const SEList<T>& items){if(items.empty())throw std::runtime_error("collections.first needs a non-empty List.");return items.front();}
template<class T> T se_last(const SEList<T>& items){if(items.empty())throw std::runtime_error("collections.last needs a non-empty List.");return items.back();}
template<class T> SEList<T> se_unique(const SEList<T>& items){SEList<T> out;for(const auto& value:items)if(!se_contains(out,value))out.push_back(value);return out;}
inline bool se_contains(const std::string& text,const std::string& needle) {
  return text.find(needle)!=std::string::npos;
}
inline std::string se_upper(std::string s){
  for(auto& c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}
inline std::string se_lower(std::string s){
  for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
inline bool se_starts(const std::string& value,const std::string& prefix){return value.starts_with(prefix);}
inline bool se_ends(const std::string& value,const std::string& suffix){return value.ends_with(suffix);}
inline std::string se_replace(std::string value,const std::string& from,const std::string& to){
  if(from.empty())return value;
  std::size_t position=0;
  while((position=value.find(from,position))!=std::string::npos){
    value.replace(position,from.size(),to);position+=to.size();
  }
  return value;
}
inline std::string se_trim(const std::string& s){
  auto i=s.find_first_not_of(" \r\n\t");
  if(i==std::string::npos)return {};
  auto j=s.find_last_not_of(" \r\n\t");
  return s.substr(i,j-i+1);
}
inline SEList<std::string> se_split(const std::string& s,const std::string& delim){
  SEList<std::string> result;
  if(delim.empty()){
    for(char c:s)result.emplace_back(1,c);
    return result;
  }
  std::size_t from=0;
  while(true){auto pos=s.find(delim,from);if(pos==std::string::npos){result.push_back(s.substr(from));break;}
    result.push_back(s.substr(from,pos-from));from=pos+delim.size();}
  return result;
}
inline std::string se_repeat(const std::string& s,std::int64_t n){
  if(n<0||n>1000000)throw std::runtime_error("Invalid repeat count");
  std::string out;
  for(std::int64_t i=0;i<n;++i)out+=s;
  return out;
}
inline std::string se_join(const SEList<std::string>& parts,const std::string& delim){
  std::string out;
  for(std::size_t i=0;i<parts.size();++i){if(i)out+=delim;out+=parts[i];}
  return out;
}
inline std::string se_base64_encode(const std::string& input){
  static constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string output;
  output.reserve(((input.size()+2)/3)*4);
  int value=0,bits=-6;
  for(unsigned char byte:input){
    value=(value<<8)+byte;
    bits+=8;
    while(bits>=0){output.push_back(alphabet[(value>>bits)&0x3F]);bits-=6;}
  }
  if(bits>-6)output.push_back(alphabet[((value<<8)>>(bits+8))&0x3F]);
  while(output.size()%4)output.push_back('=');
  return output;
}
inline std::string se_base64_decode(const std::string& input){
  static constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::array<int,256> table{};
  table.fill(-1);
  for(int i=0;i<64;++i)table[static_cast<unsigned char>(alphabet[i])]=i;
  std::string output;
  int value=0,bits=-8;
  for(unsigned char byte:input){
    if(std::isspace(byte))continue;
    if(byte=='=')break;
    const int digit=table[byte];
    if(digit<0)throw std::runtime_error("Invalid Base64 in SE bootstrap AST.");
    value=(value<<6)+digit;
    bits+=6;
    if(bits>=0){output.push_back(static_cast<char>((value>>bits)&0xFF));bits-=8;}
  }
  return output;
}
template<class T> inline SEList<T> se_take(const SEList<T>& values,std::int64_t amount){
  if(amount<0)throw std::runtime_error("collections.take requires nonnegative count");
  const auto n=std::min<std::size_t>(values.size(),static_cast<std::size_t>(amount));
  return {values.begin(),values.begin()+static_cast<std::ptrdiff_t>(n)};
}
template<class T> inline SEList<T> se_drop(const SEList<T>& values,std::int64_t amount){
  if(amount<0)throw std::runtime_error("collections.drop requires nonnegative count");
  const auto n=std::min<std::size_t>(values.size(),static_cast<std::size_t>(amount));
  return {values.begin()+static_cast<std::ptrdiff_t>(n),values.end()};
}
template<class T> inline SEList<T> se_slice(const SEList<T>& items,std::int64_t first,std::int64_t last){
  if(first<0 || last<0)throw std::runtime_error("collections.slice expects nonnegative indices");
  const auto left=std::min<std::size_t>(items.size(),static_cast<std::size_t>(first));
  const auto right=std::min<std::size_t>(items.size(),static_cast<std::size_t>(last));
  if(right<left)return {};
  return {items.begin()+static_cast<std::ptrdiff_t>(left),
          items.begin()+static_cast<std::ptrdiff_t>(right)};
}
template<class T,class Pred> inline SEList<T> se_filter(const SEList<T>& items,Pred predicate){
  SEList<T> result;
  for(const auto& item:items)if(predicate(item))result.push_back(item);
  return result;
}
template<class T,class F> inline auto se_transform(const SEList<T>& items,F function){
  using Output=std::decay_t<decltype(function(std::declval<T>()))>;
  SEList<Output> out;
  out.reserve(items.size());
  for(const auto& item:items)out.push_back(function(item));
  return out;
}
template<class T,class A,class F> inline A se_reduce(const SEList<T>& items,A state,F combine){
  for(const auto& item:items)state=combine(state,item);
  return state;
}
template<class T,class F> inline SEList<T> se_sort_with(SEList<T> values,F before){
  values=values.clone();
  std::stable_sort(values.begin(),values.end(),before);
  return values;
}
template<class T> inline SEList<T> se_sorted(SEList<T> items){items=items.clone();std::sort(items.begin(),items.end());return items;}
template<class T> inline SEList<T> se_reversed(SEList<T> items){items=items.clone();std::reverse(items.begin(),items.end());return items;}

template<class T, class... Ts> auto se_list(T first, Ts... rest){
  using Item=std::common_type_t<T,Ts...>;
  return SEList<Item>{static_cast<Item>(first),static_cast<Item>(rest)...};
}
inline SEList<std::string> se_list(){ return {}; }
template<class Tuple> auto se_list_ordered(Tuple&& tuple){return std::apply([](auto&&... values){return se_list(std::forward<decltype(values)>(values)...);},std::forward<Tuple>(tuple));}
inline std::string se_read(const std::string& path) {
  std::ifstream file(path,std::ios::binary);
  if(!file)throw std::runtime_error("Cannot read: "+path);
  return {std::istreambuf_iterator<char>(file),{}};
}
inline void se_write(const std::string& path,const std::string& data) {
  std::ofstream file(path,std::ios::binary);
  if(!file)throw std::runtime_error("Cannot write: "+path);
  file<<data;
}
inline void se_append(const std::string& path,const std::string& data) {
  std::ofstream file(path,std::ios::binary|std::ios::app);
  if(!file)throw std::runtime_error("Cannot append: "+path);
  file<<data;
}
inline SEList<SEList<std::string>> se_csv_read(const std::string& path){return se_csv_parse(se_read(path));}
inline void se_csv_write(const std::string& path,const SEList<SEList<std::string>>& rows){
  std::ofstream file(path,std::ios::binary|std::ios::trunc);
  if(!file)throw std::runtime_error("csv.write could not open '"+path+"'.");
  file<<se_csv_stringify(rows);file.flush();
  if(!file)throw std::runtime_error("csv.write failed.");
}
inline void se_file_copy(const std::string& source,const std::string& target){
  std::error_code error;
  std::filesystem::copy_file(source,target,std::filesystem::copy_options::overwrite_existing,error);
  if(error)throw std::runtime_error("file.copy: "+error.message());
}
inline void se_file_move(const std::string& source,const std::string& target){
  std::error_code error;std::filesystem::rename(source,target,error);
  if(error)throw std::runtime_error("file.move: "+error.message());
}
inline void se_file_copytree(const std::string& source,const std::string& target){
  std::error_code error;
  std::filesystem::copy(source,target,std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing,error);
  if(error)throw std::runtime_error("file.copytree: "+error.message());
}
inline void se_file_remove(const std::string& path){
  std::error_code error;std::filesystem::remove_all(path,error);
  if(error)throw std::runtime_error("file.remove: "+error.message());
}
inline void se_file_mkdir(const std::string& path){
  std::error_code error;std::filesystem::create_directories(path,error);
  if(error)throw std::runtime_error("file.mkdir: "+error.message());
}

// Ordered Map and Set use the same insertion-order semantics as SE runtime.
template<class T> struct SEMap {
  SEList<std::pair<std::string,T>> items;
  void put(const std::string& key, const T& value) {
    for(auto& pair:items)if(pair.first==key){pair.second=value;return;}
    items.emplace_back(key,value);
  }
  T& at(const std::string& key) {
    for(auto& pair:items)if(pair.first==key)return pair.second;
    throw std::runtime_error("Map has no key: "+key);
  }
  const T& at(const std::string& key) const {
    for(const auto& pair:items)if(pair.first==key)return pair.second;
    throw std::runtime_error("Map has no key: "+key);
  }
  std::size_t size() const {return items.size();}
  SEMap clone()const{SEMap out;for(const auto& item:items)out.items.push_back(item);return out;}
  bool operator==(const SEMap& other)const{
    if(items.size()!=other.items.size())return false;
    for(std::size_t i=0;i<items.size();++i){
      if(items[i].first!=other.items[i].first)return false;
      if constexpr(requires { items[i].second==other.items[i].second; }){if(!(items[i].second==other.items[i].second))return false;}
      else return false;
    }
    return true;
  }
};
template<class T> struct SESet {
  SEList<T> items;
  void add(const T& value) {
    if(std::find(items.begin(),items.end(),value)==items.end())items.push_back(value);
  }
  std::size_t size() const{return items.size();}
  SESet clone()const{SESet out;for(const auto& item:items)out.items.push_back(item);return out;}
  bool operator==(const SESet& other)const{
    if(items.size()!=other.items.size())return false;
    for(const auto& value:items)if(std::find(other.items.begin(),other.items.end(),value)==other.items.end())return false;
    return true;
  }
};
inline std::string se_module_help(const std::string& module){
  if(module=="shutil")return "members: help, copy, move, copytree, remove, mkdir";
  if(module=="typing")return "members: help, type_of, is, cast";
  if(module=="cookie")return "members: help, parse, set";
  if(module=="cors")return "members: help, allow_origin, preflight";
  if(module=="template")return "members: help, escape, render";
  if(module=="static")return "members: help, mime, read";
  if(module=="upload")return "members: help, save";
  if(module=="dotenv")return "members: help, parse, get";
  if(module=="config")return "members: help, parse, get, get_int, get_bool, section, merge";
  if(module=="itertools")return "members: help, chain, flatten, chunked, take, drop, windows, cycle, pairs, unique";
  if(module=="glob")return "members: help, match, find";
  if(module=="hashlib")return "members: help, compare, to_hex";
  if(module=="datetime")return "members: help, now, timestamp, from_timestamp, format, add_seconds";
  if(module=="iter")return "members: help, range";
  if(module=="functools")return "members: help, partial, reduce, map, filter";
  if(module=="enum")return "members: help, make, name, value, has";
  if(module=="math")return "members: help, pi, e, sqrt, pow, abs, floor, ceil, round, sin, cos, tan, log, log10, exp, hypot, min, max";
  if(module=="time")return "members: help, now, unix, from_unix, iso";
  if(module=="text")return "members: help, split, trim, contains, repeat, join";
  if(module=="collections")return "members: help, contains, first, last, unique, keys, values, sort, sort_by, sort_by_desc, reverse, take, drop, slice, filter, map, reduce, sort_with";
  if(module=="series")return "members: help, sum, mean, min, max, diff, lag, moving_average, cumulative_sum, returns, normalize";
  if(module=="array")return "members: help, sum, mean, slice";
  if(module=="matrix")return "members: help, transpose, multiply, dot";
  if(module=="linear")return "members: help, transpose, multiply, dot, add, subtract, scale, identity, determinant, inverse, norm, normalize";
  if(module=="probability")return "members: help, factorial, choose";
  if(module=="fraction")return "members: help, make, decimal";
  if(module=="complex")return "members: help, make, add, multiply, magnitude";
  if(module=="calculus")return "members: help, polynomial, derivative, integral";
  if(module=="units")return "members: help, convert, celsius_to_fahrenheit, fahrenheit_to_celsius";
  if(module=="table")return "members: help, column, row_count, select";
  if(module=="dataset")return "members: help, columns, row_count, select, split";
  if(module=="re")return "members: help, find_all, count, escape, groups, match, search, replace, split";
  if(module=="uuid")return "members: help, v4, valid";
  if(module=="path")return "members: help, join, name, ext, parent, exists, is_file, is_dir";
  if(module=="os")return "members: help, getenv, platform, cwd";
  if(module=="random")return "members: help, int, num";
  if(module=="json")return "members: help, parse, stringify";
  if(module=="file")return "members: help, read, write, append, open, copy, move, copytree, remove";
  if(module=="base64")return "members: help, encode, decode";
  if(module=="test")return "members: help, ok, equal, not_equal, fail";
  return "members: help";
}
inline std::string se_help_text(const std::string&){return "members: help, len, upper, lower";}
template<class T> inline std::string se_help_text(const SEList<T>&){return "members: help, len, add value, remove value";}
template<class T> inline std::string se_help_text(const SEMap<T>&){return "members: help, len, remove key; index with map[key]";}
template<class T> inline std::string se_help_text(const SESet<T>&){return "members: help, len, add value, remove value";}
inline std::string se_help_text(const SEError&){return "members: help, message, kind";}
inline std::string se_help_text(const SEFile&){return "members: help, read, write text, close";}
template<class T> inline std::string se_help_text(const T&){return "members: help";}
inline void se_test_ok(bool value){if(!value)throw std::runtime_error("test.ok failed");}
template<class A,class B> inline void se_test_equal(const A& actual,const B& expected){
  if constexpr(requires { actual==expected; }){
    if(!(actual==expected))throw std::runtime_error("test.equal failed");
  }else throw std::runtime_error("test.equal does not support these value types");
}
template<class A,class B> inline void se_test_not_equal(const A& actual,const B& expected){
  if constexpr(requires { actual==expected; }){
    if(actual==expected)throw std::runtime_error("test.not_equal failed");
  }else throw std::runtime_error("test.not_equal does not support these value types");
}
inline void se_test_fail(const std::string& message){throw std::runtime_error(message);}
template<class K,class V,class... Pairs> auto se_map(std::pair<K,V> first, Pairs... rest) {
  using Element=std::common_type_t<V,typename Pairs::second_type...>;
  SEMap<Element> out;
  out.put(se_string(first.first),static_cast<Element>(first.second));
  (out.put(se_string(rest.first),static_cast<Element>(rest.second)),...);
  return out;
}
inline SEMap<std::string> se_map(){return {};}
template<class K,class V> struct SEOrderedPair{K first;V second;};
template<class Tuple> auto se_ordered_pair(Tuple&& tuple){return std::apply([](auto&& key,auto&& value){return SEOrderedPair<std::decay_t<decltype(key)>,std::decay_t<decltype(value)>>{std::forward<decltype(key)>(key),std::forward<decltype(value)>(value)};},std::forward<Tuple>(tuple));}
template<class Tuple> auto se_map_ordered(Tuple&& tuple){return std::apply([](auto&&... pairs){return se_map(std::pair{pairs.first,pairs.second}...);},std::forward<Tuple>(tuple));}
template<class T,class... Values> auto se_set(T first,Values... rest){
  using Element=std::common_type_t<T,Values...>;
  SESet<Element> out;
  out.add(static_cast<Element>(first));
  (out.add(static_cast<Element>(rest)),...);
  return out;
}
inline SESet<std::string> se_set(){return {};}
template<class Tuple> auto se_set_ordered(Tuple&& tuple){return std::apply([](auto&&... values){return se_set(std::forward<decltype(values)>(values)...);},std::forward<Tuple>(tuple));}
template<class T> inline bool se_contains(const SESet<T>& values,const T& item){
  return std::find(values.items.begin(),values.items.end(),item)!=values.items.end();
}
template<class T> inline bool se_contains(const SEMap<T>& values,const std::string& key){
  for(const auto& item:values.items)if(item.first==key)return true;
  return false;
}
template<class T> inline SEList<T> se_copy_shallow(const SEList<T>& value){return value.clone();}
template<class T> inline SEMap<T> se_copy_shallow(const SEMap<T>& value){return value.clone();}
template<class T> inline SESet<T> se_copy_shallow(const SESet<T>& value){return value.clone();}
template<class T> inline T se_copy_deep(const T& value){return value;}
template<class T> inline SEList<T> se_copy_deep(const SEList<T>& value){SEList<T> out;for(const auto& item:value)out.push_back(se_copy_deep(item));return out;}
template<class T> inline SEMap<T> se_copy_deep(const SEMap<T>& value){SEMap<T> out;for(const auto& item:value.items)out.put(item.first,se_copy_deep(item.second));return out;}
template<class T> inline SESet<T> se_copy_deep(const SESet<T>& value){SESet<T> out;for(const auto& item:value.items)out.add(se_copy_deep(item));return out;}
template<class T> inline SEList<std::string> se_keys(const SEMap<T>& values){SEList<std::string> out;for(const auto& item:values.items)out.push_back(item.first);return out;}
template<class T> inline SEList<T> se_values(const SEMap<T>& values){SEList<T> out;for(const auto& item:values.items)out.push_back(item.second);return out;}
template<class T> inline SEList<SEMap<T>> se_sort_by(SEList<SEMap<T>> values,const std::string& key){values=values.clone();std::stable_sort(values.begin(),values.end(),[&](const auto& a,const auto& b){return a.at(key)<b.at(key);});return values;}
template<class T> inline SEList<SEMap<T>> se_sort_by_desc(SEList<SEMap<T>> values,const std::string& key){values=values.clone();std::stable_sort(values.begin(),values.end(),[&](const auto& a,const auto& b){return b.at(key)<a.at(key);});return values;}
template<class T> inline void se_print_value(std::ostream& stream,const SESet<T>& values){
  stream<<"set [";
  for(std::size_t i=0;i<values.items.size();++i){
    if(i)stream<<", ";
    se_print_value(stream,values.items[i]);
  }
  stream<<']';
}
template<class T> inline void se_print_value(std::ostream& stream,const SEMap<T>& values){
  stream<<'[';
  for(std::size_t i=0;i<values.items.size();++i){
    if(i)stream<<", ";
    stream<<'"'<<values.items[i].first<<"\": ";
    se_print_value(stream,values.items[i].second);
  }
  stream<<']';
}
template<class T> decltype(auto) se_index(SEList<T>& items,std::int64_t i){
  if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("List index out of bounds");
  return items.at(static_cast<std::size_t>(i));
}
template<class T> decltype(auto) se_index(const SEList<T>& items,std::int64_t i){
  if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("List index out of bounds");
  return items.at(static_cast<std::size_t>(i));
}
inline std::int64_t se_index(SEBytes& items,std::int64_t i){if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("Bytes index out of bounds");return items[static_cast<std::size_t>(i)];}
inline std::int64_t se_index(const SEBytes& items,std::int64_t i){if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("Bytes index out of bounds");return items[static_cast<std::size_t>(i)];}
template<class T> decltype(auto) se_index(SEMap<T>& items,const std::string& key){return items.at(key);}
template<class T> decltype(auto) se_index(const SEMap<T>& items,const std::string& key){return items.at(key);}
// Reads reject missing keys; assignment inserts a new map entry or updates
// an existing one while retaining insertion order, as the SE evaluator does.
template<class T,class V> void se_assign_index(SEList<T>& values,std::int64_t index,V&& value) {
  se_index(values,index)=std::forward<V>(value);
}
template<class V> void se_assign_index(SEBytes& values,std::int64_t index,V&& value){
  if(index<0||static_cast<std::size_t>(index)>=values.size())throw std::runtime_error("Bytes index out of bounds");
  const auto byte=static_cast<std::int64_t>(value);
  if(byte<0||byte>255)throw std::runtime_error("Bytes values must be from 0 to 255.");
  values[static_cast<std::size_t>(index)]=static_cast<std::uint8_t>(byte);
}
template<class T,class V> void se_assign_index(SEMap<T>& values,const std::string& key,V&& value) {
  values.put(key,std::forward<V>(value));
}
template<class T> inline void se_add_item(SEList<T>& values,const T& value){values.push_back(value);}
template<class T> inline void se_add_item(SESet<T>& values,const T& value){values.add(value);}
template<class T> inline bool se_remove_item(SEList<T>& values,const T& value){
  const auto it=std::find(values.begin(),values.end(),value);
  if(it==values.end())return false;
  values.erase(it); return true;
}
template<class T> inline bool se_remove_item(SESet<T>& values,const T& value){
  const auto it=std::find(values.items.begin(),values.items.end(),value);
  if(it==values.items.end())return false;
  values.items.erase(it); return true;
}
template<class T> inline bool se_remove_item(SEMap<T>& values,const std::string& key){
  for(auto it=values.items.begin();it!=values.items.end();++it)
    if(it->first==key){values.items.erase(it);return true;}
  return false;
}

// Math API ported from src/runtime/ecosystem.cpp.
inline double se_math_cbrt(double x){return std::cbrt(x);}
inline double se_math_abs(double x){return std::fabs(x);}
inline double se_math_floor(double x){return std::floor(x);}
inline double se_math_ceil(double x){return std::ceil(x);}
inline double se_math_round(double x){return std::round(x);}
inline double se_math_trunc(double x){return std::trunc(x);}
inline double se_math_sin(double x){return std::sin(x);}
inline double se_math_cos(double x){return std::cos(x);}
inline double se_math_tan(double x){return std::tan(x);}
inline double se_math_asin(double x){return std::asin(x);}
inline double se_math_acos(double x){return std::acos(x);}
inline double se_math_atan(double x){return std::atan(x);}
inline double se_math_sinh(double x){return std::sinh(x);}
inline double se_math_cosh(double x){return std::cosh(x);}
inline double se_math_tanh(double x){return std::tanh(x);}
inline double se_math_asinh(double x){return std::asinh(x);}
inline double se_math_acosh(double x){return std::acosh(x);}
inline double se_math_atanh(double x){return std::atanh(x);}
inline double se_math_exp(double x){return std::exp(x);}
inline double se_math_exp2(double x){return std::exp2(x);}
inline double se_math_expm1(double x){return std::expm1(x);}
inline double se_math_log(double x){return std::log(x);}
inline double se_math_log10(double x){return std::log10(x);}
inline double se_math_log2(double x){return std::log2(x);}
inline double se_math_log1p(double x){return std::log1p(x);}
inline double se_math_gamma(double x){return std::tgamma(x);}
inline double se_math_lgamma(double x){return std::lgamma(x);}
inline double se_math_erf(double x){return std::erf(x);}
inline double se_math_erfc(double x){return std::erfc(x);}
inline double se_math_pow(double x,double y){return std::pow(x,y);}
inline double se_math_atan2(double x,double y){return std::atan2(x,y);}
inline double se_math_fmod(double x,double y){return std::fmod(x,y);}
inline double se_math_remainder(double x,double y){return std::remainder(x,y);}
inline double se_math_copysign(double x,double y){return std::copysign(x,y);}
inline double se_math_nextafter(double x,double y){return std::nextafter(x,y);}
inline double se_math_sqrt(double x){if(x<0)throw std::runtime_error("math.sqrt needs a non-negative number.");return std::sqrt(x);}
inline double se_math_degrees(double x){return x*180.0/std::numbers::pi_v<double>;}
inline double se_math_radians(double x){return x*std::numbers::pi_v<double>/180.0;}
template<class... X> double se_math_hypot(X... x){return std::sqrt(((static_cast<double>(x)*static_cast<double>(x))+...));}
template<class... X> double se_math_min(X... x){return std::min({static_cast<double>(x)...});}
template<class... X> double se_math_max(X... x){return std::max({static_cast<double>(x)...});}
inline double se_math_clamp(double x,double lo,double hi){if(lo>hi)throw std::runtime_error("math.clamp needs min <= max.");return std::clamp(x,lo,hi);}
inline double se_math_lerp(double x,double y,double t){return x+(y-x)*t;}
inline double se_math_map_range(double x,double a,double b,double c,double d){if(a==b)throw std::runtime_error("math.map_range input range cannot have zero width.");return c+(x-a)*(d-c)/(b-a);}
inline std::int64_t se_math_sign(double x){return (x>0)-(x<0);}
inline bool se_math_isfinite(double x){return std::isfinite(x);}
inline bool se_math_isinf(double x){return std::isinf(x);}
inline bool se_math_isnan(double x){return std::isnan(x);}
template<class... X> std::int64_t se_math_gcd(std::int64_t x,X... rest){((x=std::gcd(x,static_cast<std::int64_t>(rest))),...);return x;}
template<class... X> std::int64_t se_math_lcm(std::int64_t x,X... rest){((x=std::lcm(x,static_cast<std::int64_t>(rest))),...);return x;}
inline std::int64_t se_math_factorial(std::int64_t n){if(n<0||n>20)throw std::runtime_error("math.factorial needs 0 <= n <= 20.");std::int64_t r=1;for(std::int64_t i=2;i<=n;++i)r*=i;return r;}
inline std::int64_t se_math_perm(std::int64_t n,std::int64_t k){if(k<0||n<0||k>n||n>20)throw std::runtime_error("math.perm needs 0 <= k <= n <= 20.");std::int64_t r=1;for(std::int64_t i=0;i<k;++i)r*=n-i;return r;}
inline std::int64_t se_math_comb(std::int64_t n,std::int64_t k){if(k<0||n<0||k>n)throw std::runtime_error("math.comb needs 0 <= k <= n.");k=std::min(k,n-k);std::int64_t r=1;for(std::int64_t i=1;i<=k;++i){auto a=n-k+i,b=i;auto g=std::gcd(a,b);a/=g;b/=g;g=std::gcd(r,b);r/=g;b/=g;if(b!=1||r>std::numeric_limits<std::int64_t>::max()/a)throw std::runtime_error("math.comb Int overflow");r*=a;}return r;}
template<class T> double se_math_sum(const SEList<T>& x){if(x.empty())throw std::runtime_error("math statistics need a non-empty List.");double r=0;for(auto v:x)r+=static_cast<double>(v);return r;}
template<class T> double se_math_mean(const SEList<T>& x){return se_math_sum(x)/x.size();}
template<class T> double se_math_median(SEList<T> x){if(x.empty())throw std::runtime_error("math statistics need a non-empty List.");x=x.clone();std::sort(x.begin(),x.end());auto n=x.size();return n%2?static_cast<double>(x[n/2]):(static_cast<double>(x[n/2-1])+static_cast<double>(x[n/2]))/2;}
template<class T> double se_math_variance(const SEList<T>& x){auto mean=se_math_mean(x);double r=0;for(auto v:x){double d=static_cast<double>(v)-mean;r+=d*d;}return r/x.size();}
template<class T> double se_math_stddev(const SEList<T>& x){return std::sqrt(se_math_variance(x));}

template<class T> double se_statistics_mean(const SEList<T>& x){return se_math_mean(x);}
template<class T> double se_statistics_median(const SEList<T>& x){return se_math_median(x);}
template<class T> double se_statistics_pvariance(const SEList<T>& x){return se_math_variance(x);}
template<class T> double se_statistics_pstdev(const SEList<T>& x){return se_math_stddev(x);}
template<class T> double se_statistics_variance(const SEList<T>& x){if(x.size()<2)throw std::runtime_error("statistics.variance needs at least 2 values.");return se_math_variance(x)*x.size()/(x.size()-1);}
template<class T> double se_statistics_stdev(const SEList<T>& x){return std::sqrt(se_statistics_variance(x));}
template<class T> inline double se_series_sum(const SEList<T>& values){double out=0;for(const auto& value:values)out+=static_cast<double>(value);return out;}
template<class T> inline double se_series_mean(const SEList<T>& values){if(values.empty())throw std::runtime_error("Mean needs values.");return se_series_sum(values)/values.size();}
template<class T> inline double se_series_min(const SEList<T>& values){if(values.empty())throw std::runtime_error("series.min needs values.");return static_cast<double>(*std::min_element(values.begin(),values.end()));}
template<class T> inline double se_series_max(const SEList<T>& values){if(values.empty())throw std::runtime_error("series.max needs values.");return static_cast<double>(*std::max_element(values.begin(),values.end()));}
template<class T> inline SEList<double> se_series_diff(const SEList<T>& values){SEList<double> out;for(std::size_t i=1;i<values.size();++i)out.push_back(static_cast<double>(values[i])-static_cast<double>(values[i-1]));return out;}
template<class T> inline SEList<T> se_series_lag(const SEList<T>& values,std::int64_t amount){if(amount<0)throw std::runtime_error("series.lag needs a non-negative lag.");SEList<T> out;for(std::size_t i=static_cast<std::size_t>(amount);i<values.size();++i)out.push_back(values[i-static_cast<std::size_t>(amount)]);return out;}
template<class T> inline SEList<double> se_series_moving_average(const SEList<T>& values,std::int64_t width){if(width<=0||static_cast<std::size_t>(width)>values.size())throw std::runtime_error("Window must be between 1 and series length.");SEList<double> out;double sum=0;for(std::int64_t i=0;i<width;++i)sum+=static_cast<double>(values[static_cast<std::size_t>(i)]);out.push_back(sum/width);for(std::size_t i=static_cast<std::size_t>(width);i<values.size();++i){sum+=static_cast<double>(values[i])-static_cast<double>(values[i-static_cast<std::size_t>(width)]);out.push_back(sum/width);}return out;}
template<class T> inline SEList<double> se_series_cumulative_sum(const SEList<T>& values){SEList<double> out;double sum=0;for(const auto& value:values){sum+=static_cast<double>(value);out.push_back(sum);}return out;}
template<class T> inline SEList<double> se_series_returns(const SEList<T>& values){SEList<double> out;for(std::size_t i=1;i<values.size();++i){double previous=static_cast<double>(values[i-1]);if(previous==0)throw std::runtime_error("Cannot calculate return from a zero value.");out.push_back(static_cast<double>(values[i])/previous-1.0);}return out;}
template<class T> inline SEList<double> se_series_normalize(const SEList<T>& values){if(values.empty())return {};double low=static_cast<double>(*std::min_element(values.begin(),values.end())),high=static_cast<double>(*std::max_element(values.begin(),values.end()));if(low==high)throw std::runtime_error("Cannot normalize a constant series.");SEList<double> out;for(const auto& value:values)out.push_back((static_cast<double>(value)-low)/(high-low));return out;}
template<class T> inline std::vector<double> se_numeric_vector(const SEList<T>& values){std::vector<double> out;out.reserve(values.size());for(const auto& value:values)out.push_back(static_cast<double>(value));return out;}
template<class T> inline std::vector<std::vector<double>> se_numeric_matrix(const SEList<SEList<T>>& values){if(values.empty())throw std::runtime_error("Matrix cannot be empty.");auto width=values.front().size();if(width==0)throw std::runtime_error("Matrix rows cannot be empty.");std::vector<std::vector<double>> out;out.reserve(values.size());for(const auto& row:values){if(row.size()!=width)throw std::runtime_error("Matrix rows must have equal width.");out.push_back(se_numeric_vector(row));}return out;}
inline SEList<SEList<double>> se_matrix_result(const std::vector<std::vector<double>>& values){SEList<SEList<double>> out;for(const auto& row:values)out.push_back(SEList<double>(row.begin(),row.end()));return out;}
template<class T> inline SEList<SEList<double>> se_linear_transpose(const SEList<SEList<T>>& values){auto a=se_numeric_matrix(values);std::vector<std::vector<double>> out(a[0].size(),std::vector<double>(a.size()));for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<a[0].size();++c)out[c][r]=a[r][c];return se_matrix_result(out);}
template<class A,class B> inline SEList<SEList<double>> se_linear_multiply(const SEList<SEList<A>>& left,const SEList<SEList<B>>& right){auto a=se_numeric_matrix(left),b=se_numeric_matrix(right);if(a[0].size()!=b.size())throw std::runtime_error("Matrix dimensions do not match.");std::vector<std::vector<double>> out(a.size(),std::vector<double>(b[0].size()));for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<b[0].size();++c)for(std::size_t k=0;k<b.size();++k)out[r][c]+=a[r][k]*b[k][c];return se_matrix_result(out);}
template<class A,class B> inline double se_linear_dot(const SEList<A>& left,const SEList<B>& right){if(left.size()!=right.size())throw std::runtime_error("Vector dimensions do not match.");double out=0;for(std::size_t i=0;i<left.size();++i)out+=static_cast<double>(left[i])*static_cast<double>(right[i]);return out;}
template<class A,class B> inline SEList<SEList<double>> se_linear_add(const SEList<SEList<A>>& left,const SEList<SEList<B>>& right){auto a=se_numeric_matrix(left),b=se_numeric_matrix(right);if(a.size()!=b.size()||a[0].size()!=b[0].size())throw std::runtime_error("Matrix dimensions do not match.");for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<a[0].size();++c)a[r][c]+=b[r][c];return se_matrix_result(a);}
template<class A,class B> inline SEList<SEList<double>> se_linear_subtract(const SEList<SEList<A>>& left,const SEList<SEList<B>>& right){auto a=se_numeric_matrix(left),b=se_numeric_matrix(right);if(a.size()!=b.size()||a[0].size()!=b[0].size())throw std::runtime_error("Matrix dimensions do not match.");for(std::size_t r=0;r<a.size();++r)for(std::size_t c=0;c<a[0].size();++c)a[r][c]-=b[r][c];return se_matrix_result(a);}
template<class T> inline SEList<SEList<double>> se_linear_scale(const SEList<SEList<T>>& values,double factor){auto a=se_numeric_matrix(values);for(auto& row:a)for(auto& value:row)value*=factor;return se_matrix_result(a);}
inline SEList<SEList<double>> se_linear_identity(std::int64_t n){if(n<=0||n>128)throw std::runtime_error("linear.identity size must be 1..128.");std::vector<std::vector<double>> out(static_cast<std::size_t>(n),std::vector<double>(static_cast<std::size_t>(n)));for(std::size_t i=0;i<out.size();++i)out[i][i]=1;return se_matrix_result(out);}
template<class T> inline double se_linear_determinant(const SEList<SEList<T>>& values){auto a=se_numeric_matrix(values);if(a.size()!=a[0].size())throw std::runtime_error("Determinant needs a square matrix.");double det=1;for(std::size_t c=0;c<a.size();++c){std::size_t pivot=c;for(std::size_t r=c+1;r<a.size();++r)if(std::fabs(a[r][c])>std::fabs(a[pivot][c]))pivot=r;if(std::fabs(a[pivot][c])<1e-12)return 0;if(pivot!=c){std::swap(a[pivot],a[c]);det=-det;}double d=a[c][c];det*=d;for(std::size_t r=c+1;r<a.size();++r){double f=a[r][c]/d;for(std::size_t k=c+1;k<a.size();++k)a[r][k]-=f*a[c][k];}}return det;}
template<class T> inline SEList<SEList<double>> se_linear_inverse(const SEList<SEList<T>>& values){auto a=se_numeric_matrix(values);if(a.size()!=a[0].size())throw std::runtime_error("Inverse needs a square matrix.");std::size_t n=a.size();std::vector<std::vector<double>> b(n,std::vector<double>(n));for(std::size_t i=0;i<n;++i)b[i][i]=1;for(std::size_t c=0;c<n;++c){std::size_t pivot=c;for(std::size_t r=c+1;r<n;++r)if(std::fabs(a[r][c])>std::fabs(a[pivot][c]))pivot=r;if(std::fabs(a[pivot][c])<1e-12)throw std::runtime_error("Matrix is singular.");std::swap(a[pivot],a[c]);std::swap(b[pivot],b[c]);double d=a[c][c];for(std::size_t k=0;k<n;++k){a[c][k]/=d;b[c][k]/=d;}for(std::size_t r=0;r<n;++r)if(r!=c){double f=a[r][c];for(std::size_t k=0;k<n;++k){a[r][k]-=f*a[c][k];b[r][k]-=f*b[c][k];}}}return se_matrix_result(b);}
template<class T> inline double se_linear_norm(const SEList<T>& values){double sum=0;for(const auto& x:values){double n=static_cast<double>(x);sum+=n*n;}return std::sqrt(sum);}
template<class T> inline SEList<double> se_linear_normalize(const SEList<T>& values){double norm=se_linear_norm(values);if(norm==0)throw std::runtime_error("Cannot normalize the zero vector.");SEList<double> out;for(const auto& x:values)out.push_back(static_cast<double>(x)/norm);return out;}
inline std::int64_t se_probability_factorial(std::int64_t n){return se_math_factorial(n);}
inline std::int64_t se_probability_choose(std::int64_t n,std::int64_t k){if(n>66)throw std::runtime_error("Choose requires 0 <= k <= n <= 66.");return se_math_comb(n,k);}
inline SEList<std::int64_t> se_fraction_make(std::int64_t a,std::int64_t b){if(b==0)throw std::runtime_error("Fraction denominator cannot be zero.");if(a==std::numeric_limits<std::int64_t>::min()||b==std::numeric_limits<std::int64_t>::min())throw std::runtime_error("Fraction exceeds supported range.");if(b<0){a=-a;b=-b;}auto d=std::gcd(a,b);return {a/d,b/d};}
inline double se_fraction_decimal(const SEList<std::int64_t>& f){if(f.size()!=2||f[1]==0)throw std::runtime_error("Expected [numerator, nonzero denominator].");return static_cast<double>(f[0])/static_cast<double>(f[1]);}
inline SEList<double> se_complex_make(double r,double i){return {r,i};}
inline std::pair<double,double> se_complex_parts(const SEList<double>& z){if(z.size()!=2)throw std::runtime_error("Complex values need [real, imaginary].");return {z[0],z[1]};}
inline SEList<double> se_complex_add(const SEList<double>& a,const SEList<double>& b){auto x=se_complex_parts(a),y=se_complex_parts(b);return {x.first+y.first,x.second+y.second};}
inline SEList<double> se_complex_multiply(const SEList<double>& a,const SEList<double>& b){auto x=se_complex_parts(a),y=se_complex_parts(b);return {x.first*y.first-x.second*y.second,x.first*y.second+x.second*y.first};}
inline double se_complex_magnitude(const SEList<double>& a){auto x=se_complex_parts(a);return std::hypot(x.first,x.second);}
template<class T> inline double se_calculus_polynomial(const SEList<T>& c,double x){double out=0;for(auto it=c.rbegin();it!=c.rend();++it)out=out*x+static_cast<double>(*it);return out;}
template<class T> inline double se_calculus_derivative(const SEList<T>& c,double x){double out=0;for(std::size_t k=c.size();k>1;--k)out=out*x+static_cast<double>(k-1)*static_cast<double>(c[k-1]);return out;}
template<class T> inline double se_calculus_integral(const SEList<T>& c,double a,double b){double out=0;for(std::size_t k=0;k<c.size();++k)out+=static_cast<double>(c[k])*(std::pow(b,static_cast<double>(k+1))-std::pow(a,static_cast<double>(k+1)))/static_cast<double>(k+1);return out;}
inline double se_units_convert(double value,const std::string& from,const std::string& to){auto family=[](const std::string& unit)->std::pair<std::string,double>{if(unit=="m")return {"length",1};if(unit=="km")return {"length",1000};if(unit=="cm")return {"length",.01};if(unit=="mm")return {"length",.001};if(unit=="s")return {"time",1};if(unit=="min")return {"time",60};if(unit=="h")return {"time",3600};if(unit=="kg")return {"mass",1};if(unit=="g")return {"mass",.001};if(unit=="lb")return {"mass",.45359237};return {"",0};};auto a=family(from),b=family(to);if(a.first.empty()||b.first.empty()||a.first!=b.first)throw std::runtime_error("Unknown or incompatible units.");return value*a.second/b.second;}
inline double se_units_celsius_to_fahrenheit(double x){return x*1.8+32;}
inline double se_units_fahrenheit_to_celsius(double x){return (x-32)/1.8;}
template<class T> inline SEList<T> se_table_column(const SEList<SEMap<T>>& rows,const std::string& key){SEList<T> out;for(const auto& row:rows)out.push_back(row.at(key));return out;}
template<class T> inline std::int64_t se_table_row_count(const SEList<SEMap<T>>& rows){return static_cast<std::int64_t>(rows.size());}
template<class T> inline std::int64_t se_dataset_row_count(const SEList<SEMap<T>>& rows){return static_cast<std::int64_t>(rows.size());}
template<class T> inline SEList<SEMap<T>> se_table_select(const SEList<SEMap<T>>& rows,const SEList<std::string>& keys){SEList<SEMap<T>> out;for(const auto& row:rows){SEMap<T> next;for(const auto& key:keys)next.put(key,row.at(key));out.push_back(next);}return out;}
template<class T> inline SEList<std::string> se_dataset_columns(const SEList<SEMap<T>>& rows){SEList<std::string> out;if(!rows.empty())for(const auto& pair:rows.front().items)out.push_back(pair.first);return out;}
template<class T> inline SEList<SEList<SEMap<T>>> se_dataset_split(const SEList<SEMap<T>>& rows,double ratio){if(ratio<=0||ratio>=1)throw std::runtime_error("Split ratio must be between 0 and 1.");std::size_t cut=static_cast<std::size_t>(rows.size()*ratio);SEList<SEMap<T>> a(rows.begin(),rows.begin()+static_cast<std::ptrdiff_t>(cut)),b(rows.begin()+static_cast<std::ptrdiff_t>(cut),rows.end());return {a,b};}
inline bool se_regex_match(const std::string& pattern,const std::string& value){return std::regex_match(value,std::regex(pattern));}
inline bool se_regex_search(const std::string& pattern,const std::string& value){return std::regex_search(value,std::regex(pattern));}
inline std::string se_regex_replace(const std::string& pattern,const std::string& value,const std::string& replacement){return std::regex_replace(value,std::regex(pattern),replacement);}
inline SEList<std::string> se_regex_split(const std::string& pattern,const std::string& value){std::regex re(pattern);std::sregex_token_iterator it(value.begin(),value.end(),re,-1),end;return SEList<std::string>(it,end);}
inline SEList<std::string> se_regex_find_all(const std::string& pattern,const std::string& value){std::regex re(pattern);SEList<std::string> out;for(std::sregex_iterator i(value.begin(),value.end(),re),end;i!=end;++i)out.push_back(i->str());return out;}
inline std::int64_t se_regex_count(const std::string& pattern,const std::string& value){return static_cast<std::int64_t>(se_regex_find_all(pattern,value).size());}
inline std::string se_regex_escape(const std::string& value){static const std::string special="^$|()[]{}*+?.\\";std::string out;for(char c:value){if(special.find(c)!=std::string::npos)out.push_back('\\');out.push_back(c);}return out;}
inline SEList<std::string> se_regex_groups(const std::string& pattern,const std::string& value){std::smatch m;if(!std::regex_search(value,m,std::regex(pattern)))return {};SEList<std::string> out;for(std::size_t i=1;i<m.size();++i)out.push_back(m[i].str());return out;}
inline std::string se_uuid_v4(){std::uniform_int_distribution<unsigned int> byte(0,255);unsigned char x[16];for(auto& v:x)v=static_cast<unsigned char>(byte(se_random_engine()));x[6]=(x[6]&0x0f)|0x40;x[8]=(x[8]&0x3f)|0x80;std::ostringstream out;out<<std::hex<<std::setfill('0');for(int i=0;i<16;++i){out<<std::setw(2)<<static_cast<unsigned int>(x[i]);if(i==3||i==5||i==7||i==9)out<<'-';}return out.str();}
inline bool se_uuid_valid(const std::string& value){static const std::regex uuid("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}$");return std::regex_match(value,uuid);}

// Standalone managed tasks and first-class callable operations. shared_future
// keeps both results and exceptions available for repeated await/join calls.
template<class F, class... A> auto se_async_run(F function, A... args) {
  return std::async(std::launch::async, std::move(function), std::move(args)...).share();
}
template<class T> decltype(auto) se_async_await(const std::shared_future<T>& task) {
  return task.get();
}
template<class T> bool se_async_ready(const std::shared_future<T>& task) {
  return task.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
template<class F, class... A> auto se_function_partial(F function, A... args) {
  return [function=std::move(function), bound=std::make_tuple(std::move(args)...)](auto&&... rest) mutable -> decltype(auto) {
    return std::apply([&](auto&... prefix) -> decltype(auto) {
      return std::invoke(function, prefix..., std::forward<decltype(rest)>(rest)...);
    }, bound);
  };
}
template<class F, class... A> auto se_function_bind(F function, A... args) {
  return se_function_partial(std::move(function), std::move(args)...);
}
template<class F, class... A> decltype(auto) se_function_call(F&& function, A&&... args) {
  return std::invoke(std::forward<F>(function), std::forward<A>(args)...);
}
template<class F, class T> auto se_function_map(F function, const SEList<T>& values) {
  return se_transform(values, std::move(function));
}
template<class F, class T> auto se_function_filter(F function, const SEList<T>& values) {
  return se_filter(values, std::move(function));
}
template<class F, class T, class A> auto se_function_reduce(F function, const SEList<T>& values, A initial) {
  return se_reduce(values, std::move(initial), std::move(function));
}
template<class F, class T> auto se_function_reduce(F function, const SEList<T>& values) {
  if(values.empty()) throw std::runtime_error("function.reduce needs a non-empty List or an initial value.");
  T state=values.front();
  for(std::size_t i=1;i<values.size();++i) state=function(state,values[i]);
  return state;
}

#include "se_extension_codec.hpp"
#include "se_extension_runtime.hpp"
#include "se_extension_sequences.hpp"
#include "se_extension_hash.hpp"
#include "se_extension_web.hpp"
#include "se_extension_tilemap.hpp"
#include "se_native_runtime.hpp"

template<class T> auto se_function_pipe(T value){return value;}
template<class T,class F,class... Rest> auto se_function_pipe(T value,F first,Rest... rest){return se_function_pipe(first(std::move(value)),std::move(rest)...);}
