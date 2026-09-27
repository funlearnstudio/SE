#include "s/ffi.hpp"
#include "s/modules.hpp"
#include "s/value.hpp"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int failures=0;
#define EXPECT(c) do { if(!(c)){ std::cerr << "FAIL " << __LINE__ << ": " #c "\n"; ++failures; } } while(0)
}

int main(){
#ifdef S_TEST_ROOT
  const auto root=std::filesystem::path(S_TEST_ROOT);
  s::ModuleLoader loader;
  auto program=loader.load(root/"examples/native/main.se");
  const s::ast::Module* metadata=nullptr;
  for(const auto& module:program.modules){
    if(module.native){metadata=&module;break;}
  }
  EXPECT(metadata!=nullptr);
  if(metadata){
    auto native=s::load_native_module(*metadata);
    auto invoke=[&](const std::string& name,std::vector<s::Value> args){
      auto exported=native->exports.at(name);
      auto callable=std::get<std::shared_ptr<s::CallableData>>(exported.data());
      return callable->call(args,{1,1});
    };

    auto input=std::make_shared<s::ByteBufferData>();
    input->bytes={std::uint8_t{0x00},std::uint8_t{0x01},std::uint8_t{0x7f},std::uint8_t{0xff}};
    EXPECT(std::get<std::int64_t>(invoke("bytes_len",{s::Value(input)}).data())==4);

    auto echoed=invoke("bytes_echo",{s::Value(input)});
    auto output=std::get<std::shared_ptr<s::ByteBufferData>>(echoed.data());
    EXPECT(output!=input);
    EXPECT(output->bytes==input->bytes);

    input->bytes[0]=std::uint8_t{0x42};
    EXPECT(output->bytes[0]==std::uint8_t{0x00});
  }
#else
  std::cerr << "S_TEST_ROOT is not defined.\n";
  return 1;
#endif
  if(failures){std::cerr<<failures<<" byte-buffer test(s) failed\n";return 1;}
  std::cout<<"Binary byte-buffer ABI tests passed.\n";
  return 0;
}
