#pragma once
// Small C++20 runtime for the SE-written bootstrap compiler.
// This header is NOT the production SE runtime. See selfhost/README.md.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <vector>

inline std::string se_string(const std::string& value) { return value; }
inline std::string se_string(const char* value) { return std::string(value); }
inline std::string se_string(bool value) { return value ? "true" : "false"; }
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
