#pragma once
#include <cstdio>
#include <cerrno>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <process.h>
#include <windows.h>
#endif
inline std::int64_t se_process_run(const std::string& command){
  if(command.find('\0')!=std::string::npos)throw std::runtime_error("Process command contains NUL.");
  int code=std::system(command.c_str());
#ifndef _WIN32
  if(code!=-1&&WIFEXITED(code))return WEXITSTATUS(code);
#endif
  return code;
}
inline std::string se_process_output(const std::string& command){
  if(command.find('\0')!=std::string::npos)throw std::runtime_error("Process command contains NUL.");
#ifdef _WIN32
  FILE* pipe=_popen(command.c_str(),"rb");
  auto close=[](FILE* p){_pclose(p);};
#else
  FILE* pipe=popen(command.c_str(),"r");
  auto close=[](FILE* p){pclose(p);};
#endif
  if(!pipe)throw std::runtime_error("Could not start process.");
  std::unique_ptr<FILE,decltype(close)> owner(pipe,close);
  std::string out;char buffer[4096];std::size_t n;
  while((n=std::fread(buffer,1,sizeof buffer,pipe))!=0)out.append(buffer,n);
  if(std::ferror(pipe))throw std::runtime_error("Could not read process output.");
  return out;
}
inline std::int64_t se_process_argv(const SEList<std::string>& arguments){
  if(arguments.empty())throw std::runtime_error("Missing process executable.");
  for(const auto& arg:arguments)if(arg.find('\0')!=std::string::npos)throw std::runtime_error("Process argument contains NUL.");
#ifdef _WIN32
  std::vector<const char*> argv;for(const auto& arg:arguments)argv.push_back(arg.c_str());argv.push_back(nullptr);
  return static_cast<std::int64_t>(_spawnvp(_P_WAIT,argv[0],argv.data()));
#else
  std::vector<char*> argv;for(const auto& arg:arguments)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);
  auto child=fork();if(child<0)throw std::runtime_error("Could not create child process.");
  if(child==0){execvp(argv[0],argv.data());_exit(127);}
  int status=0;while(waitpid(child,&status,0)<0){if(errno!=EINTR)throw std::runtime_error("Could not wait for child process.");}
  if(WIFEXITED(status))return WEXITSTATUS(status);if(WIFSIGNALED(status))return 128+WTERMSIG(status);return -1;
#endif
}
template<class... Args> inline std::int64_t se_process_exec(const std::string& command,const Args&... args){
  SEList<std::string> argv{command,std::string(args)...};
  return se_process_argv(argv);
}
struct SEProcessCapture { std::string output; std::int64_t status=0; };
inline SEProcessCapture se_process_capture_argv(const SEList<std::string>& arguments){
  if(arguments.empty())throw std::runtime_error("Missing process executable.");
  for(const auto& arg:arguments)if(arg.find('\0')!=std::string::npos)throw std::runtime_error("Process argument contains NUL.");
#ifdef _WIN32
  auto quote=[](const std::string& arg){std::string out="\"";std::size_t slashes=0;for(char c:arg){if(c=='\\'){++slashes;continue;}if(c=='\"'){out.append(slashes*2+1,'\\');out.push_back('\"');slashes=0;continue;}out.append(slashes,'\\');slashes=0;out.push_back(c);}out.append(slashes*2,'\\');out.push_back('\"');return out;};
  std::string command;for(const auto& arg:arguments){if(!command.empty())command.push_back(' ');command+=quote(arg);}
  SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};HANDLE read_handle=nullptr,write_handle=nullptr;
  if(!CreatePipe(&read_handle,&write_handle,&security,0))throw std::runtime_error("Could not create process output pipe.");
  SetHandleInformation(read_handle,HANDLE_FLAG_INHERIT,0);STARTUPINFOA startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdOutput=write_handle;startup.hStdError=GetStdHandle(STD_ERROR_HANDLE);startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);PROCESS_INFORMATION process{};
  std::vector<char> mutable_command(command.begin(),command.end());mutable_command.push_back('\0');
  BOOL started=CreateProcessA(nullptr,mutable_command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);CloseHandle(write_handle);
  if(!started){CloseHandle(read_handle);throw std::runtime_error("Could not start process.");}
  SEProcessCapture result;char buffer[4096];DWORD count=0;while(ReadFile(read_handle,buffer,sizeof(buffer),&count,nullptr)&&count){if(result.output.size()+count>64*1024*1024){TerminateProcess(process.hProcess,1);CloseHandle(read_handle);CloseHandle(process.hThread);CloseHandle(process.hProcess);throw std::runtime_error("Process output exceeds 64 MiB.");}result.output.append(buffer,count);}
  CloseHandle(read_handle);WaitForSingleObject(process.hProcess,INFINITE);DWORD status=1;GetExitCodeProcess(process.hProcess,&status);result.status=static_cast<std::int64_t>(status);CloseHandle(process.hThread);CloseHandle(process.hProcess);return result;
#else
  int descriptors[2];if(pipe(descriptors)!=0)throw std::runtime_error("Could not create process output pipe.");
  auto child=fork();if(child<0){close(descriptors[0]);close(descriptors[1]);throw std::runtime_error("Could not create child process.");}
  if(child==0){close(descriptors[0]);if(dup2(descriptors[1],STDOUT_FILENO)<0)_exit(126);close(descriptors[1]);std::vector<char*> argv;for(const auto& arg:arguments)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);execvp(argv[0],argv.data());_exit(127);}
  close(descriptors[1]);SEProcessCapture result;char buffer[4096];for(;;){auto count=read(descriptors[0],buffer,sizeof(buffer));if(count==0)break;if(count<0){if(errno==EINTR)continue;close(descriptors[0]);kill(child,SIGTERM);waitpid(child,nullptr,0);throw std::runtime_error("Could not read process output.");}if(result.output.size()+static_cast<std::size_t>(count)>64*1024*1024){close(descriptors[0]);kill(child,SIGTERM);waitpid(child,nullptr,0);throw std::runtime_error("Process output exceeds 64 MiB.");}result.output.append(buffer,static_cast<std::size_t>(count));}
  close(descriptors[0]);int status=0;while(waitpid(child,&status,0)<0){if(errno!=EINTR)throw std::runtime_error("Could not wait for child process.");}
  if(WIFEXITED(status))result.status=WEXITSTATUS(status);else if(WIFSIGNALED(status))result.status=128+WTERMSIG(status);else result.status=-1;return result;
#endif
}
inline SEList<std::string> se_process_split_arguments(const std::string& source){
  SEList<std::string> result;std::string current;char quote=0;bool escaped=false,started=false;
  for(char c:source){if(escaped){current.push_back(c);escaped=false;started=true;continue;}if(c=='\\'&&quote!='\''){escaped=true;started=true;continue;}if(quote){if(c==quote)quote=0;else current.push_back(c);started=true;continue;}if(c=='\''||c=='\"'){quote=c;started=true;continue;}if(std::isspace(static_cast<unsigned char>(c))){if(started){result.push_back(current);current.clear();started=false;}continue;}current.push_back(c);started=true;}
  if(escaped||quote)throw std::runtime_error("Unclosed quote or escape in process arguments.");if(started)result.push_back(current);return result;
}
