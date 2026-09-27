#pragma once

#include "s/checker.hpp"
#include "s/value.hpp"
#include "s/advanced.hpp"
#include "s/database.hpp"
#include "s/ecosystem.hpp"
#include "s/game_ext.hpp"
#include "s/expansion.hpp"
#include "s/formats.hpp"
#include <memory>
#include <string>

namespace s {
class Interpreter;

bool is_platform_builtin(const std::string& name);
TypeInfo platform_builtin_type(const std::string& name);
std::shared_ptr<ModuleData> platform_builtin_module(const std::string& name, Interpreter& vm);

inline bool combined_platform_builtin(const std::string& name){
  return is_format_builtin(name)||is_expansion_builtin(name)||is_ecosystem_builtin(name)||is_platform_builtin(name)||is_advanced_builtin(name);
}

inline TypeInfo combined_platform_builtin_type(const std::string& name){
  if(is_format_builtin(name)) return format_builtin_type(name);
  if(is_expansion_builtin(name)) return expansion_builtin_type(name);
  if(is_ecosystem_builtin(name)){
    auto module=ecosystem_builtin_type(name);
    if(name=="game") extend_game_type(module);
    return module;
  }
  if(is_advanced_builtin(name)){
    auto module=advanced_builtin_type(name);
    if(name=="db") extend_database_type(module);
    return module;
  }
  auto module=platform_builtin_type(name);
  if(name=="collections") extend_collections_type(module);
  return module;
}

inline std::shared_ptr<ModuleData> combined_platform_builtin_module(const std::string& name,Interpreter& vm){
  if(is_format_builtin(name)) return format_builtin_module(name,vm);
  if(is_expansion_builtin(name)) return expansion_builtin_module(name,vm);
  if(is_ecosystem_builtin(name)){
    auto module=ecosystem_builtin_module(name,vm);
    if(name=="game") extend_game_module(module,vm);
    return module;
  }
  if(is_advanced_builtin(name)){
    auto module=advanced_builtin_module(name,vm);
    if(name=="db") extend_database_module(module,vm);
    return module;
  }
  auto module=platform_builtin_module(name,vm);
  if(name=="collections") extend_collections_module(module,vm);
  return module;
}

} // namespace s
