#include "s/modules.hpp"
#include "s/error.hpp"
#include "s/lexer.hpp"
#include "s/parser.hpp"
#include "s/platform.hpp"
#include "s/advanced.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace s {
namespace {
std::string read_text(const std::filesystem::path& p){
  std::ifstream f(p);
  if(!f) throw Error({1,1},"I could not open module '"+p.string()+"'.");
  return {std::istreambuf_iterator<char>(f),{}};
}
std::vector<std::string> words(const std::string& line){
  std::istringstream in(line);
  std::vector<std::string> r;
  for(std::string x;in>>x;) r.push_back(x);
  return r;
}
std::string key_for(const std::filesystem::path& p){
  std::error_code ec;
  auto q=std::filesystem::weakly_canonical(p,ec);
  return (ec?p:q).string();
}
}

ast::Program ModuleLoader::load(const std::filesystem::path& entry){
  program_={};
  loaded_.clear();
  stack_.clear();
  std::error_code ec;
  auto absolute=std::filesystem::absolute(entry,ec);
  if(ec) absolute=entry;
  root_=absolute.parent_path();
  auto module=parse_source_module(absolute.stem().string(),absolute);
  stack_.push_back(absolute);
  for(auto& name:module.imports) load_module(name,absolute.parent_path());
  stack_.pop_back();
  program_.entry=module.name;
  program_.statements=module.statements;
  program_.modules.push_back(std::move(module));
  return program_;
}

std::filesystem::path ModuleLoader::resolve(const std::string& name,const std::filesystem::path& from,bool& builtin,bool& native) const{
  builtin=false;
  native=false;
  if(name=="file"||name=="path"||name=="time"||name=="math"||name=="random"||name=="os"||combined_platform_builtin(name)){
    builtin=true;
    return {};
  }
  std::vector<std::filesystem::path> bases{from,root_,from/".se/packages",root_/".se/packages"};
  if(const char* home=std::getenv("SE_HOME")) bases.emplace_back(std::filesystem::path(home)/"packages");
  else if(const char* home=std::getenv("S_HOME")) bases.emplace_back(std::filesystem::path(home)/"packages");
#ifdef S_SOURCE_ROOT
  bases.emplace_back(std::filesystem::path(S_SOURCE_ROOT));
#endif
  for(auto& base:bases){
    std::vector<std::filesystem::path> source_candidates{
      base/(name+".se"),base/name/(name+".se"),
      base/(name+".s"),base/name/(name+".s")};
    for(auto& p:source_candidates) if(std::filesystem::is_regular_file(p)) return p;
    std::vector<std::filesystem::path> native_candidates{base/(name+".snative"),base/"native"/(name+".snative"),base/name/(name+".snative")};
    for(auto& p:native_candidates){
      if(std::filesystem::is_regular_file(p)){
        native=true;
        return p;
      }
    }
  }
  throw Error({1,1},"I could not find module '"+name+"'.","SE looked in the current project, standard library, and package directories.");
}

std::size_t ModuleLoader::load_module(const std::string& name,const std::filesystem::path& from){
  bool builtin=false,native=false;
  auto path=resolve(name,from,builtin,native);
  std::string key=builtin?"@std/"+name:key_for(path);
  if(auto i=loaded_.find(key);i!=loaded_.end()) return i->second;
  if(!builtin){
    auto cycle=std::find_if(stack_.begin(),stack_.end(),[&](const auto& p){return key_for(p)==key_for(path);});
    if(cycle!=stack_.end()){
      std::string chain;
      for(auto i=cycle;i!=stack_.end();++i){
        if(!chain.empty()) chain+=" -> ";
        chain+=i->stem().string();
      }
      chain+=" -> "+name;
      throw Error({1,1},"Circular module import: "+chain+".");
    }
  }
  ast::Module module;
  if(builtin){
    module.name=name;
    module.builtin=true;
    module.path="std/"+name;
  }else if(native){
    module=parse_native_module(name,path);
  }else{
    module=parse_source_module(name,path);
  }
  if(!builtin&&!native){
    stack_.push_back(path);
    for(auto& dep:module.imports) load_module(dep,path.parent_path());
    stack_.pop_back();
  }
  auto index=program_.modules.size();
  program_.modules.push_back(std::move(module));
  loaded_[key]=index;
  return index;
}

ast::Module ModuleLoader::parse_source_module(const std::string& name,const std::filesystem::path& path){
  Lexer lexer(read_text(path));
  Parser parser(lexer.scan());
  auto parsed=parser.parse();
  ast::Module m;
  m.name=name;
  m.path=path.string();
  m.statements=std::move(parsed.statements);
  for(auto& s:m.statements){
    if(auto u=std::dynamic_pointer_cast<ast::Use>(s)) m.imports.push_back(u->name);
  }
  return m;
}

ast::Module ModuleLoader::parse_native_module(const std::string& name,const std::filesystem::path& path){
  ast::Module m;
  m.name=name;
  m.path=path.string();
  m.native=true;
  std::ifstream in(path);
  if(!in) throw Error({1,1},"I could not open native module '"+name+"'.");
  std::string line;
  int line_no=0;
  while(std::getline(in,line)){
    ++line_no;
    auto hash=line.find('#');
    if(hash!=std::string::npos) line.resize(hash);
    auto w=words(line);
    if(w.empty()) continue;
    if(w[0]=="library"){
      if(w.size()!=2) throw Error({line_no,1},"Native library line must be: library name");
      m.native_library=w[1];
      continue;
    }
    if(w.size()<4) throw Error({line_no,1},"Native function line is incomplete.");
    ast::NativeFunction f;
    f.name=w[0];
    f.symbol=w[1];
    auto arrow=std::find(w.begin()+2,w.end(),"->");
    if(arrow==w.end()||arrow+1==w.end()) throw Error({line_no,1},"Native function needs '-> ReturnType'.");
    for(auto i=w.begin()+2;i!=arrow;++i) f.args.push_back(*i);
    f.result=*(arrow+1);
    for(auto i=arrow+2;i!=w.end();++i){
      if(*i=="fallible") f.fallible=true;
      else if(*i=="cleanup"&&i+1!=w.end()) f.cleanup=*++i;
    }
    m.native_functions.push_back(std::move(f));
  }
  if(m.native_library.empty()) throw Error({1,1},"Native module '"+name+"' has no library line.");
  return m;
}

} // namespace s
