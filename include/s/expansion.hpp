#pragma once
#include "s/checker.hpp"
#include "s/value.hpp"
#include <memory>
#include <string>
namespace s {
class Interpreter;
bool is_expansion_builtin(const std::string& name);
TypeInfo expansion_builtin_type(const std::string& name);
std::shared_ptr<ModuleData> expansion_builtin_module(const std::string& name, Interpreter& vm);
}
