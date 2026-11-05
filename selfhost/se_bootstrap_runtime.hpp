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
  if constexpr(std::is_same_v<std::decay_t<A>,std::string> || std::is_same_v<std::decay_t<B>,std::string>)
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
