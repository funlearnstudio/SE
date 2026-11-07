#pragma once
#include <cstdio>
#include <cerrno>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <process.h>
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
