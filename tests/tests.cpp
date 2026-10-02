#include "s/checker.hpp"
#include "s/error.hpp"
#include "s/ffi.hpp"
#include "s/interpreter.hpp"
#include "s/lexer.hpp"
#include "s/modules.hpp"
#include "s/parser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static int failures=0;
#define EXPECT(c) do{if(!(c)){std::cerr<<"FAIL "<<__LINE__<<": "#c"\n";++failures;}}while(0)

s::ast::Program parse(const std::string&x){s::Lexer l(x);s::Parser p(l.scan());return p.parse();}
void check(const std::string&x){auto p=parse(x);s::Checker{}.check(p);}
std::string run(const std::string&x){auto p=parse(x);s::Checker{}.check(p);std::istringstream in;std::ostringstream out;s::Interpreter vm(in,out);vm.run(p);return out.str();}
std::string run_project(const std::filesystem::path&p){s::ModuleLoader l;auto program=l.load(p);s::Checker{}.check(program);std::istringstream in;std::ostringstream out;s::Interpreter vm(in,out);vm.run(program);return out.str();}
template<class F>bool throws_error(F f){try{f();return false;}catch(const s::Error&){return true;}}
template<class F>bool throws_runtime(F f){try{f();return false;}catch(const s::RuntimeFailure&){return true;}}

int main(){
  // S 0.1 compatibility.
  {s::Lexer l("if true\n    say 1\nsay 2\n");auto t=l.scan();int i=0,d=0;for(auto&x:t){i+=x.kind==s::TokenKind::Indent;d+=x.kind==s::TokenKind::Dedent;}EXPECT(i==1&&d==1);}
  EXPECT(run("say 2 + 3 * 4\n")=="14\n");
  EXPECT(run("if 3 > 2\n    repeat 2\n        say \"yes\"\nelse\n    say \"no\"\n")=="yes\nyes\n");
  EXPECT(run("total = 0\nfor x in 1..4\n    total = total + x\nsay total\n")=="10\n");
  EXPECT(run("make add a b\n    give a + b\nsay add 4 5\n")=="9\n");
  // Arithmetic belongs to bare-call arguments, independent of whitespace.
  const std::string fib_compact=
    "s = 5\\n"
    "make func a\\n"
    "    if a <= 1\\n"
    "        give a\\n"
    "    ans = func a-1 + func a-2\\n"
    "    give ans\\n"
    "say func s\\n";
  const std::string fib_spaced=
    "s = 5\\n"
    "make func a\\n"
    "    if a <= 1\\n"
    "        give a\\n"
    "    ans = func a - 1 + func a - 2\\n"
    "    give ans\\n"
    "say func s\\n";
  EXPECT(run(fib_compact)=="5\\n");
  EXPECT(run(fib_spaced)=="5\\n");
  EXPECT(run("x = [1, 2]\nx[0] = 9\nsay x[0]\nsay x.len\n")=="9\n2\n");

  // Core conversions and familiar aliases.
  EXPECT(run("say int \"42\"\nsay integer \"-7\"\nsay double \"3.5\"\nsay float 2\nsay string 99\nsay text true\nsay bool \"TRUE\"\nsay boolean \"false\"\nsay char \"é\"\nsay char 65\n")=="42\n-7\n3.5\n2\n99\ntrue\ntrue\nfalse\né\nA\n");
  EXPECT(run("number = int \"12\"\nwhile number < 14\n    number = number + 1\nsay number\n")=="14\n");
  EXPECT(throws_error([]{run("say int \"12x\"\n");}));
  EXPECT(throws_error([]{run("say char \"ab\"\n");}));

  // User-defined types, default/explicit initialization, methods, inference, instances.
  const std::string type_program=
    "type Player\n"
    "    name = \"\"\n"
    "    hp = 100\n"
    "    make hit damage\n"
    "        hp = hp - damage\n"
    "    make alive\n"
    "        give hp > 0\n"
    "a = Player\n"
    "    name = \"A\"\n"
    "b = Player\n"
    "    name = \"B\"\n"
    "a.hit 20\n"
    "say a.name\n"
    "say a.hp\n"
    "say b.hp\n"
    "say a.alive\n";
  EXPECT(run(type_program)=="A\n80\n100\ntrue\n");
  EXPECT(throws_error([]{check("type Dog\n    age = 0\nd = Dog\nd.age = \"old\"\n");}));
  EXPECT(throws_error([]{check("type Dog\n    age = 0\nd = Dog\nsay d.name\n");}));
  EXPECT(throws_error([]{check("type Dog\n    age = 0\nd = Dog\nd.fly\n");}));
  EXPECT(run("type Point\n    x = 3\ntype Box\n    point = Point\nb = Box\nsay b.point.x\n")=="3\n");

  // Collections.
  EXPECT(run("nums = [1, 2, 3]\nnums.add 4\nnums.remove 2\nfor n in nums\n    say n\n")=="1\n3\n4\n");
  EXPECT(run("m = [\"a\": 1, \"b\": 2]\nsay m[\"a\"]\nfor key value in m\n    say key\n    say value\n")=="1\na\n1\nb\n2\n");
  EXPECT(run("s = set [1, 2, 2, 3]\ns.add 3\ns.add 4\nsay s.len\nif 3 in s\n    say \"yes\"\n")=="4\nyes\n");
  EXPECT(throws_error([]{check("nums = [1, 2]\nnums.add \"bad\"\n");}));
  EXPECT(throws_error([]{check("nums = [1, \"bad\"]\n");}));
  EXPECT(throws_error([]{run("x = [1]\nsay x[4]\n");}));
  EXPECT(run("empty = []\nsay empty.len\ns = set []\nsay s.len\n")=="0\n0\n");

  // Error model.
  EXPECT(run("try\n    fail \"boom\"\nelse err\n    say err.kind\n    say err\n")=="UserError\nboom\n");
  EXPECT(run("try\n    try\n        fail \"inner\"\n    else first\n        say first\nelse outer\n    say outer\n")=="inner\n");
  EXPECT(throws_error([]{check("text = read \"missing.txt\"\n");}));
  EXPECT(run("make load\n    give try read \"definitely-missing-s-file.txt\"\ntry\n    say load\nelse err\n    say err.kind\n")=="FileError\n");

  // File + Unicode + empty file + automatic FileData cleanup.
  auto temp=std::filesystem::temp_directory_path()/"s_02_runtime_test.txt";
  auto path=temp.generic_string();
  std::string file_program="try\n    write \""+path+"\" \"中文 hello\"\n    append \""+path+"\" \"!\"\n    text = read \""+path+"\"\n    say text\nelse err\n    say err\n";
  EXPECT(run(file_program)=="中文 hello!\n");
  std::string empty_program="try\n    write \""+path+"\" \"\"\n    text = read \""+path+"\"\n    say text.len\nelse err\n    say err\n";
  EXPECT(run(empty_program)=="0\n");
  {
    std::ofstream seed(temp);seed<<"resource";seed.close();
    std::string open_program="try\n    file = open \""+path+"\"\n    say file.read\nelse err\n    say err\n";
    EXPECT(run(open_program)=="resource\n");
  }
  std::filesystem::remove(temp);
  EXPECT(run("try\n    text = read \"definitely-missing-s-file.txt\"\n    say text\nelse err\n    say err.kind\n")=="FileError\n");

  // Path and time.
  EXPECT(run("use_dummy = 1\nsay 1ms\nsay 2s\nsay 1min\nwait 1ms\n")=="1ms\n2s\n1min\n");
  EXPECT(throws_error([]{parse("if true\n   say 1\n");}));

#ifdef S_TEST_ROOT
  const auto root=std::filesystem::path(S_TEST_ROOT);
  EXPECT(run_project(root/"examples/modules/main.se")=="80\nstill alive\n");
  EXPECT(run_project(root/"examples/paths.se").find("user.txt")!=std::string::npos);
  EXPECT(run_project(root/"examples/time.se").find("done\n")!=std::string::npos);

  // Missing and circular modules.
  auto mod_dir=std::filesystem::temp_directory_path()/"s_02_modules_test";
  std::filesystem::remove_all(mod_dir);std::filesystem::create_directories(mod_dir);
  {std::ofstream a(mod_dir/"a.s");a<<"use b\nsay 1\n";std::ofstream b(mod_dir/"b.s");b<<"use a\nsay 2\n";}
  EXPECT(throws_error([&]{s::ModuleLoader l;(void)l.load(mod_dir/"a.s");}));
  {std::ofstream m(mod_dir/"missing.s");m<<"use nowhere\n";}
  EXPECT(throws_error([&]{s::ModuleLoader l;(void)l.load(mod_dir/"missing.s");}));
  std::filesystem::remove_all(mod_dir);

  // C ABI scalar calls, native errors, opaque-handle ownership and cleanup.
  s::ModuleLoader native_loader;auto native_program=native_loader.load(root/"examples/native/main.se");
  const s::ast::Module* native_meta=nullptr;for(auto&module:native_program.modules)if(module.native)native_meta=&module;
  EXPECT(native_meta!=nullptr);
  if(native_meta){
    auto native=s::load_native_module(*native_meta);
    auto invoke=[&](const std::string&name,std::vector<s::Value> args={}){auto value=native->exports.at(name);auto c=std::get<std::shared_ptr<s::CallableData>>(value.data());return c->call(args,{1,1});};
    EXPECT(std::get<std::int64_t>(invoke("add",{s::Value(std::int64_t{5}),s::Value(std::int64_t{3})}).data())==8);
    EXPECT(std::get<double>(invoke("square",{s::Value(4.0)}).data())==16.0);
    EXPECT(std::get<bool>(invoke("not_bool",{s::Value(false)}).data()));
    EXPECT(std::get<std::string>(invoke("echo",{s::Value(std::string("hello"))}).data())=="hello");
    EXPECT(throws_runtime([&]{(void)invoke("native_error",{s::Value(std::int64_t{1})});}));
    EXPECT(std::get<std::int64_t>(invoke("live_counters").data())==0);
    {auto handle=invoke("make_counter");EXPECT(std::get<std::int64_t>(invoke("live_counters").data())==1);EXPECT(std::get<std::int64_t>(invoke("counter_value",{handle}).data())==42);}
    EXPECT(std::get<std::int64_t>(invoke("live_counters").data())==0);
    for(int i=0;i<20;++i)EXPECT(std::get<std::int64_t>(invoke("add",{s::Value(std::int64_t{i}),s::Value(std::int64_t{1})}).data())==i+1);
  }
#endif

  if(failures){std::cerr<<failures<<" test(s) failed\n";return 1;}std::cout<<"All S 0.2 tests passed.\n";return 0;
}
