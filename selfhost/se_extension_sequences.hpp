#pragma once
// Additional portable list, glob and hashing helpers for the SE bootstrap.
// This header is included after the basic SEList/SEMap and se_dynamic_type APIs.
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>

template<class T> SEList<T> se_itertools_chain(const SEList<T>& first) {return first.clone();}
template<class T,class... R> SEList<T> se_itertools_chain(const SEList<T>& first,const SEList<T>& second,const R&... rest){
  SEList<T> result=first.clone();
  for(const auto& x:second)result.push_back(x);
  if constexpr(sizeof...(rest)>0)return se_itertools_chain(result,rest...);
  return result;
}
template<class T> SEList<T> se_itertools_flatten(const SEList<T>& input){return input.clone();}
template<class T> SEList<T> se_itertools_flatten(const SEList<SEList<T>>& input){
  SEList<T> result;
  for(const auto& row:input)for(const auto& item:row)result.push_back(item);
  return result;
}
template<class T> SEList<SEList<T>> se_itertools_chunked(const SEList<T>& input,std::int64_t width){
  if(width<=0)throw std::runtime_error("Chunk size must be positive.");
  SEList<SEList<T>> result;
  for(std::size_t i=0;i<input.size();){
    SEList<T> chunk;
    auto count=std::min(static_cast<std::uint64_t>(width),
                        static_cast<std::uint64_t>(input.size()-i));
    for(std::size_t j=0;j<count;++j)chunk.push_back(input[i++]);
    result.push_back(std::move(chunk));
  }
  return result;
}
template<class T> SEList<T> se_itertools_take(const SEList<T>& input,std::int64_t count){
  if(count<0)throw std::runtime_error("Count cannot be negative.");
  SEList<T> result;
  auto size=std::min(static_cast<std::uint64_t>(count),
                     static_cast<std::uint64_t>(input.size()));
  for(std::size_t i=0;i<size;++i)result.push_back(input[i]);
  return result;
}
template<class T> SEList<T> se_itertools_drop(const SEList<T>& input,std::int64_t count){
  if(count<0)throw std::runtime_error("Count cannot be negative.");
  SEList<T> result;
  auto start=std::min(static_cast<std::uint64_t>(count),
                      static_cast<std::uint64_t>(input.size()));
  for(std::size_t i=start;i<input.size();++i)result.push_back(input[i]);
  return result;
}
template<class T> SEList<SEList<T>> se_itertools_windows(const SEList<T>& input,std::int64_t width){
  if(width<=0)throw std::runtime_error("Window size must be positive.");
  SEList<SEList<T>> result;
  if(static_cast<std::uint64_t>(width)>input.size())return result;
  for(std::size_t i=0;i<=input.size()-static_cast<std::size_t>(width);++i){
    SEList<T> window;
    for(std::size_t j=0;j<static_cast<std::size_t>(width);++j)window.push_back(input[i+j]);
    result.push_back(std::move(window));
  }
  return result;
}
template<class T> SEList<T> se_itertools_cycle(const SEList<T>& input,std::int64_t repeats){
  if(repeats<0)throw std::runtime_error("Repeat count cannot be negative.");
  if(!input.empty()&&static_cast<std::uint64_t>(repeats)>std::numeric_limits<std::size_t>::max()/input.size())
    throw std::runtime_error("itertools.cycle exceeds addressable size.");
  SEList<T> result;
  for(std::int64_t i=0;i<repeats;++i)for(const auto& x:input)result.push_back(x);
  return result;
}
template<class T> SEList<SEList<T>> se_itertools_pairs(const SEList<T>& input){
  SEList<SEList<T>> result;
  for(std::size_t i=1;i<input.size();++i)result.push_back(SEList<T>{input[i-1],input[i]});
  return result;
}
template<class T> SEList<T> se_itertools_unique(const SEList<T>& input){return se_unique(input);}

inline std::string se_glob_regex(const std::string& pattern){
  std::string expression="^";
  for(char c:pattern){
    if(c=='*')expression+=".*";
    else if(c=='?')expression+='.';
    else if(std::string(R"(\.^$|()[]{}+)").find(c)!=std::string::npos){
      expression+='\\';
      expression+=c;
    }else expression+=c;
  }
  return expression+'$';
}
inline bool se_glob_match(const std::string& pattern,const std::string& value){
  return std::regex_match(value,std::regex(se_glob_regex(pattern)));
}
inline SEList<std::string> se_glob_find(const std::string& pattern){
  namespace fs=std::filesystem;
  SEList<std::string> result;
  const auto wildcard=pattern.find_first_of("*?");
  const auto prefix=wildcard==std::string::npos ? pattern : pattern.substr(0,wildcard);
  const fs::path prefix_path(prefix);
  const fs::path root=prefix_path.has_parent_path()?prefix_path.parent_path():fs::path(".");
  std::error_code error;
  if(!fs::exists(root,error)||error)return result;
  const auto regex=std::regex(se_glob_regex(fs::path(pattern).generic_string()));
  for(fs::recursive_directory_iterator it(root,fs::directory_options::skip_permission_denied,error),end;
      it!=end&&!error;it.increment(error)){
    auto name=it->path().generic_string();
    if(std::regex_match(name,regex))result.push_back(name);
  }
  return result;
}
inline bool se_hashlib_compare(const std::string& left,const std::string& right){
  std::size_t difference=left.size()^right.size();
  for(std::size_t i=0;i<std::max(left.size(),right.size());++i){
    const unsigned char a=i<left.size()?static_cast<unsigned char>(left[i]):0;
    const unsigned char b=i<right.size()?static_cast<unsigned char>(right[i]):0;
    difference|=a^b;
  }
  return difference==0;
}
inline std::string se_hashlib_to_hex(const std::string& source){return se_encoding_hex(source);}
template<class T> bool se_typing_is(const T& value,std::string name){
  auto actual=se_dynamic_type(value);
  for(char& c:actual)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  for(char& c:name)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return actual==name;
}
template<class T> T se_typing_cast(const T& value,std::string name){
  auto actual=se_dynamic_type(value);
  for(char& c:actual)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  for(char& c:name)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if(actual!=name)throw std::runtime_error("typing.cast expected "+name+" but got "+se_dynamic_type(value)+".");
  return value;
}


inline std::string se_datetime_now(){return se_time_iso(se_time_now());}
inline std::int64_t se_datetime_timestamp(){return se_time_unix(se_time_now());}
inline std::string se_datetime_from_timestamp(std::int64_t timestamp){
  return se_time_iso(se_time_from_unix(timestamp));
}
inline std::string se_datetime_format(std::int64_t timestamp,const std::string& format){
  const auto value=static_cast<std::time_t>(timestamp);
  std::tm result{};
#ifdef _WIN32
  gmtime_s(&result,&value);
#else
  gmtime_r(&value,&result);
#endif
  std::ostringstream output;
  output<<std::put_time(&result,format.c_str());
  return output.str();
}
inline std::int64_t se_datetime_add_seconds(std::int64_t timestamp,std::int64_t seconds){
  constexpr auto max=std::numeric_limits<std::int64_t>::max();
  constexpr auto min=std::numeric_limits<std::int64_t>::min();
  if((seconds>0 && timestamp>max-seconds)||
     (seconds<0 && timestamp<min-seconds))
    throw std::runtime_error("datetime.add_seconds overflows Int.");
  return timestamp+seconds;
}
inline SEList<std::int64_t> se_iter_range(std::int64_t start,std::int64_t stop,std::int64_t step);
inline SEList<std::int64_t> se_iter_range(std::int64_t stop){
  return se_iter_range(std::int64_t{0},stop,std::int64_t{1});
}
inline SEList<std::int64_t> se_iter_range(std::int64_t start,std::int64_t stop){
  return se_iter_range(start,stop,std::int64_t{1});
}
inline SEList<std::int64_t> se_iter_range(std::int64_t start,std::int64_t stop,std::int64_t step){
  if(step==0)throw std::runtime_error("iter.range step cannot be zero.");
  SEList<std::int64_t> output;
  if(step>0){
    while(start<stop){
      output.push_back(start);
      if(start>std::numeric_limits<std::int64_t>::max()-step)break;
      start+=step;
    }
  }else{
    while(start>stop){
      output.push_back(start);
      if(start<std::numeric_limits<std::int64_t>::min()-step)break;
      start+=step;
    }
  }
  return output;
}


inline SEMap<std::int64_t> se_enum_make(const SEList<std::string>& names){
  SEMap<std::int64_t> result;
  if(names.size()>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    throw std::runtime_error("enum.make has too many values.");
  for(std::size_t i=0;i<names.size();++i){
    result.put(names[i],static_cast<std::int64_t>(i));
  }
  return result;
}
inline std::string se_enum_name(const SEMap<std::int64_t>& values,std::int64_t index){
  for(const auto& entry:values.items)if(entry.second==index)return entry.first;
  throw std::runtime_error("enum.name value was not found.");
}
inline std::int64_t se_enum_value(const SEMap<std::int64_t>& values,const std::string& name){
  for(const auto& entry:values.items)if(entry.first==name)return entry.second;
  throw std::runtime_error("enum.value name was not found.");
}
inline bool se_enum_has(const SEMap<std::int64_t>& values,const std::string& name){
  for(const auto& entry:values.items)if(entry.first==name)return true;
  return false;
}
