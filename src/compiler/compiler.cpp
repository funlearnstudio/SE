#include "s/compiler.hpp"
#include <iomanip>
#include <sstream>

namespace s {
namespace {
std::string q(const std::string& s){
  std::ostringstream out;out<<'"';
  for(unsigned char c:s){
    switch(c){
      case '"':out<<"\\\"";break;
      case '\\':out<<"\\\\";break;
      case '\n':out<<"\\n";break;
      case '\r':out<<"\\r";break;
      case '\t':out<<"\\t";break;
      default:
        if(c<32||c==127)out<<'\\'<<std::oct<<std::setw(3)<<std::setfill('0')<<static_cast<unsigned>(c)<<std::dec;
        else out<<c;
    }
  }
  return out.str()+'"';
}
std::string pos(SourcePos p){return "SourcePos{"+std::to_string(p.line)+","+std::to_string(p.column)+"}";}
std::string strings(const std::vector<std::string>&v){std::string r="std::vector<std::string>{";for(std::size_t i=0;i<v.size();++i){if(i)r+=",";r+=q(v[i]);}return r+"}";}
std::string type_ref(const ast::TypeRef&t){std::string r="ast::TypeRef{"+q(t.name)+",std::vector<ast::TypeRef>{";for(std::size_t i=0;i<t.args.size();++i){if(i)r+=",";r+=type_ref(t.args[i]);}return r+"}}";}
std::string type_refs(const std::vector<ast::TypeRef>&v){std::string r="std::vector<ast::TypeRef>{";for(std::size_t i=0;i<v.size();++i){if(i)r+=",";r+=type_ref(v[i]);}return r+"}";}
}

std::string CppCompiler::expr(const ast::ExprPtr&e) const{
  if(auto x=std::dynamic_pointer_cast<ast::Literal>(e))return std::visit([&](auto v)->std::string{using T=decltype(v);if constexpr(std::is_same_v<T,std::string>)return "std::make_shared<ast::Literal>("+pos(x->pos)+",ast::Literal::Data{"+q(v)+"})";else if constexpr(std::is_same_v<T,bool>)return "std::make_shared<ast::Literal>("+pos(x->pos)+",ast::Literal::Data{"+std::string(v?"true":"false")+"})";else if constexpr(std::is_same_v<T,std::int64_t>)return "std::make_shared<ast::Literal>("+pos(x->pos)+",ast::Literal::Data{std::int64_t{"+std::to_string(v)+"}})";else return "std::make_shared<ast::Literal>("+pos(x->pos)+",ast::Literal::Data{double{"+std::to_string(v)+"}})";},x->value);
  if(auto x=std::dynamic_pointer_cast<ast::Duration>(e))return "std::make_shared<ast::Duration>("+pos(x->pos)+",std::int64_t{"+std::to_string(x->milliseconds)+"})";
  if(auto x=std::dynamic_pointer_cast<ast::Variable>(e))return "std::make_shared<ast::Variable>("+pos(x->pos)+","+q(x->name)+","+type_refs(x->type_args)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Binary>(e))return "std::make_shared<ast::Binary>("+pos(x->pos)+","+expr(x->left)+",TokenKind::"+std::string(token_name(x->op))+","+expr(x->right)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Unary>(e))return "std::make_shared<ast::Unary>("+pos(x->pos)+",TokenKind::"+std::string(token_name(x->op))+","+expr(x->value)+")";
  if(auto x=std::dynamic_pointer_cast<ast::List>(e)){std::string r="std::make_shared<ast::List>("+pos(x->pos)+",std::vector<ast::ExprPtr>{";for(std::size_t i=0;i<x->items.size();++i){if(i)r+=",";r+=expr(x->items[i]);}return r+"})";}
  if(auto x=std::dynamic_pointer_cast<ast::Set>(e)){std::string r="std::make_shared<ast::Set>("+pos(x->pos)+",std::vector<ast::ExprPtr>{";for(std::size_t i=0;i<x->items.size();++i){if(i)r+=",";r+=expr(x->items[i]);}return r+"})";}
  if(auto x=std::dynamic_pointer_cast<ast::Map>(e)){std::string r="std::make_shared<ast::Map>("+pos(x->pos)+",std::vector<std::pair<ast::ExprPtr,ast::ExprPtr>>{";for(std::size_t i=0;i<x->items.size();++i){if(i)r+=",";r+="{"+expr(x->items[i].first)+","+expr(x->items[i].second)+"}";}return r+"})";}
  if(auto x=std::dynamic_pointer_cast<ast::Index>(e))return "std::make_shared<ast::Index>("+pos(x->pos)+","+expr(x->value)+","+expr(x->index)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Member>(e))return "std::make_shared<ast::Member>("+pos(x->pos)+","+expr(x->value)+","+q(x->name)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Call>(e)){std::string r="std::make_shared<ast::Call>("+pos(x->pos)+","+expr(x->callee)+",std::vector<ast::ExprPtr>{";for(std::size_t i=0;i<x->args.size();++i){if(i)r+=",";r+=expr(x->args[i]);}return r+"})";}
  if(auto x=std::dynamic_pointer_cast<ast::Ask>(e))return "std::make_shared<ast::Ask>("+pos(x->pos)+","+expr(x->question)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Range>(e))return "std::make_shared<ast::Range>("+pos(x->pos)+","+expr(x->start)+","+expr(x->end)+")";
  if(auto x=std::dynamic_pointer_cast<ast::TryExpr>(e))return "std::make_shared<ast::TryExpr>("+pos(x->pos)+","+expr(x->value)+")";
  return "ast::ExprPtr{}";
}

std::string CppCompiler::block(const ast::Block&b) const{std::string r="ast::Block{";for(std::size_t i=0;i<b.size();++i){if(i)r+=",";r+=stmt(b[i]);}return r+"}";}
std::string CppCompiler::function(const std::shared_ptr<ast::Function>&x) const{return "std::make_shared<ast::Function>("+pos(x->pos)+","+q(x->name)+","+strings(x->params)+","+block(x->body)+","+strings(x->generic_params)+","+type_refs(x->param_types)+","+type_ref(x->result_type)+")";}

std::string CppCompiler::stmt(const ast::StmtPtr&s) const{
  if(auto x=std::dynamic_pointer_cast<ast::ExprStmt>(s))return "std::make_shared<ast::ExprStmt>("+pos(x->pos)+","+expr(x->value)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Assign>(s))return "std::make_shared<ast::Assign>("+pos(x->pos)+","+expr(x->target)+","+expr(x->value)+","+block(x->init)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Say>(s))return "std::make_shared<ast::Say>("+pos(x->pos)+","+expr(x->value)+")";
  if(auto x=std::dynamic_pointer_cast<ast::If>(s))return "std::make_shared<ast::If>("+pos(x->pos)+","+expr(x->condition)+","+block(x->then_block)+","+block(x->else_block)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Match>(s)){
    std::string cases="std::vector<ast::MatchCase>{";
    for(std::size_t i=0;i<x->cases.size();++i){if(i)cases+=",";cases+="ast::MatchCase{"+pos(x->cases[i].pos)+","+expr(x->cases[i].pattern)+","+block(x->cases[i].body)+"}";}cases+="}";
    return "std::make_shared<ast::Match>("+pos(x->pos)+","+expr(x->value)+","+cases+","+block(x->else_block)+")";
  }
  if(auto x=std::dynamic_pointer_cast<ast::Repeat>(s))return "std::make_shared<ast::Repeat>("+pos(x->pos)+","+expr(x->count)+","+block(x->body)+")";
  if(auto x=std::dynamic_pointer_cast<ast::For>(s))return "std::make_shared<ast::For>("+pos(x->pos)+","+strings(x->names)+","+expr(x->values)+","+block(x->body)+")";
  if(auto x=std::dynamic_pointer_cast<ast::While>(s))return "std::make_shared<ast::While>("+pos(x->pos)+","+expr(x->condition)+","+block(x->body)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Function>(s))return function(x);
  if(auto x=std::dynamic_pointer_cast<ast::Give>(s))return "std::make_shared<ast::Give>("+pos(x->pos)+","+expr(x->value)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Use>(s))return "std::make_shared<ast::Use>("+pos(x->pos)+","+q(x->name)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Try>(s))return "std::make_shared<ast::Try>("+pos(x->pos)+","+block(x->body)+","+q(x->error_name)+","+block(x->else_block)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Fail>(s))return "std::make_shared<ast::Fail>("+pos(x->pos)+","+expr(x->value)+")";
  if(auto x=std::dynamic_pointer_cast<ast::Type>(s)){
    std::string fields="std::vector<ast::FieldDecl>{";for(std::size_t i=0;i<x->fields.size();++i){if(i)fields+=",";fields+="ast::FieldDecl{"+pos(x->fields[i].pos)+","+q(x->fields[i].name)+","+expr(x->fields[i].value)+","+type_ref(x->fields[i].type)+","+(x->fields[i].has_default?"true":"false")+"}";}fields+="}";
    std::string methods="std::vector<std::shared_ptr<ast::Function>>{";for(std::size_t i=0;i<x->methods.size();++i){if(i)methods+=",";methods+=function(x->methods[i]);}methods+="}";
    return "std::make_shared<ast::Type>("+pos(x->pos)+","+q(x->name)+","+fields+","+methods+","+strings(x->generic_params)+")";
  }
  return "ast::StmtPtr{}";
}

std::string CppCompiler::module(const ast::Module&m) const{
  std::string r="[](){ ast::Module m; m.name="+q(m.name)+"; m.path="+q(m.path)+"; m.statements="+block(m.statements)+"; m.imports="+strings(m.imports)+"; m.builtin="+(m.builtin?"true":"false")+"; m.native="+(m.native?"true":"false")+"; m.native_library="+q(m.native_library)+";";
  for(auto&f:m.native_functions){r+=" { ast::NativeFunction f; f.name="+q(f.name)+"; f.symbol="+q(f.symbol)+"; f.args="+strings(f.args)+"; f.result="+q(f.result)+"; f.cleanup="+q(f.cleanup)+"; f.fallible="+(f.fallible?"true":"false")+"; m.native_functions.push_back(std::move(f)); }";}return r+=" return m; }()";
}

std::string CppCompiler::generate(const ast::Program&p){
  std::string out="#include \"s/interpreter.hpp\"\n#include \"s/error.hpp\"\n#include <iostream>\nusing namespace s;\nint main(){ try { ast::Program p; p.entry="+q(p.entry)+"; p.statements="+block(p.statements)+"; p.modules=std::vector<ast::Module>{";
  for(std::size_t i=0;i<p.modules.size();++i){if(i)out+=",";out+=module(p.modules[i]);}
  out+="}; Interpreter vm(std::cin,std::cout); vm.run(p); return 0; } catch(const RuntimeFailure& e){ const auto& x=e.error(); std::cerr<<x.kind; if(!x.source.empty()) std::cerr<<\" in \"<<x.source; if(x.line) std::cerr<<\" on line \"<<x.line; std::cerr<<\": \"<<x.message<<'\\n'; return 1; } catch(const Error& e){ std::cerr<<\"S error on line \"<<e.pos().line<<\": \"<<e.what()<<'\\n'; return 1; } catch(const std::exception& e){ std::cerr<<\"Error: \"<<e.what()<<'\\n'; return 1; } }\n";return out;
}

} // namespace s
