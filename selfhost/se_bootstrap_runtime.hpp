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
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

// Production SE automatically evaluates zero-parameter function values.
// Keep non-callable values (including mutable collections) as references, and
// preserve function values that require one or more explicit arguments.
template<class T> decltype(auto) se_implicit_call(T&& value) {
  if constexpr (std::is_invocable_v<T&&>)
    return std::forward<T>(value)();
  else
    return std::forward<T>(value);
}

struct SEError {
  std::string message;
  std::string kind = "Error";
};
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
template<class T> inline void se_print_value(std::ostream& stream, const T& value) {
  stream << std::boolalpha << value;
}
template<class T> inline void se_print_value(std::ostream& stream, const std::vector<T>& values) {
  stream << '[';
  for (std::size_t i=0;i<values.size();++i) {
    if(i)stream<<", ";
    se_print_value(stream, values[i]);
  }
  stream << ']';
}
template<class T> inline void se_say(const T& value) {
  se_print_value(std::cout, value);
  std::cout << '\n';
}
template<class A,class B> auto se_add(const A& a,const B& b) {
  if constexpr(std::is_constructible_v<std::string, A> || std::is_constructible_v<std::string, B>)
    return se_string(a)+se_string(b);
  else return a+b;
}
inline std::int64_t se_int(const std::string& v) {
  std::size_t end=0;
  auto n=std::stoll(v,&end);
  if(end!=v.size())throw std::runtime_error("Invalid Int: "+v);
  return static_cast<std::int64_t>(n);
}
template<class T> inline std::int64_t se_int(const T& v) { return static_cast<std::int64_t>(v); }
inline double se_num(const std::string& v) {
  std::size_t end=0;
  double n=std::stod(v,&end);
  if(end!=v.size()||!std::isfinite(n))throw std::runtime_error("Invalid Num: "+v);
  return n;
}
template<class T> inline double se_num(const T& v) {return static_cast<double>(v);}
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
inline std::string se_json_stringify(const std::vector<std::string>& values){
  std::string result="[";
  for(const auto& value:values){
    if(result.size()!=1)result+=",";
    result+=se_json_stringify(value);
  }
  return result+"]";
}
inline std::vector<std::string> se_json_parse(const std::string& json){
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
  std::vector<std::string> out;
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

inline std::string se_getenv(const std::string& key) {
  const char* raw=std::getenv(key.c_str());
  return raw ? std::string(raw) : std::string();
}

inline std::string se_ask(const std::string& question) {
  std::cout<<question;
  std::cout.flush();
  std::string out;
  if(!std::getline(std::cin,out))throw std::runtime_error("Unable to read input");
  return out;
}
inline std::vector<std::int64_t> se_range(std::int64_t start,std::int64_t finish) {
  std::vector<std::int64_t> out;
  auto step=start<=finish?std::int64_t{1}:std::int64_t{-1};
  for(auto n=start;;n+=step) {
    out.push_back(n);
    if(n==finish)break;
  }
  return out;
}
template<class T> bool se_contains(const std::vector<T>& items,const T& target) {
  return std::find(items.begin(),items.end(),target)!=items.end();
}
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
inline std::vector<std::string> se_split(const std::string& s,const std::string& delim){
  std::vector<std::string> result;
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
inline std::string se_join(const std::vector<std::string>& parts,const std::string& delim){
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
template<class T> inline std::vector<T> se_take(const std::vector<T>& values,std::int64_t amount){
  if(amount<0)throw std::runtime_error("collections.take requires nonnegative count");
  const auto n=std::min<std::size_t>(values.size(),static_cast<std::size_t>(amount));
  return {values.begin(),values.begin()+static_cast<std::ptrdiff_t>(n)};
}
template<class T> inline std::vector<T> se_drop(const std::vector<T>& values,std::int64_t amount){
  if(amount<0)throw std::runtime_error("collections.drop requires nonnegative count");
  const auto n=std::min<std::size_t>(values.size(),static_cast<std::size_t>(amount));
  return {values.begin()+static_cast<std::ptrdiff_t>(n),values.end()};
}
template<class T> inline std::vector<T> se_slice(const std::vector<T>& items,std::int64_t first,std::int64_t last){
  if(first<0 || last<0)throw std::runtime_error("collections.slice expects nonnegative indices");
  const auto left=std::min<std::size_t>(items.size(),static_cast<std::size_t>(first));
  const auto right=std::min<std::size_t>(items.size(),static_cast<std::size_t>(last));
  if(right<left)return {};
  return {items.begin()+static_cast<std::ptrdiff_t>(left),
          items.begin()+static_cast<std::ptrdiff_t>(right)};
}
template<class T,class Pred> inline std::vector<T> se_filter(const std::vector<T>& items,Pred predicate){
  std::vector<T> result;
  for(const auto& item:items)if(predicate(item))result.push_back(item);
  return result;
}
template<class T,class F> inline auto se_transform(const std::vector<T>& items,F function){
  using Output=std::decay_t<decltype(function(std::declval<T>()))>;
  std::vector<Output> out;
  out.reserve(items.size());
  for(const auto& item:items)out.push_back(function(item));
  return out;
}
template<class T,class A,class F> inline A se_reduce(const std::vector<T>& items,A state,F combine){
  for(const auto& item:items)state=combine(state,item);
  return state;
}
template<class T,class F> inline std::vector<T> se_sort_with(std::vector<T> values,F before){
  std::stable_sort(values.begin(),values.end(),before);
  return values;
}
template<class T> inline std::vector<T> se_sorted(std::vector<T> items){std::sort(items.begin(),items.end());return items;}
template<class T> inline std::vector<T> se_reversed(std::vector<T> items){std::reverse(items.begin(),items.end());return items;}

template<class T, class... Ts> auto se_list(T first, Ts... rest){
  using Item=std::common_type_t<T,Ts...>;
  return std::vector<Item>{static_cast<Item>(first),static_cast<Item>(rest)...};
}
inline std::vector<std::string> se_list(){ return {}; }
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
  std::vector<std::pair<std::string,T>> items;
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
};
template<class T> struct SESet {
  std::vector<T> items;
  void add(const T& value) {
    if(std::find(items.begin(),items.end(),value)==items.end())items.push_back(value);
  }
  std::size_t size() const{return items.size();}
};
inline std::string se_module_help(const std::string& module){
  if(module=="math")return "members: help, pi, e, sqrt, pow, abs, floor, ceil, round, sin, cos, tan, log, log10, exp, hypot, min, max";
  if(module=="time")return "members: help, now, unix, from_unix, iso";
  if(module=="text")return "members: help, split, trim, contains, repeat, join";
  if(module=="collections")return "members: help, sort, reverse, take, drop, slice, filter, map, reduce, sort_with";
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
template<class T> inline std::string se_help_text(const std::vector<T>&){return "members: help, len, add value, remove value";}
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
template<class T,class... Values> auto se_set(T first,Values... rest){
  using Element=std::common_type_t<T,Values...>;
  SESet<Element> out;
  out.add(static_cast<Element>(first));
  (out.add(static_cast<Element>(rest)),...);
  return out;
}
inline SESet<std::string> se_set(){return {};}
template<class T> inline bool se_contains(const SESet<T>& values,const T& item){
  return std::find(values.items.begin(),values.items.end(),item)!=values.items.end();
}
template<class T> inline bool se_contains(const SEMap<T>& values,const std::string& key){
  for(const auto& item:values.items)if(item.first==key)return true;
  return false;
}
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
template<class T> decltype(auto) se_index(std::vector<T>& items,std::int64_t i){
  if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("List index out of bounds");
  return items.at(static_cast<std::size_t>(i));
}
template<class T> decltype(auto) se_index(const std::vector<T>& items,std::int64_t i){
  if(i<0||static_cast<std::size_t>(i)>=items.size())throw std::runtime_error("List index out of bounds");
  return items.at(static_cast<std::size_t>(i));
}
template<class T> decltype(auto) se_index(SEMap<T>& items,const std::string& key){return items.at(key);}
template<class T> decltype(auto) se_index(const SEMap<T>& items,const std::string& key){return items.at(key);}
template<class T> inline void se_add_item(std::vector<T>& values,const T& value){values.push_back(value);}
template<class T> inline void se_add_item(SESet<T>& values,const T& value){values.add(value);}
template<class T> inline bool se_remove_item(std::vector<T>& values,const T& value){
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
template<class T> double se_math_sum(const std::vector<T>& x){if(x.empty())throw std::runtime_error("math statistics need a non-empty List.");double r=0;for(auto v:x)r+=static_cast<double>(v);return r;}
template<class T> double se_math_mean(const std::vector<T>& x){return se_math_sum(x)/x.size();}
template<class T> double se_math_median(std::vector<T> x){if(x.empty())throw std::runtime_error("math statistics need a non-empty List.");std::sort(x.begin(),x.end());auto n=x.size();return n%2?static_cast<double>(x[n/2]):(static_cast<double>(x[n/2-1])+static_cast<double>(x[n/2]))/2;}
template<class T> double se_math_variance(const std::vector<T>& x){auto mean=se_math_mean(x);double r=0;for(auto v:x){double d=static_cast<double>(v)-mean;r+=d*d;}return r/x.size();}
template<class T> double se_math_stddev(const std::vector<T>& x){return std::sqrt(se_math_variance(x));}

template<class T> double se_statistics_mean(const std::vector<T>& x){return se_math_mean(x);}
template<class T> double se_statistics_median(const std::vector<T>& x){return se_math_median(x);}
template<class T> double se_statistics_pvariance(const std::vector<T>& x){return se_math_variance(x);}
template<class T> double se_statistics_pstdev(const std::vector<T>& x){return se_math_stddev(x);}
template<class T> double se_statistics_variance(const std::vector<T>& x){if(x.size()<2)throw std::runtime_error("statistics.variance needs at least 2 values.");return se_math_variance(x)*x.size()/(x.size()-1);}
template<class T> double se_statistics_stdev(const std::vector<T>& x){return std::sqrt(se_statistics_variance(x));}
inline bool se_regex_match(const std::string& pattern,const std::string& value){return std::regex_match(value,std::regex(pattern));}
inline bool se_regex_search(const std::string& pattern,const std::string& value){return std::regex_search(value,std::regex(pattern));}
inline std::string se_regex_replace(const std::string& pattern,const std::string& value,const std::string& replacement){return std::regex_replace(value,std::regex(pattern),replacement);}
inline std::vector<std::string> se_regex_split(const std::string& pattern,const std::string& value){std::regex re(pattern);std::sregex_token_iterator it(value.begin(),value.end(),re,-1),end;return std::vector<std::string>(it,end);}
