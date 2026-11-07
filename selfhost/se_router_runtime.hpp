#pragma once
#include <functional>
#include <unordered_map>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

struct SERouterResponse { std::int64_t status=200; std::string body; std::string type="text/plain; charset=utf-8"; };
struct SERouterRequest {
  std::string method,path,query,body;
  std::unordered_map<std::string,std::string> headers,params;
};
struct SERouterRoute {
  std::string method,path;
  std::function<SEAny(const std::string&)> handler;
};
struct SERouterState { std::vector<SERouterRoute> routes; SERouterRequest current; };
inline SERouterState& se_router_state(){static SERouterState state;return state;}
inline std::string se_router_lower(std::string value){std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return value;}
inline bool se_router_match(const std::string& pattern,const std::string& path,std::unordered_map<std::string,std::string>& params){
  std::size_t a=0,b=0;
  while(a<=pattern.size()&&b<=path.size()){
    auto ae=pattern.find('/',a),be=path.find('/',b);if(ae==std::string::npos)ae=pattern.size();if(be==std::string::npos)be=path.size();
    auto expected=pattern.substr(a,ae-a),actual=path.substr(b,be-b);
    if(!expected.empty()&&expected.front()==':'){if(actual.empty())return false;params[expected.substr(1)]=actual;}else if(expected!=actual)return false;
    if(ae==pattern.size()||be==path.size())return ae==pattern.size()&&be==path.size();a=ae+1;b=be+1;
  }return false;
}
inline SERouterResponse se_router_response(const SEAny& value){
  if(auto text=std::get_if<std::string>(&value))return {200,*text,"text/plain; charset=utf-8"};
  SERouterResponse response;
  auto dynamic=std::get_if<std::shared_ptr<SEDynamic>>(&value);if(!dynamic||!*dynamic)throw std::runtime_error("Web handler must return Text or a response object.");
  for(const auto& item:(*dynamic)->pairs()){
    if(item.first=="status"){if(auto n=std::get_if<std::int64_t>(&item.second))response.status=*n;}
    else if(item.first=="body"){if(auto t=std::get_if<std::string>(&item.second))response.body=*t;}
    else if(item.first=="type"){if(auto t=std::get_if<std::string>(&item.second))response.type=*t;}
  }return response;
}
template<class F> inline void se_router_add(const std::string& method,const std::string& path,F&& handler){
  if(path.empty()||path.front()!='/')throw std::runtime_error("Route path must start with '/'.");
  auto& routes=se_router_state().routes;
  for(const auto& route:routes)if(route.method==method&&route.path==path)throw std::runtime_error("Duplicate route.");
  routes.push_back({method,path,[fn=std::forward<F>(handler)](const std::string& body) mutable -> SEAny{return SEAny(fn(body));}});
}
inline void se_web_get(const std::string& path,auto&& handler){se_router_add("GET",path,std::forward<decltype(handler)>(handler));}
inline void se_web_post(const std::string& path,auto&& handler){se_router_add("POST",path,std::forward<decltype(handler)>(handler));}
inline void se_web_put(const std::string& path,auto&& handler){se_router_add("PUT",path,std::forward<decltype(handler)>(handler));}
inline void se_web_patch(const std::string& path,auto&& handler){se_router_add("PATCH",path,std::forward<decltype(handler)>(handler));}
inline void se_web_delete(const std::string& path,auto&& handler){se_router_add("DELETE",path,std::forward<decltype(handler)>(handler));}
inline SERouterResponse se_web_dispatch(const std::string& method,const std::string& target,const std::string& body,const std::unordered_map<std::string,std::string>& headers={}){
  auto& state=se_router_state();auto query=target.find('?');state.current.method=method;state.current.path=target.substr(0,query);state.current.query=query==std::string::npos?"":target.substr(query+1);state.current.body=body;state.current.params.clear();state.current.headers.clear();for(const auto& header:headers)state.current.headers[se_router_lower(header.first)]=header.second;
  for(const auto& route:state.routes){if(route.method!=method)continue;std::unordered_map<std::string,std::string> params;if(!se_router_match(route.path,state.current.path,params))continue;state.current.params=std::move(params);return se_router_response(route.handler(body));}
  return {404,"Not Found","text/plain; charset=utf-8"};
}
inline std::string se_web_handle(const std::string& method,const std::string& path,const std::string& body){return se_web_dispatch(method,path,body).body;}
inline std::int64_t se_web_handle_status(const std::string& method,const std::string& path,const std::string& body){return se_web_dispatch(method,path,body).status;}
inline std::int64_t se_web_route_count(){return static_cast<std::int64_t>(se_router_state().routes.size());}
inline std::string se_web_text(const std::string& body){return body;}
inline SEAny se_web_json(const std::string& body){SEMap<SEAny> out;out.put("status",SEAny(std::int64_t{200}));out.put("body",SEAny(body));out.put("type",SEAny(std::string("application/json; charset=utf-8")));return SEAny(out);}
inline SEAny se_web_response(std::int64_t status,const std::string& body,const std::string& type){SEMap<SEAny> out;out.put("status",SEAny(status));out.put("body",SEAny(body));out.put("type",SEAny(type));return SEAny(out);}
inline std::string se_web_method(){return se_router_state().current.method;}
inline std::string se_web_path(){return se_router_state().current.path;}
inline std::string se_web_query(){return se_router_state().current.query;}
inline std::string se_web_body(){return se_router_state().current.body;}
inline std::string se_web_header(const std::string& name){auto& h=se_router_state().current.headers;auto it=h.find(se_router_lower(name));return it==h.end()?std::string{}:it->second;}
inline std::string se_web_param(const std::string& name){auto& p=se_router_state().current.params;auto it=p.find(name);return it==p.end()?std::string{}:it->second;}
#ifdef _WIN32
using SERouterSocket=SOCKET;inline void se_router_close(SERouterSocket socket){closesocket(socket);}
#else
using SERouterSocket=int;inline void se_router_close(SERouterSocket socket){::close(socket);}
#endif
inline void se_router_send_all(SERouterSocket socket,const std::string& bytes){std::size_t sent=0;while(sent<bytes.size()){
#ifdef _WIN32
  int n=::send(socket,bytes.data()+sent,static_cast<int>(std::min<std::size_t>(bytes.size()-sent,1u<<20)),0);
#else
  auto n=::send(socket,bytes.data()+sent,bytes.size()-sent,0);
#endif
  if(n<=0)throw std::runtime_error("HTTP server send failed.");sent+=static_cast<std::size_t>(n);}}
inline void se_web_listen(std::int64_t port){
  if(port<1||port>65535)throw std::runtime_error("http_server.listen port must be 1..65535.");
#ifdef _WIN32
  WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)throw std::runtime_error("Could not initialize Winsock.");
#endif
  SERouterSocket server=::socket(AF_INET,SOCK_STREAM,0);
#ifdef _WIN32
  if(server==INVALID_SOCKET){WSACleanup();throw std::runtime_error("Could not create HTTP server socket.");}
#else
  if(server<0)throw std::runtime_error("Could not create HTTP server socket.");
#endif
  int reuse=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_ANY);address.sin_port=htons(static_cast<unsigned short>(port));
  if(::bind(server,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||::listen(server,64)!=0){se_router_close(server);
#ifdef _WIN32
    WSACleanup();
#endif
    throw std::runtime_error("Could not bind or listen on the requested HTTP port.");}
  for(;;){SERouterSocket client=::accept(server,nullptr,nullptr);
#ifdef _WIN32
    if(client==INVALID_SOCKET)continue;
#else
    if(client<0)continue;
#endif
    try{
      std::string request;char buffer[8192];std::size_t header_end=std::string::npos,content_length=0;
      while(request.size()<16u*1024u*1024u){
#ifdef _WIN32
        int count=::recv(client,buffer,sizeof(buffer),0);
#else
        auto count=::recv(client,buffer,sizeof(buffer),0);
#endif
        if(count<=0)break;request.append(buffer,static_cast<std::size_t>(count));header_end=request.find("\r\n\r\n");if(header_end!=std::string::npos){auto end=request.find("\r\n");std::size_t line=end==std::string::npos?request.size():end;auto headers_end=header_end;auto pos=line==request.size()?line:line+2;while(pos+2<=headers_end){auto next=request.find("\r\n",pos);if(next==std::string::npos||next>headers_end)break;auto colon=request.find(':',pos);if(colon!=std::string::npos&&colon<next&&se_router_lower(request.substr(pos,colon-pos))=="content-length"){try{content_length=static_cast<std::size_t>(std::stoull(se_web_trim(request.substr(colon+1,next-colon-1))));}catch(...){content_length=0;}}pos=next+2;}if(request.size()>=header_end+4+content_length)break;}}
      if(header_end==std::string::npos)throw std::runtime_error("Malformed HTTP request.");auto first_end=request.find("\r\n");std::istringstream start(request.substr(0,first_end));std::string method,target,version;if(!(start>>method>>target>>version))throw std::runtime_error("Malformed HTTP request line.");std::unordered_map<std::string,std::string> headers;std::size_t pos=first_end+2;while(pos<header_end){auto end=request.find("\r\n",pos);if(end==std::string::npos||end>header_end)break;auto colon=request.find(':',pos);if(colon!=std::string::npos&&colon<end)headers[se_router_lower(request.substr(pos,colon-pos))]=se_web_trim(request.substr(colon+1,end-colon-1));pos=end+2;}
      auto body_start=header_end+4;auto available=request.size()>body_start?std::min(content_length,request.size()-body_start):0;auto body=request.substr(body_start,available);auto response=se_web_dispatch(method,target,body,headers);std::string reason=response.status==200?"OK":response.status==201?"Created":response.status==400?"Bad Request":response.status==404?"Not Found":"Response";std::ostringstream out;out<<"HTTP/1.1 "<<response.status<<' '<<reason<<"\r\nContent-Type: "<<response.type<<"\r\nContent-Length: "<<response.body.size()<<"\r\nConnection: close\r\n\r\n"<<response.body;se_router_send_all(client,out.str());
    }catch(...){try{se_router_send_all(client,"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");}catch(...){}}
    se_router_close(client);
  }
}
