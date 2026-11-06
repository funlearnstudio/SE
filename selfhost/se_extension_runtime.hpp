#pragma once
// Standalone, production-compatible subset of web/configuration utility APIs.
// Included after SEMap and the core file helpers in se_bootstrap_runtime.hpp.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sstream>
#include <system_error>

inline std::string se_ext_trim(const std::string& source) {
  auto left=source.find_first_not_of(" \r\n\t");
  if(left==std::string::npos)return "";
  auto right=source.find_last_not_of(" \r\n\t");
  return source.substr(left,right-left+1);
}
inline std::string se_ext_lookup(const SEMap<std::string>& values,const std::string& key) {
  for(const auto& entry:values.items)if(entry.first==key)return entry.second;
  throw std::runtime_error("Missing configuration key: "+key);
}
inline SEMap<std::string> se_dotenv_parse(const std::string& source) {
  SEMap<std::string> result;
  std::istringstream input(source);
  std::string line;
  while(std::getline(input,line)) {
    line=se_ext_trim(line);
    if(line.empty()||line[0]=='#')continue;
    if(line.rfind("export ",0)==0)line=se_ext_trim(line.substr(7));
    auto sep=line.find('=');
    if(sep==std::string::npos)throw std::runtime_error("Expected KEY=VALUE.");
    auto key=se_ext_trim(line.substr(0,sep)),value=se_ext_trim(line.substr(sep+1));
    if(key.empty()||!(std::isalpha(static_cast<unsigned char>(key[0]))||key[0]=='_'))
      throw std::runtime_error("Invalid configuration key.");
    for(unsigned char c:key)if(!(std::isalnum(c)||c=='_'))
      throw std::runtime_error("Invalid configuration key.");
    if(value.size()>=2 && ((value.front()=='"'&&value.back()=='"')||
                            (value.front()=='\''&&value.back()=='\'')))
      value=value.substr(1,value.size()-2);
    result.put(key,value);
  }
  return result;
}
inline std::string se_dotenv_get(const SEMap<std::string>& values,const std::string& key){
  return se_ext_lookup(values,key);
}
inline SEMap<std::string> se_config_parse(const std::string& source) {
  SEMap<std::string> result;
  std::istringstream input(source);
  std::string line,section;
  while(std::getline(input,line)) {
    line=se_ext_trim(line);
    if(line.empty()||line[0]=='#'||line[0]==';')continue;
    if(line.front()=='['&&line.back()==']') {
      section=se_ext_trim(line.substr(1,line.size()-2));
      if(section.empty())throw std::runtime_error("Config section name cannot be empty.");
      continue;
    }
    auto sep=line.find('=');
    if(sep==std::string::npos)throw std::runtime_error("Expected key = value in config.");
    auto key=se_ext_trim(line.substr(0,sep)),value=se_ext_trim(line.substr(sep+1));
    if(key.empty())throw std::runtime_error("Config key cannot be empty.");
    if(value.size()>=2 && ((value.front()=='"'&&value.back()=='"')||
                            (value.front()=='\''&&value.back()=='\'')))
      value=value.substr(1,value.size()-2);
    result.put(section.empty()?key:section+"."+key,value);
  }
  return result;
}
inline std::string se_config_get(const SEMap<std::string>& values,const std::string& key){
  return se_ext_lookup(values,key);
}
inline std::int64_t se_config_get_int(const SEMap<std::string>& values,const std::string& key){
  auto raw=se_ext_lookup(values,key);
  try {
    std::size_t n=0;
    auto value=std::stoll(raw,&n);
    if(n!=raw.size())throw std::invalid_argument("trailing");
    return value;
  } catch(...) {throw std::runtime_error("Config value is not an Int: "+key);}
}
inline bool se_config_get_bool(const SEMap<std::string>& values,const std::string& key){
  auto raw=se_ext_lookup(values,key);
  std::transform(raw.begin(),raw.end(),raw.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
  if(raw=="true"||raw=="yes"||raw=="1")return true;
  if(raw=="false"||raw=="no"||raw=="0")return false;
  throw std::runtime_error("Config value is not a Bool: "+key);
}
inline SEMap<std::string> se_config_section(const SEMap<std::string>& values,const std::string& section){
  SEMap<std::string> result;
  const auto prefix=section+".";
  for(const auto& item:values.items)
    if(item.first.rfind(prefix,0)==0)result.put(item.first.substr(prefix.size()),item.second);
  return result;
}
inline SEMap<std::string> se_config_merge(const SEMap<std::string>& left,const SEMap<std::string>& right){
  auto result=left.clone();
  for(const auto& item:right.items)result.put(item.first,item.second);
  return result;
}
inline SEMap<std::string> se_cookie_parse(const std::string& header){
  SEMap<std::string> cookies;
  std::istringstream input(header);
  std::string part;
  while(std::getline(input,part,';')){
    const auto sep=part.find('=');
    if(sep!=std::string::npos)
      cookies.put(se_ext_trim(part.substr(0,sep)),se_ext_trim(part.substr(sep+1)));
  }
  return cookies;
}
inline std::string se_cookie_set(const std::string& name,const std::string& value){
  if(name.empty()||name.find_first_of(";=\r\n \t")!=std::string::npos||
     value.find_first_of(";\r\n")!=std::string::npos)
    throw std::runtime_error("Invalid cookie name or value.");
  return name+"="+value+"; Path=/; HttpOnly; SameSite=Lax";
}
inline void se_ext_header_safe(const std::string& value){
  if(value.find_first_of("\r\n")!=std::string::npos)
    throw std::runtime_error("Invalid HTTP header value.");
}
inline SEMap<std::string> se_cors_allow_origin(const std::string& origin){
  se_ext_header_safe(origin);
  SEMap<std::string> result;
  result.put("Access-Control-Allow-Origin",origin);
  result.put("Vary","Origin");
  return result;
}
inline SEMap<std::string> se_cors_preflight(const std::string& origin,const std::string& methods){
  se_ext_header_safe(origin);se_ext_header_safe(methods);
  auto result=se_cors_allow_origin(origin);
  result.put("Access-Control-Allow-Methods",methods);
  return result;
}
inline std::string se_template_escape(const std::string& value){
  std::string out;
  for(char c:value){
    switch(c){
      case '&':out+="&amp;";break;
      case '<':out+="&lt;";break;
      case '>':out+="&gt;";break;
      case '"':out+="&quot;";break;
      case '\'':out+="&#39;";break;
      default:out+=c;
    }
  }
  return out;
}
inline std::string se_template_render(const std::string& source,const SEMap<std::string>& values){
  std::string out;
  std::size_t position=0;
  while(position<source.size()){
    auto left=source.find("{{",position);
    if(left==std::string::npos){out+=source.substr(position);break;}
    out+=source.substr(position,left-position);
    auto right=source.find("}}",left+2);
    if(right==std::string::npos)throw std::runtime_error("Template placeholder is not closed.");
    auto key=se_ext_trim(source.substr(left+2,right-left-2));
    try{out+=se_template_escape(se_ext_lookup(values,key));}
    catch(const std::runtime_error&){throw std::runtime_error("Missing template value: "+key);}
    position=right+2;
  }
  return out;
}
inline std::string se_static_mime(const std::string& name){
  const auto dot=name.find_last_of('.');
  const auto ext=dot==std::string::npos?std::string{}:name.substr(dot);
  if(ext==".html")return "text/html; charset=utf-8";
  if(ext==".css")return "text/css; charset=utf-8";
  if(ext==".js")return "text/javascript; charset=utf-8";
  if(ext==".json")return "application/json";
  if(ext==".svg")return "image/svg+xml";
  if(ext==".png")return "image/png";
  if(ext==".jpg")return "image/jpeg";
  if(ext==".txt")return "text/plain; charset=utf-8";
  return "application/octet-stream";
}
inline std::filesystem::path se_ext_safe_file(const std::string& root,const std::string& name){
  namespace fs=std::filesystem;
  const fs::path relative(name);
  if(name.empty()||relative.is_absolute())throw std::runtime_error("Invalid file path.");
  std::error_code ec;
  const auto base=fs::weakly_canonical(fs::path(root),ec);
  if(ec||!fs::is_directory(base))throw std::runtime_error("Root directory does not exist.");
  const auto target=fs::weakly_canonical(base/relative,ec);
  if(ec)throw std::runtime_error("Invalid file path.");
  auto cursor=target.begin();
  for(auto part:base){
    if(cursor==target.end()||*cursor!=part)throw std::runtime_error("File path escapes root.");
    ++cursor;
  }
  if(cursor==target.end())throw std::runtime_error("File path must name a child.");
  return target;
}
inline std::string se_static_read(const std::string& root,const std::string& name){
  auto path=se_ext_safe_file(root,name);
  std::ifstream input(path,std::ios::binary);
  if(!input)throw std::runtime_error("Could not read static file.");
  return std::string(std::istreambuf_iterator<char>(input),{});
}
inline std::string se_upload_save(const std::string& root,const std::string& name,const std::string& body){
  auto path=se_ext_safe_file(root,name);
  std::ofstream out(path,std::ios::binary|std::ios::trunc);
  if(!out)throw std::runtime_error("Could not write upload.");
  out.write(body.data(),static_cast<std::streamsize>(body.size()));
  if(!out)throw std::runtime_error("Could not finish upload.");
  return path.string();
}
