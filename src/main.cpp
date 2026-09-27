#include "s/bindgen.hpp"
#include "s/checker.hpp"
#include "s/compiler.hpp"
#include "s/component_web.hpp"
#include "s/error.hpp"
#include "s/interpreter.hpp"
#include "s/lexer.hpp"
#include "s/modules.hpp"
#include "s/parser.hpp"
#include "s/web_compiler.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
std::string read_file(const std::filesystem::path&p){std::ifstream f(p);if(!f)throw std::runtime_error("Could not open "+p.string());return {std::istreambuf_iterator<char>(f),{}};}
s::ast::Program frontend_file(const std::filesystem::path&path){s::ModuleLoader loader;auto program=loader.load(path);s::Checker{}.check(program);return program;}
std::string shell_quote(const std::string&s){std::string r="'";for(char c:s){if(c=='\'')r+="'\\''";else r+=c;}return r+"'";}

bool is_se_source(const std::filesystem::path&path){auto ext=path.extension().string();return ext==".se"||ext==".s";}

std::vector<std::filesystem::path> source_files(const std::filesystem::path&root){
  std::vector<std::filesystem::path> out;
  if(std::filesystem::is_regular_file(root)){if(is_se_source(root))out.push_back(root);return out;}
  if(!std::filesystem::exists(root))throw std::runtime_error("Path does not exist: "+root.string());
  for(const auto&entry:std::filesystem::recursive_directory_iterator(root)){
    if(entry.is_regular_file()&&is_se_source(entry.path()))out.push_back(entry.path());
  }
  std::sort(out.begin(),out.end());
  return out;
}

void write_new_file(const std::filesystem::path&path,const std::string&content){
  if(std::filesystem::exists(path))throw std::runtime_error("Refusing to overwrite "+path.string());
  if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path,std::ios::binary);
  if(!out)throw std::runtime_error("Could not create "+path.string());
  out<<content;
}

void write_output_file(const std::filesystem::path&path,const std::string&content){
  if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path,std::ios::binary|std::ios::trunc);
  if(!out)throw std::runtime_error("Could not write "+path.string());
  out<<content;
}

void print_help(){
  std::cout<<
    "SE - simple at every level\n\n"
    "Commands:\n"
    "  se run file.se                 Run an SE program\n"
    "  se check file.se               Parse and type-check one file\n"
    "  se check-all [path]            Check every .se file in a project\n"
    "  se test [path]                 Run *_test.se files recursively\n"
    "  se build file.se               Build a native executable\n"
    "  se web build file.se [dist]    Compile one SE browser file to HTML/CSS/JS/TS\n"
    "  se bind module.sbind [dir]     Generate C ABI bindings\n"
    "  se new app NAME                Create a normal SE application\n"
    "  se new web NAME                Create an SE web/API + HTML/CSS/JS/TS project\n"
    "  se doctor                      Show local toolchain diagnostics\n"
    "  se help                        Show this help\n"
    "  se --version                   Show the SE version\n\n"
    "Built-in modules include core platform modules plus statistics, regex/re, decimal, csv, datetime, hash/hashlib, base64, uuid, iter/itertools, args/argparse, logging, file/archive/process/socket, threading/queue, sqlite, functools, operator, copy, enum and typing.\n"
    "Source files use .se. Legacy .s files are still accepted during migration.\n";
}

int doctor_command(){
  std::cout<<"SE doctor\n";
  std::cout<<"  version: SE 0.7.0\n";
#ifdef _WIN32
  std::cout<<"  platform: Windows\n";
#elif __APPLE__
  std::cout<<"  platform: macOS\n";
#elif __linux__
  std::cout<<"  platform: Linux\n";
#else
  std::cout<<"  platform: unknown\n";
#endif
  if(const char*cxx=std::getenv("CXX"))std::cout<<"  CXX: "<<cxx<<"\n";
  else std::cout<<"  CXX: not set (SE will use c++)\n";
  if(const char*home=std::getenv("SE_HOME"))std::cout<<"  SE_HOME: "<<home<<"\n";
  else if(const char*legacy=std::getenv("S_HOME"))std::cout<<"  S_HOME: "<<legacy<<" (legacy)\n";
  else std::cout<<"  package home: default search paths\n";
  std::cout<<"  Browser web build: built into SE; generated sites need only a browser/static host\n";
  std::cout<<"  MongoDB adapter: requires node plus the official mongodb package when db.exec is used\n";
  std::cout<<"  GAS adapter: requires curl when db.exec is used\n";
  std::cout<<"  Node bridge: requires node on PATH for use js\n";
  std::cout<<"  TypeScript bridge: requires ts-node/tsc on PATH for use ts\n";
  std::cout<<"  HTTPS bridge: requires curl on PATH for use https\n";
  std::cout<<"  current directory: "<<std::filesystem::current_path().string()<<"\n";
  std::cout<<"Doctor finished. Use 'se check file.se' or 'se check-all .' for source diagnostics.\n";
  return 0;
}

int check_all_command(const std::filesystem::path&root){
  auto files=source_files(root);
  if(files.empty()){std::cout<<"No SE source files found under "<<root.string()<<"\n";return 0;}
  std::size_t passed=0,failed=0;
  for(const auto&path:files){
    try{frontend_file(path);++passed;std::cout<<"PASS "<<path.string()<<'\n';}
    catch(const s::Error&e){++failed;std::cerr<<"FAIL "<<path.string()<<'\n'<<s::format_error(e,read_file(path));}
    catch(const std::exception&e){++failed;std::cerr<<"FAIL "<<path.string()<<": "<<e.what()<<'\n';}
  }
  std::cout<<"Checked "<<files.size()<<" file(s): "<<passed<<" passed, "<<failed<<" failed.\n";
  return failed?1:0;
}

bool is_test_file(const std::filesystem::path&path){
  auto name=path.filename().string();
  return name.size()>=8&&(name.ends_with("_test.se")||name.ends_with("_test.s"));
}

int test_command(const std::filesystem::path&root){
  auto all=source_files(root);
  std::vector<std::filesystem::path> tests;
  std::copy_if(all.begin(),all.end(),std::back_inserter(tests),is_test_file);
  if(tests.empty()){std::cout<<"No *_test.se files found under "<<root.string()<<"\n";return 0;}
  std::size_t passed=0,failed=0;
  for(const auto&path:tests){
    try{
      auto program=frontend_file(path);
      std::ostringstream captured;
      std::istringstream input;
      s::Interpreter vm(input,captured);
      vm.run(program);
      ++passed;
      std::cout<<"PASS "<<path.string()<<'\n';
      auto text=captured.str();if(!text.empty())std::cout<<text;
    }catch(const s::Error&e){++failed;std::cerr<<"FAIL "<<path.string()<<'\n'<<s::format_error(e,read_file(path));}
    catch(const s::RuntimeFailure&e){++failed;const auto&x=e.error();std::cerr<<"FAIL "<<path.string()<<": "<<x.kind<<": "<<x.message<<'\n';}
    catch(const std::exception&e){++failed;std::cerr<<"FAIL "<<path.string()<<": "<<e.what()<<'\n';}
  }
  std::cout<<"Tests: "<<passed<<" passed, "<<failed<<" failed.\n";
  return failed?1:0;
}

int web_build_command(const std::filesystem::path&source_path,const std::filesystem::path&output){
  auto source=read_file(source_path);
  try{
    s::Lexer lexer(source);s::Parser parser(lexer.scan());auto program=parser.parse();
    auto bundle=s::has_component_web(program)?s::ComponentWebCompiler{}.generate(program):s::WebCompiler{}.generate(program);
    std::filesystem::create_directories(output);
    write_output_file(output/"index.html",bundle.html);
    write_output_file(output/"style.css",bundle.css);
    write_output_file(output/"app.js",bundle.js);
    write_output_file(output/"app.ts",bundle.ts);
    std::cout<<"Built SE website in "<<output.string()<<"\n";
    std::cout<<"  index.html\n  style.css\n  app.js\n  app.ts\n";
    return 0;
  }catch(const s::Error&e){std::cerr<<s::format_error(e,source);return 1;}
}

int new_project(const std::string&kind,const std::filesystem::path&root){
  if(root.empty())throw std::runtime_error("Project name cannot be empty.");
  if(std::filesystem::exists(root))throw std::runtime_error("Project path already exists: "+root.string());
  std::filesystem::create_directories(root);
  if(kind=="app"){
    write_new_file(root/"src/main.se","say \"Hello from SE\"\n");
    write_new_file(root/"tests/main_test.se","use test\n\nvalue = 2 + 2\ntest.equal value 4\n");
    write_new_file(root/"README.md","# "+root.filename().string()+"\n\nRun:\n\n```sh\nse run src/main.se\n```\n\nCheck the project:\n\n```sh\nse check-all .\n```\n\nRun tests:\n\n```sh\nse test .\n```\n\nBuild:\n\n```sh\nse build src/main.se\n```\n");
    std::cout<<"Created SE app at "<<root.string()<<"\n";
    std::cout<<"Next: cd "<<root.string()<<" && se check-all . && se test . && se run src/main.se\n";
    return 0;
  }
  if(kind=="web"){
    write_new_file(root/"backend/main.se",
      "use web\n"
      "use json\n\n"
      "make home body\n"
      "    give \"Hello from SE Web\"\n\n"
      "make hello body\n"
      "    name = web.param \"name\"\n"
      "    data = [\"message\": \"Hello \" + name, \"received\": body]\n"
      "    payload = json.stringify data\n"
      "    give web.json payload\n\n"
      "web.get \"/\" home\n"
      "web.get \"/api/hello/:name\" hello\n\n"
      "say \"SE web server: http://localhost:8080\"\n"
      "try web.listen 8080\n");
    write_new_file(root/"backend/tests/backend_test.se",
      "use web\nuse json\nuse test\n\n"
      "make hello body\n"
      "    name = web.param \"name\"\n"
      "    data = [\"message\": \"Hello \" + name]\n"
      "    payload = json.stringify data\n"
      "    give web.json payload\n\n"
      "web.get \"/api/hello/:name\" hello\n"
      "status = web.handle_status \"GET\" \"/api/hello/SE\" \"\"\n"
      "body = web.handle \"GET\" \"/api/hello/SE\" \"\"\n"
      "test.equal status 200\n"
      "has_name = body != \"\"\n"
      "test.ok has_name\n");
    write_new_file(root/"frontend/app.se",
      "make Button text\n"
      "    html\n"
      "        button text\n\n"
      "    css\n"
      "        padding 12\n"
      "        border_radius 8\n\n"
      "    js\n"
      "        when click\n"
      "            say text\n\n"
      "make Card text\n"
      "    text text\n\n"
      "    style\n"
      "        padding 20\n\n"
      "page \"/\"\n"
      "    Card \"Hello from SE Web\"\n"
      "    Button \"Save\"\n"
      "    Button \"Cancel\"\n");
    write_new_file(root/"README.md",
      "# "+root.filename().string()+"\n\nSE web project with an SE backend and a browser frontend authored in SE.\n\n- `backend/main.se` - SE HTTP server/router\n- `backend/tests/` - backend route tests\n- `frontend/app.se` - reusable HTML/CSS/JS components in normal SE `make` blocks\n\nBuild frontend:\n\n```sh\nse web build frontend/app.se frontend/dist\n```\n\nThis generates deployable `index.html`, `style.css`, `app.js`, and `app.ts`.\n\nRun backend:\n\n```sh\nse check-all backend\nse test backend\nse run backend/main.se\n```\n\nThe browser compiler is a growing SE subset. Keep database credentials on the backend; do not place MongoDB or other private connection strings in frontend SE files.\n");
    std::cout<<"Created SE web project at "<<root.string()<<"\n";
    std::cout<<"Frontend: cd "<<root.string()<<" && se web build frontend/app.se frontend/dist\n";
    std::cout<<"Backend:  se test backend && se run backend/main.se\n";
    return 0;
  }
  throw std::runtime_error("Unknown project kind '"+kind+"'. Use app or web.");
}

int file_command(const std::string&cmd,const std::filesystem::path&path){
  auto source=read_file(path);
  try{auto p=frontend_file(path);if(cmd=="check"){std::cout<<path.string()<<" is valid SE.\n";return 0;}if(cmd=="run"){s::Interpreter vm(std::cin,std::cout);vm.run(p);return 0;}
    auto cpp=s::CppCompiler{}.generate(p);auto cpp_path=path;cpp_path.replace_extension(".se.cpp");auto output=path;output.replace_extension();{std::ofstream f(cpp_path);f<<cpp;}
    const char*cxx=std::getenv("CXX");std::string compiler=cxx?cxx:"c++";
#ifdef S_SOURCE_ROOT
    std::filesystem::path root=S_SOURCE_ROOT;
#else
    std::filesystem::path root=std::filesystem::current_path();
#endif
    std::string command=compiler+" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation -I"+shell_quote((root/"include").string())+" "+shell_quote(cpp_path.string())+" "+shell_quote((root/"src/runtime/error.cpp").string())+" "+shell_quote((root/"src/runtime/value.cpp").string())+" "+shell_quote((root/"src/runtime/platform_base.cpp").string())+" "+shell_quote((root/"src/runtime/advanced.cpp").string())+" "+shell_quote((root/"src/runtime/database.cpp").string())+" "+shell_quote((root/"src/interpreter/interpreter.cpp").string())+" "+shell_quote((root/"src/ffi/ffi.cpp").string())+" -pthread";
#ifdef __linux__
    command+=" -ldl";
#endif
    command+=" -o "+shell_quote(output.string());int code=std::system(command.c_str());std::filesystem::remove(cpp_path);if(code!=0)throw std::runtime_error("The C++ compiler could not build the generated program.");std::cout<<"Built "<<output.string()<<"\n";return 0;
  }catch(const s::Error&e){std::cerr<<s::format_error(e,source);return 1;}catch(const s::RuntimeFailure&e){auto&x=e.error();std::cerr<<x.kind;if(!x.source.empty())std::cerr<<" in "<<x.source;if(x.line)std::cerr<<" on line "<<x.line;std::cerr<<": "<<x.message<<'\n';return 1;}
}
int bind_command(const std::filesystem::path&definition,const std::filesystem::path&output){
  try{
    auto files=s::generate_bindings(definition,output);
    std::cout<<"Generated "<<files.metadata.string()<<'\n';
    std::cout<<"Generated "<<files.header.string()<<'\n';
    std::cout<<"Generated "<<files.source.string()<<'\n';
    return 0;
  }catch(const s::Error&e){std::string source;try{source=read_file(definition);}catch(...){ }std::cerr<<s::format_error(e,source);return 1;}
}
void repl(){
  std::cout<<"SE 0.7.0\nType SE code. Use a blank line to finish a block. Ctrl-D exits.\n";
  std::string pending,line; s::Checker checker; s::Interpreter vm(std::cin,std::cout);
  auto execute=[&]{if(pending.empty())return;try{s::Lexer lexer(pending);s::Parser parser(lexer.scan());auto program=parser.parse();checker.check(program);vm.run(program);}catch(const s::Error&e){std::cerr<<s::format_error(e,pending);}catch(const s::RuntimeFailure&e){std::cerr<<e.what()<<'\n';}pending.clear();};
  while(true){std::cout<<(pending.empty()?"> ":". ");if(!std::getline(std::cin,line)){execute();break;}if(line.empty()&&!pending.empty()){execute();continue;}pending+=line+'\n';if(line.find_first_not_of(' ')==0&&line.rfind("if ",0)!=0&&line.rfind("for ",0)!=0&&line.rfind("while ",0)!=0&&line.rfind("repeat ",0)!=0&&line.rfind("make ",0)!=0&&line.rfind("type ",0)!=0&&line.rfind("match ",0)!=0&&line!="try")execute();}
}
}
int main(int argc,char**argv){
  try{
    if(argc==1){repl();return 0;}
    if(argc==2&&std::string(argv[1])=="--version"){std::cout<<"SE 0.7.0\n";return 0;}
    if(argc==2&&(std::string(argv[1])=="help"||std::string(argv[1])=="--help"||std::string(argv[1])=="-h")){print_help();return 0;}
    if(argc==2&&std::string(argv[1])=="doctor")return doctor_command();
    if((argc==2||argc==3)&&std::string(argv[1])=="check-all")return check_all_command(argc==3?std::filesystem::path(argv[2]):std::filesystem::path("."));
    if((argc==2||argc==3)&&std::string(argv[1])=="test")return test_command(argc==3?std::filesystem::path(argv[2]):std::filesystem::path("."));
    if(argc==4&&std::string(argv[1])=="new")return new_project(argv[2],argv[3]);
    if((argc==4||argc==5)&&std::string(argv[1])=="web"&&std::string(argv[2])=="build")return web_build_command(argv[3],argc==5?std::filesystem::path(argv[4]):std::filesystem::path("dist"));
    if(argc>=3&&std::string(argv[1])=="bind"){
      if(argc>4){std::cerr<<"Use: se bind module.sbind [output-directory]\n";return 2;}
      std::filesystem::path definition=argv[2];
      std::filesystem::path output=argc==4?std::filesystem::path(argv[3]):definition.parent_path();
      if(output.empty())output=".";
      return bind_command(definition,output);
    }
    if(argc!=3){print_help();return 2;}
    std::string cmd=argv[1];if(cmd!="run"&&cmd!="check"&&cmd!="build"){std::cerr<<"Unknown command '"<<cmd<<"'.\n\n";print_help();return 2;}return file_command(cmd,argv[2]);
  }catch(const std::exception&e){std::cerr<<"Error: "<<e.what()<<'\n';return 1;}
}