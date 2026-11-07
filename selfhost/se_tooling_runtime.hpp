#pragma once

inline SEProcessCapture se_tooling_capture(SEList<std::string> arguments){auto result=se_process_capture_argv(arguments);if(result.status!=0)throw std::runtime_error("External tool exited with status "+std::to_string(result.status)+".");return result;}
inline std::int64_t se_node_run(const std::string& file){return se_process_argv(SEList<std::string>{"node",file});}
inline std::string se_node_output(const std::string& file){return se_tooling_capture(SEList<std::string>{"node",file}).output;}
inline std::string se_node_eval(const std::string& code){return se_tooling_capture(SEList<std::string>{"node","-e",code}).output;}
inline std::string se_node_version(){return se_tooling_capture(SEList<std::string>{"node","--version"}).output;}
inline std::int64_t se_node_npm(const std::string& source){auto args=se_process_split_arguments(source);SEList<std::string> argv{"npm"};for(const auto& arg:args)argv.push_back(arg);return se_process_argv(argv);}
inline std::int64_t se_node_npx(const std::string& source){auto args=se_process_split_arguments(source);SEList<std::string> argv{"npx"};for(const auto& arg:args)argv.push_back(arg);return se_process_argv(argv);}
inline std::int64_t se_js_run(const std::string& file){return se_node_run(file);}
inline std::string se_js_output(const std::string& file){return se_node_output(file);}
inline std::string se_js_eval(const std::string& code){return se_node_eval(code);}
inline std::int64_t se_ts_run(const std::string& file){return se_process_argv(SEList<std::string>{"ts-node",file});}
inline std::int64_t se_ts_compile(const std::string& file){return se_process_argv(SEList<std::string>{"tsc","--pretty","false",file});}
inline std::string se_ts_output(const std::string& file){return se_tooling_capture(SEList<std::string>{"ts-node",file}).output;}
inline std::int64_t se_next_create(const std::string& project){return se_process_argv(SEList<std::string>{"npx","--yes","create-next-app@latest",project});}
inline std::int64_t se_next_command(const std::string& project,const std::string& command){return se_process_argv(SEList<std::string>{"npm","--prefix",project,"run",command});}
