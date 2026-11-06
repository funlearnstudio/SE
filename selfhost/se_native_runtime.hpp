#pragma once
#include <memory>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <stdexcept>
#include <limits>
#include <type_traits>
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif
struct SENativeError : std::runtime_error { using std::runtime_error::runtime_error; };
// Bytes alias the shared byte list defined with the collection storage.
struct SENativeBytesView {const std::uint8_t* data;std::size_t size;};
struct SENativeHandle {std::shared_ptr<void> resource;std::string tag;};
inline SEBytes se_bytes(const std::string& text){return {text.begin(),text.end()};}
inline std::string se_error_kind(const std::exception& error){return dynamic_cast<const SENativeError*>(&error)?"NativeError":"Error";}
inline std::shared_ptr<void> se_native_library(const std::string& metadata,const std::string& name){
  static std::mutex mutex;
  static std::unordered_map<std::string,std::shared_ptr<void>> cache;
  std::lock_guard<std::mutex> lock(mutex);
  auto key=metadata+"\n"+name;
  if(auto found=cache.find(key);found!=cache.end())return found->second;
  std::vector<std::filesystem::path> dirs{std::filesystem::path(metadata).parent_path(),std::filesystem::current_path(),std::filesystem::current_path()/"build"};
  if(const char* value=std::getenv("S_NATIVE_PATH")){
    std::string paths=value;std::size_t start=0;
    while(start<=paths.size()){auto end=paths.find(
#ifdef _WIN32
    ';'
#else
    ':'
#endif
    ,start);dirs.emplace_back(paths.substr(start,end-start));if(end==std::string::npos)break;start=end+1;}
  }
  for(const auto& dir:dirs)for(const auto& file:std::vector<std::string>{name,"lib"+name+".so","lib"+name+".dylib",name+".dll"}){
#ifdef _WIN32
    if(auto handle=LoadLibraryW((dir/file).wstring().c_str()))return cache[key]=std::shared_ptr<void>(handle,[](void* h){FreeLibrary(static_cast<HMODULE>(h));});
#else
    if(auto handle=dlopen((dir/file).string().c_str(),RTLD_NOW|RTLD_LOCAL))return cache[key]=std::shared_ptr<void>(handle,[](void* h){dlclose(h);});
#endif
  }
  throw SENativeError("Could not load native library '"+name+"'.");
}
inline void* se_native_symbol(const std::shared_ptr<void>& lib,const std::string& name){
#ifdef _WIN32
  auto result=reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib.get()),name.c_str()));
#else
  auto result=dlsym(lib.get(),name.c_str());
#endif
  if(!result)throw SENativeError("Native symbol '"+name+"' was not found.");return result;
}
inline int se_native_argument(std::int64_t value){
  if(value<std::numeric_limits<int>::min()||value>std::numeric_limits<int>::max())
    throw SENativeError("Native Int argument is outside the C int range.");
  return static_cast<int>(value);
}
inline int se_native_argument(bool value){return value?1:0;}
inline double se_native_argument(double value){return value;}
inline const char* se_native_argument(const std::string& value){return value.c_str();}
inline SENativeBytesView se_native_argument(const SEBytes& value){return {value.data(),value.size()};}
inline void* se_native_handle(const SENativeHandle& value,const std::string& tag){if(value.tag!=tag||!value.resource)throw SENativeError("Native handle has the wrong type.");return value.resource.get();}
template<class R,class... A> auto se_native_invoke(const std::string& metadata,const std::string& library,const std::string& symbol,bool fallible,const std::string& cleanup,const std::string& tag,const std::string& export_name,A... args){
  auto lib=se_native_library(metadata,library);
  void* raw=nullptr;
  try{raw=se_native_symbol(lib,symbol);}catch(const SENativeError& e){throw SENativeError("Native function '"+export_name+"': "+e.what());}
  auto function=reinterpret_cast<R(*)(A...)>(raw);
  void(*destroy)(void*)=nullptr;
  if constexpr(std::is_same_v<R,void*>)if(!cleanup.empty()){
    try{destroy=reinterpret_cast<void(*)(void*)>(se_native_symbol(lib,cleanup));}catch(const SENativeError& e){throw SENativeError("Native function '"+export_name+"': "+e.what());}
  }
  if constexpr(std::is_void_v<R>){function(args...);return;}
  else {
    auto result=function(args...);
    if constexpr(std::is_same_v<R,int>){if(fallible&&result<0)throw SENativeError("Native function '"+export_name+"' reported an error.");return static_cast<std::int64_t>(result);}
    else if constexpr(std::is_same_v<R,const char*>){if(!result)throw SENativeError("Native function returned no Text.");return std::string(result);}
    else if constexpr(std::is_same_v<R,SENativeBytesView>){if(result.size&&!result.data)throw SENativeError("Native function returned invalid Bytes.");if(!result.size)return SEBytes{};return SEBytes(result.data,result.data+result.size);}
    else if constexpr(std::is_same_v<R,void*>){if(!result)throw SENativeError("Native resource creation failed.");return SENativeHandle{std::shared_ptr<void>(result,[lib,destroy](void* p){if(destroy)destroy(p);}),tag};}
    else return result;
  }
}
