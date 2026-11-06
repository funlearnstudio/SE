#pragma once

#include "s/token.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace s::ast {

struct Expr { explicit Expr(SourcePos p) : pos(p) {} virtual ~Expr()=default; SourcePos pos; };
using ExprPtr = std::shared_ptr<Expr>;
struct Stmt { explicit Stmt(SourcePos p) : pos(p) {} virtual ~Stmt()=default; SourcePos pos; };
using StmtPtr = std::shared_ptr<Stmt>;
using Block = std::vector<StmtPtr>;

struct TypeRef {
  std::string name;
  std::vector<TypeRef> args;
  TypeRef()=default;
  explicit TypeRef(std::string n):name(std::move(n)){}
  TypeRef(std::string n,std::vector<TypeRef> a):name(std::move(n)),args(std::move(a)){}
  bool empty() const { return name.empty(); }
};

struct Literal final : Expr { using Data=std::variant<std::int64_t,double,std::string,bool>; Literal(SourcePos p,Data v):Expr(p),value(std::move(v)){} Data value; };
struct Duration final : Expr { Duration(SourcePos p,std::int64_t ms):Expr(p),milliseconds(ms){} std::int64_t milliseconds; };
struct Variable final : Expr {
  Variable(SourcePos p,std::string n,std::vector<TypeRef> ta={}):Expr(p),name(std::move(n)),type_args(std::move(ta)){}
  std::string name;
  std::vector<TypeRef> type_args;
};
struct Binary final : Expr { Binary(SourcePos p,ExprPtr l,TokenKind o,ExprPtr r):Expr(p),left(std::move(l)),op(o),right(std::move(r)){} ExprPtr left; TokenKind op; ExprPtr right; };
struct Unary final : Expr { Unary(SourcePos p,TokenKind o,ExprPtr v):Expr(p),op(o),value(std::move(v)){} TokenKind op; ExprPtr value; };
struct List final : Expr { List(SourcePos p,std::vector<ExprPtr> v):Expr(p),items(std::move(v)){} std::vector<ExprPtr> items; };
// `a[n] = []`, `a[n]:Text = []`, and `a[rows][cols] = []`.
struct SizedList final : Expr {
  SizedList(SourcePos p,ExprPtr n,std::vector<ExprPtr> more={},std::string t="Int"):
    Expr(p),count(std::move(n)),other_counts(std::move(more)),element_type(std::move(t)){}
  ExprPtr count;
  std::vector<ExprPtr> other_counts;
  std::string element_type;
};
struct Map final : Expr { Map(SourcePos p,std::vector<std::pair<ExprPtr,ExprPtr>> v):Expr(p),items(std::move(v)){} std::vector<std::pair<ExprPtr,ExprPtr>> items; };
struct Set final : Expr { Set(SourcePos p,std::vector<ExprPtr> v):Expr(p),items(std::move(v)){} std::vector<ExprPtr> items; };
struct Index final : Expr { Index(SourcePos p,ExprPtr v,ExprPtr i):Expr(p),value(std::move(v)),index(std::move(i)){} ExprPtr value,index; };
struct Member final : Expr { Member(SourcePos p,ExprPtr v,std::string n):Expr(p),value(std::move(v)),name(std::move(n)){} ExprPtr value; std::string name; };
struct Call final : Expr { Call(SourcePos p,ExprPtr c,std::vector<ExprPtr> a):Expr(p),callee(std::move(c)),args(std::move(a)){} ExprPtr callee; std::vector<ExprPtr> args; };
struct Ask final : Expr { Ask(SourcePos p,ExprPtr q):Expr(p),question(std::move(q)){} ExprPtr question; };
struct Range final : Expr { Range(SourcePos p,ExprPtr a,ExprPtr b):Expr(p),start(std::move(a)),end(std::move(b)){} ExprPtr start,end; };
struct TryExpr final : Expr { TryExpr(SourcePos p,ExprPtr v):Expr(p),value(std::move(v)){} ExprPtr value; };

struct ExprStmt final : Stmt { ExprStmt(SourcePos p,ExprPtr v):Stmt(p),value(std::move(v)){} ExprPtr value; };
struct Read final : Stmt {
  Read(SourcePos p,std::string n,std::string t,ExprPtr len={}):Stmt(p),name(std::move(n)),type(std::move(t)),count(std::move(len)){}
  std::string name,type;
  ExprPtr count;
};
struct MultiAsk final : Stmt {
  MultiAsk(SourcePos p,std::vector<std::string> n,std::vector<std::string> t):Stmt(p),names(std::move(n)),types(std::move(t)){}
  std::vector<std::string> names,types;
};
struct Assign final : Stmt { Assign(SourcePos p,ExprPtr t,ExprPtr v,Block i={}):Stmt(p),target(std::move(t)),value(std::move(v)),init(std::move(i)){} ExprPtr target,value; Block init; };
struct Say final : Stmt { Say(SourcePos p,ExprPtr v):Stmt(p),value(std::move(v)){} ExprPtr value; };
struct If final : Stmt { If(SourcePos p,ExprPtr c,Block t,Block e):Stmt(p),condition(std::move(c)),then_block(std::move(t)),else_block(std::move(e)){} ExprPtr condition; Block then_block,else_block; };
struct Repeat final : Stmt { Repeat(SourcePos p,ExprPtr c,Block b):Stmt(p),count(std::move(c)),body(std::move(b)){} ExprPtr count; Block body; };
struct For final : Stmt { For(SourcePos p,std::vector<std::string> n,ExprPtr v,Block b):Stmt(p),names(std::move(n)),values(std::move(v)),body(std::move(b)){} std::vector<std::string> names; ExprPtr values; Block body; };
struct While final : Stmt { While(SourcePos p,ExprPtr c,Block b):Stmt(p),condition(std::move(c)),body(std::move(b)){} ExprPtr condition; Block body; };
struct Function final : Stmt {
  Function(SourcePos p,std::string n,std::vector<std::string> a,Block b,std::vector<std::string> g={},std::vector<TypeRef> pt={},TypeRef rt={})
      :Stmt(p),name(std::move(n)),params(std::move(a)),body(std::move(b)),generic_params(std::move(g)),param_types(std::move(pt)),result_type(std::move(rt)){}
  std::string name;
  std::vector<std::string> params;
  Block body;
  std::vector<std::string> generic_params;
  std::vector<TypeRef> param_types;
  TypeRef result_type;
};
struct Give final : Stmt { Give(SourcePos p,ExprPtr v):Stmt(p),value(std::move(v)){} ExprPtr value; };
struct FieldDecl {
  SourcePos pos;
  std::string name;
  ExprPtr value;
  TypeRef type;
  bool has_default=true;
};
struct Type final : Stmt {
  Type(SourcePos p,std::string n,std::vector<FieldDecl> f,std::vector<std::shared_ptr<Function>> m,std::vector<std::string> g={})
      :Stmt(p),name(std::move(n)),fields(std::move(f)),methods(std::move(m)),generic_params(std::move(g)){}
  std::string name;
  std::vector<FieldDecl> fields;
  std::vector<std::shared_ptr<Function>> methods;
  std::vector<std::string> generic_params;
};
struct Use final : Stmt { Use(SourcePos p,std::string n):Stmt(p),name(std::move(n)){} std::string name; };
struct Try final : Stmt { Try(SourcePos p,Block b,std::string n,Block e):Stmt(p),body(std::move(b)),error_name(std::move(n)),else_block(std::move(e)){} Block body; std::string error_name; Block else_block; };
struct Fail final : Stmt { Fail(SourcePos p,ExprPtr v):Stmt(p),value(std::move(v)){} ExprPtr value; };
struct MatchCase { SourcePos pos; ExprPtr pattern; Block body; };
struct Match final : Stmt {
  Match(SourcePos p,ExprPtr v,std::vector<MatchCase> c,Block e):Stmt(p),value(std::move(v)),cases(std::move(c)),else_block(std::move(e)){}
  ExprPtr value;
  std::vector<MatchCase> cases;
  Block else_block;
};

// Web syntax is contextual rather than a new family of lexer keywords. These
// nodes keep HTML/CSS/JS related data associated with the same `make`
// component while leaving normal SE functions and statements unchanged.
struct WebElement {
  SourcePos pos;
  std::string tag;
  std::vector<ExprPtr> values;
  std::vector<WebElement> children;
};
struct WebCssItem {
  SourcePos pos;
  std::string name;
  std::vector<ExprPtr> values;
  std::vector<WebCssItem> children;
};
struct WebEvent {
  SourcePos pos;
  std::string event;
  Block body;
};
struct WebSection final : Stmt {
  WebSection(SourcePos p,std::string k):Stmt(p),kind(std::move(k)){}
  std::string kind; // html, css, style, js
  std::vector<WebElement> elements;
  std::vector<WebCssItem> css;
  std::vector<WebEvent> events;
  std::vector<ExprPtr> native;
};
struct Page final : Stmt {
  Page(SourcePos p,ExprPtr r,Block b):Stmt(p),route(std::move(r)),body(std::move(b)){}
  ExprPtr route;
  Block body;
};

struct NativeFunction {
  std::string name;
  std::string symbol;
  std::vector<std::string> args;
  std::string result;
  std::string cleanup;
  bool fallible=false;
};
struct Module {
  std::string name;
  std::string path;
  Block statements;
  std::vector<std::string> imports;
  bool builtin=false;
  bool native=false;
  std::string native_library;
  std::vector<NativeFunction> native_functions;
};
struct Program { Block statements; std::vector<Module> modules; std::string entry; };

} // namespace s::ast