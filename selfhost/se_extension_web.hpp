#pragma once
// Standalone implementations for selected SE cookie/cors/template helpers.
// Input validation must prevent response-header injection and unsafe HTML.
#include <stdexcept>
#include <string>
#include <sstream>

inline std::string se_web_trim(const std::string& value) {
  auto first=value.find_first_not_of(" \t\r\n");
  if(first==std::string::npos)return "";
  auto last=value.find_last_not_of(" \t\r\n");
  return value.substr(first,last-first+1);
}
inline SEMap<std::string> se_cookie_parse(const std::string& input){
  SEMap<std::string> output;
  std::istringstream stream(input);
  std::string part;
  while(std::getline(stream,part,';')){
    const auto eq=part.find('=');
    if(eq==std::string::npos)continue;
    output.put(se_web_trim(part.substr(0,eq)),se_web_trim(part.substr(eq+1)));
  }
  return output;
}
inline std::string se_cookie_set(const std::string& name,const std::string& value){
  if(name.empty()||name.find_first_of(";=\r\n \t")!=std::string::npos||
     value.find_first_of(";\r\n")!=std::string::npos)
    throw std::runtime_error("Invalid cookie name or value.");
  return name+"="+value+"; Path=/; HttpOnly; SameSite=Lax";
}
inline SEMap<std::string> se_cors_allow_origin(const std::string& origin){
  if(origin.find_first_of("\r\n")!=std::string::npos)
    throw std::runtime_error("Invalid origin.");
  SEMap<std::string> result;
  result.put("Access-Control-Allow-Origin",origin);
  result.put("Vary","Origin");
  return result;
}
inline SEMap<std::string> se_cors_preflight(const std::string& origin,const std::string& methods){
  if(origin.find_first_of("\r\n")!=std::string::npos||
     methods.find_first_of("\r\n")!=std::string::npos)
    throw std::runtime_error("Invalid CORS header value.");
  SEMap<std::string> result;
  result.put("Access-Control-Allow-Origin",origin);
  result.put("Access-Control-Allow-Methods",methods);
  result.put("Vary","Origin");
  return result;
}
inline std::string se_template_escape(const std::string& input){
  std::string output;
  for(char c:input){
    switch(c){
      case '&':output+="&amp;";break;
      case '<':output+="&lt;";break;
      case '>':output+="&gt;";break;
      case '"':output+="&quot;";break;
      case '\'':output+="&#39;";break;
      default:output+=c;
    }
  }
  return output;
}
inline std::string se_template_render(const std::string& source,
                                      const SEMap<std::string>& values){
  std::string output;
  for(std::size_t pos=0;pos<source.size();){
    const auto open=source.find("{{",pos);
    if(open==std::string::npos){output+=source.substr(pos);break;}
    output+=source.substr(pos,open-pos);
    const auto close=source.find("}}",open+2);
    if(close==std::string::npos)
      throw std::runtime_error("Template placeholder is not closed.");
    const auto key=se_web_trim(source.substr(open+2,close-open-2));
    bool found=false;
    for(const auto& item:values.items){
      if(item.first==key){output+=se_template_escape(item.second);found=true;break;}
    }
    if(!found)throw std::runtime_error("Missing template value: "+key);
    pos=close+2;
  }
  return output;
}
