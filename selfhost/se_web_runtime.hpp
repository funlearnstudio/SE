#pragma once

inline std::string se_web_trim(std::string value) {
  auto first=value.find_first_not_of(" \t\r\n"), last=value.find_last_not_of(" \t\r\n");
  return first==std::string::npos ? std::string{} : value.substr(first,last-first+1);
}
inline SEMap<std::string> se_cookie_parse(const std::string& header) {
  SEMap<std::string> result; std::size_t start=0;
  while(start<header.size()) { auto end=header.find(';',start); if(end==std::string::npos)end=header.size(); auto item=header.substr(start,end-start); auto eq=item.find('='); if(eq!=std::string::npos){auto key=se_web_trim(item.substr(0,eq));if(!key.empty())result.put(key,se_web_trim(item.substr(eq+1)));} start=end+1; }
  return result;
}
inline std::string se_cookie_set(const std::string& name,const std::string& value) {
  if(name.empty()||name.find_first_of(";=\r\n")!=std::string::npos||value.find_first_of(";\r\n")!=std::string::npos)throw std::runtime_error("Invalid cookie name or value.");
  return name+"="+value+"; Path=/; SameSite=Lax";
}
inline SEMap<std::string> se_cors_allow_origin(const std::string& origin) { SEMap<std::string> h;h.put("Access-Control-Allow-Origin",origin);h.put("Vary","Origin");return h; }
inline SEMap<std::string> se_cors_preflight(const std::string& origin,const std::string& methods) { auto h=se_cors_allow_origin(origin);h.put("Access-Control-Allow-Methods",methods);h.put("Access-Control-Allow-Headers","Content-Type, Authorization");h.put("Access-Control-Max-Age","600");return h; }
inline std::string se_template_escape(const std::string& s) { std::string o;for(char c:s)switch(c){case '&':o+="&amp;";break;case '<':o+="&lt;";break;case '>':o+="&gt;";break;case '"':o+="&quot;";break;case '\'':o+="&#39;";break;default:o+=c;}return o; }
inline std::string se_template_render(std::string source,const SEMap<std::string>& values) { std::size_t p=0;while((p=source.find("{{",p))!=std::string::npos){auto e=source.find("}}",p+2);if(e==std::string::npos)throw std::runtime_error("Unclosed template placeholder.");auto key=se_web_trim(source.substr(p+2,e-p-2));const std::string* value=nullptr;for(const auto& item:values.items)if(item.first==key){value=&item.second;break;}if(!value)throw std::runtime_error("Missing template value: "+key);auto replacement=se_template_escape(*value);source.replace(p,e+2-p,replacement);p+=replacement.size();}return source; }
inline std::string se_static_mime(const std::string& filename) { auto ext=std::filesystem::path(filename).extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});if(ext==".html"||ext==".htm")return "text/html; charset=utf-8";if(ext==".css")return "text/css; charset=utf-8";if(ext==".js"||ext==".mjs")return "text/javascript; charset=utf-8";if(ext==".json")return "application/json";if(ext==".txt")return "text/plain; charset=utf-8";if(ext==".svg")return "image/svg+xml";if(ext==".png")return "image/png";if(ext==".jpg"||ext==".jpeg")return "image/jpeg";if(ext==".gif")return "image/gif";if(ext==".wasm")return "application/wasm";return "application/octet-stream"; }
inline std::filesystem::path se_web_safe_path(const std::string& root,const std::string& filename) {
  auto rel=std::filesystem::path(filename);
  if(rel.is_absolute())throw std::runtime_error("Path must be relative to the root.");
  std::error_code error;
  auto base=std::filesystem::weakly_canonical(std::filesystem::absolute(root),error);
  if(error)throw std::runtime_error("Static root could not be resolved.");
  auto path=std::filesystem::weakly_canonical(base/rel,error);
  if(error)throw std::runtime_error("Path could not be resolved.");
  auto root_part=base.begin(), path_part=path.begin();
  for(;root_part!=base.end()&&path_part!=path.end()&&*root_part==*path_part;++root_part,++path_part){}
  if(root_part!=base.end())throw std::runtime_error("Path escapes the root directory.");
  return path;
}
inline std::string se_static_read(const std::string& root,const std::string& filename) { auto p=se_web_safe_path(root,filename);std::ifstream in(p,std::ios::binary);if(!in)throw std::runtime_error("Static file could not be opened.");std::ostringstream out;out<<in.rdbuf();return out.str(); }
inline std::string se_upload_save(const std::string& root,const std::string& filename,const std::string& body) { auto p=se_web_safe_path(root,filename);std::filesystem::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Upload file could not be written.");out.write(body.data(),static_cast<std::streamsize>(body.size()));if(!out)throw std::runtime_error("Upload file write failed.");return p.string(); }
using SETileMap=SEList<std::string>;
inline SETileMap se_tilemap_parse(const std::string& text) { SETileMap rows;std::istringstream in(text);std::string row;std::size_t width=0;while(std::getline(in,row)){if(!row.empty()&&row.back()=='\r')row.pop_back();if(row.empty())continue;if(width==0)width=row.size();else if(row.size()!=width)throw std::runtime_error("Tilemap rows must have equal width.");rows.push_back(row);}if(rows.empty())throw std::runtime_error("Tilemap cannot be empty.");return rows; }
inline std::string se_tilemap_at(const SETileMap& map,std::int64_t x,std::int64_t y) { if(y<0||static_cast<std::size_t>(y)>=map.size()||x<0||static_cast<std::size_t>(x)>=map[static_cast<std::size_t>(y)].size())throw std::runtime_error("Tilemap coordinates are out of bounds.");return map[static_cast<std::size_t>(y)].substr(static_cast<std::size_t>(x),1); }
inline SEList<std::int64_t> se_tilemap_size(const SETileMap& map) { return {static_cast<std::int64_t>(map.front().size()),static_cast<std::int64_t>(map.size())}; }
