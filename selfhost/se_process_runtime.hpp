#pragma once
#include <cstdio>
#include <cerrno>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <signal.h>
#else
#include <process.h>
#include <windows.h>
#endif
inline std::int64_t se_process_run(const std::string& command){
  if(command.find('\0')!=std::string::npos)throw std::runtime_error("Process command contains NUL.");
  int code=std::system(command.c_str());
#ifndef _WIN32
  if(code!=-1&&WIFEXITED(code))return WEXITSTATUS(code);
