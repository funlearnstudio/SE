#include "s/parser.hpp"
#include "s/error.hpp"
#include <algorithm>
#include <cctype>

namespace s {

const Token& Parser::peek(int o) const {
  auto index=static_cast<long long>(at_)+static_cast<long long>(o);
  if(index<0)index=0;
  auto last=static_cast<long long>(tokens_.size()-1);
  if(index>last)index=last;
  return tokens_[static_cast<std::size_t>(index)];
}
bool Parser::check(TokenKind k) const { return peek().kind==k; }
bool Parser::match(TokenKind k) { if(!check(k)) return false; ++at_; return true; }
const Token& Parser::take(TokenKind k,const std::string& message) { if(!check(k)) throw Error(peek().pos,message); return tokens_[at_++]; }
bool Parser::identifier(const std::string& name) const { return check(TokenKind::Identifier)&&peek().text==name; }
void Parser::line_end(){ if(match(TokenKind::Newline)) return; if(check(TokenKind::Dedent)||check(TokenKind::End)) return; throw Error(peek().pos,"I expected the line to end here."); }

ast::Program Parser::parse(){ ast::Program p; while(!check(TokenKind::End)){ if(match(TokenKind::Newline)) continue; p.statements.push_back(statement()); } return p; }

ast::Block Parser::block(){
  take(TokenKind::Newline,"Start the block on the next line.");
  take(TokenKind::Indent,"This line needs to be indented by 4 spaces.");
  ast::Block result;
  while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){ if(match(TokenKind::Newline)) continue; result.push_back(statement()); }
  take(TokenKind::Dedent,"This block was not closed correctly.");
  return result;
}

ast::TypeRef Parser::type_ref(){
  auto name=take(TokenKind::Identifier,"Write a type name here.");
  std::vector<ast::TypeRef> args;
  if(match(TokenKind::LeftBracket)){
    if(check(TokenKind::RightBracket)) throw Error(peek().pos,"A generic type needs at least one type argument.");
    do { args.push_back(type_ref()); } while(match(TokenKind::Comma));
    take(TokenKind::RightBracket,"Close type arguments with ']'.");
  }
  return ast::TypeRef{name.text,std::move(args)};
}

std::shared_ptr<ast::Function> Parser::function(SourcePos start){
  auto name=take(TokenKind::Identifier,"Give this function a name.");
  std::vector<std::string> generics;
  if(match(TokenKind::LeftBracket)){
    if(check(TokenKind::RightBracket)) throw Error(peek().pos,"A generic function needs at least one type name.");
    do { generics.push_back(take(TokenKind::Identifier,"Write a generic type name.").text); } while(match(TokenKind::Comma));
    take(TokenKind::RightBracket,"Close generic type names with ']'.");
  }
  std::vector<std::string> params;
  std::vector<ast::TypeRef> param_types;
  while(check(TokenKind::Identifier)){
    params.push_back(tokens_[at_++].text);
    ast::TypeRef type;
    if(match(TokenKind::Colon)) type=type_ref();
    param_types.push_back(std::move(type));
  }
  ast::TypeRef result_type;
  if(match(TokenKind::Arrow)) result_type=type_ref();
  auto b=block();

  // Once a normal make block contains a Web section, preserve the shorter
  // component form `text value` by lowering it into the same Web AST. This is
  // contextual, so ordinary SE functions keep their existing call semantics.
  bool web=false;
  for(const auto& item:b)if(std::dynamic_pointer_cast<ast::WebSection>(item)){web=true;break;}
  if(web){
    ast::Block lowered;
    for(const auto& item:b){
      auto statement=std::dynamic_pointer_cast<ast::ExprStmt>(item);
      auto call=statement?std::dynamic_pointer_cast<ast::Call>(statement->value):nullptr;
      auto callee=call?std::dynamic_pointer_cast<ast::Variable>(call->callee):nullptr;
      if(callee&&callee->name=="text"&&call->args.size()==1){
        auto section=std::make_shared<ast::WebSection>(item->pos,"html");
        section->elements.push_back({item->pos,"text",{call->args.front()}, {}});
        lowered.push_back(section);
      }else lowered.push_back(item);
    }
    b=std::move(lowered);
  }
  return std::make_shared<ast::Function>(start,name.text,std::move(params),std::move(b),std::move(generics),std::move(param_types),std::move(result_type));
}

std::shared_ptr<ast::Type> Parser::type_decl(SourcePos start){
  auto name=take(TokenKind::Identifier,"Give this type a name.");
  std::vector<std::string> generics;
  if(match(TokenKind::LeftBracket)){
    if(check(TokenKind::RightBracket)) throw Error(peek().pos,"A generic type needs at least one type name.");
    do { generics.push_back(take(TokenKind::Identifier,"Write a generic type name.").text); } while(match(TokenKind::Comma));
    take(TokenKind::RightBracket,"Close generic type names with ']'.");
  }
  take(TokenKind::Newline,"Start the type on the next line.");
  take(TokenKind::Indent,"Indent the fields and methods inside this type.");
  std::vector<ast::FieldDecl> fields; std::vector<std::shared_ptr<ast::Function>> methods;
  while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){
    if(match(TokenKind::Newline)) continue;
    Token item=peek();
    if(match(TokenKind::Make)){ methods.push_back(function(item.pos)); continue; }
    auto field=take(TokenKind::Identifier,"Inside a type, write a field or a make method.");
    ast::TypeRef annotation;
    if(match(TokenKind::Colon)) annotation=type_ref();
    bool has_default=false;
    ast::ExprPtr value;
    if(match(TokenKind::Equal)){ value=expression(); has_default=true; }
    else if(annotation.empty()) throw Error(peek().pos,"Give this field a default value with '=', or write its type with ':'.");
    else value=std::make_shared<ast::Literal>(field.pos,std::string{});
    line_end();
    fields.push_back({field.pos,field.text,std::move(value),std::move(annotation),has_default});
  }
  take(TokenKind::Dedent,"This type was not closed correctly.");
  return std::make_shared<ast::Type>(start,name.text,std::move(fields),std::move(methods),std::move(generics));
}

std::string Parser::selector_token(const Token& token) const {
  switch(token.kind){
    case TokenKind::Identifier:return token.text;
    case TokenKind::Colon:return ":";
    case TokenKind::Dot:return ".";
    case TokenKind::Greater:return ">";
    case TokenKind::Star:return "*";
    case TokenKind::LeftBracket:return "[";
    case TokenKind::RightBracket:return "]";
    case TokenKind::Equal:return "=";
    case TokenKind::String:return "\""+token.text+"\"";
    default:return token.text;
  }
}

ast::WebElement Parser::web_element(){
  Token tag=peek();
  if(!check(TokenKind::Identifier))throw Error(tag.pos,"Write an HTML element name here, like div, h1, p, or button.");
  ++at_;
  ast::WebElement item{tag.pos,tag.text,{},{}};
  if(!check(TokenKind::Newline)&&!check(TokenKind::Dedent)&&!check(TokenKind::End))item.values.push_back(expression());
  if(check(TokenKind::Newline)&&peek(1).kind==TokenKind::Indent){
    ++at_;++at_;
    while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){if(match(TokenKind::Newline))continue;item.children.push_back(web_element());}
    take(TokenKind::Dedent,"This HTML element block was not closed correctly.");
  }else line_end();
  return item;
}

std::vector<ast::WebElement> Parser::web_elements(){
  take(TokenKind::Newline,"Start HTML on the next line.");
  take(TokenKind::Indent,"Indent HTML elements under html.");
  std::vector<ast::WebElement> items;
  while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){if(match(TokenKind::Newline))continue;items.push_back(web_element());}
  take(TokenKind::Dedent,"This html block was not closed correctly.");
  return items;
}

ast::WebCssItem Parser::web_css_item(){
  Token first=peek();
  if(!check(TokenKind::Identifier)&&!check(TokenKind::Dot))throw Error(first.pos,"Write a CSS property or selector here.");
  std::size_t end=at_;
  while(end<tokens_.size()&&tokens_[end].kind!=TokenKind::Newline&&tokens_[end].kind!=TokenKind::Dedent&&tokens_[end].kind!=TokenKind::End)++end;
  bool nested=end<tokens_.size()&&tokens_[end].kind==TokenKind::Newline&&end+1<tokens_.size()&&tokens_[end+1].kind==TokenKind::Indent;
  if(nested){
    std::string selector;
    while(at_<end)selector+=selector_token(tokens_[at_++]);
    if(selector.empty())throw Error(first.pos,"Write a CSS selector before the indented block.");
    ++at_;++at_;
    ast::WebCssItem item{first.pos,std::move(selector),{}, {}};
    while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){if(match(TokenKind::Newline))continue;item.children.push_back(web_css_item());}
    take(TokenKind::Dedent,"This CSS selector block was not closed correctly.");
    return item;
  }
  auto name=take(TokenKind::Identifier,"Write a CSS property name here.");
  ast::WebCssItem item{name.pos,name.text,{}, {}};
  if(check(TokenKind::Newline)||check(TokenKind::Dedent)||check(TokenKind::End))throw Error(name.pos,"CSS property '"+name.text+"' needs a value.");
  item.values.push_back(expression());
  line_end();
  return item;
}

std::vector<ast::WebCssItem> Parser::web_css_items(){
  take(TokenKind::Newline,"Start CSS on the next line.");
  take(TokenKind::Indent,"Indent CSS under css or style.");
  std::vector<ast::WebCssItem> items;
  while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){if(match(TokenKind::Newline))continue;items.push_back(web_css_item());}
  take(TokenKind::Dedent,"This CSS block was not closed correctly.");
  return items;
}

ast::StmtPtr Parser::web_section(SourcePos start,const std::string& kind){
  auto section=std::make_shared<ast::WebSection>(start,kind);
  if(kind=="html"){
    section->elements=web_elements();
    return section;
  }
  if(kind=="css"||kind=="style"){
    section->css=web_css_items();
    return section;
  }
  take(TokenKind::Newline,"Start JavaScript behavior on the next line.");
  take(TokenKind::Indent,"Indent JavaScript behavior under js.");
  while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){
    if(match(TokenKind::Newline))continue;
    Token item=peek();
    if(identifier("when")){
      ++at_;
      std::string event;
      if(check(TokenKind::Identifier)||check(TokenKind::String))event=tokens_[at_++].text;
      else throw Error(peek().pos,"Write an event after when, like when click.");
      auto body=block();
      section->events.push_back({item.pos,std::move(event),std::move(body)});
      continue;
    }
    if(identifier("native")){
      ++at_;
      section->native.push_back(expression());
      line_end();
      continue;
    }
    throw Error(item.pos,"Inside js, use 'when event' for behavior or 'native \"...\"' as an escape hatch.");
  }
  take(TokenKind::Dedent,"This js block was not closed correctly.");
  return section;
}

ast::StmtPtr Parser::if_statement(SourcePos start){
  auto condition=expression();
  auto yes=block();
  ast::Block no;
  if(match(TokenKind::Else)){
    if(match(TokenKind::If)) no.push_back(if_statement(peek(-1).pos));
    else no=block();
  }
  return std::make_shared<ast::If>(start,condition,std::move(yes),std::move(no));
}

ast::StmtPtr Parser::statement(){
  Token start=peek();
  if(check(TokenKind::Identifier)&&(peek().text=="html"||peek().text=="css"||peek().text=="js"||peek().text=="style")&&peek(1).kind==TokenKind::Newline){
    std::string kind=peek().text;++at_;return web_section(start.pos,kind);
  }
  if(identifier("page")){
    std::size_t end=at_;
    while(end<tokens_.size()&&tokens_[end].kind!=TokenKind::Newline&&tokens_[end].kind!=TokenKind::End)++end;
    if(end<tokens_.size()&&tokens_[end].kind==TokenKind::Newline&&end+1<tokens_.size()&&tokens_[end+1].kind==TokenKind::Indent){
      ++at_;
      auto route=expression();
      auto body=block();
      return std::make_shared<ast::Page>(start.pos,std::move(route),std::move(body));
    }
  }
  if(match(TokenKind::Say)){ auto e=expression(); line_end(); return std::make_shared<ast::Say>(start.pos,e); }
  if(match(TokenKind::Use)){ auto n=take(TokenKind::Identifier,"Write a module name after 'use'."); line_end(); return std::make_shared<ast::Use>(start.pos,n.text); }
  if(match(TokenKind::Type)) return type_decl(start.pos);
  if(match(TokenKind::Match)){
    auto value=expression();
    take(TokenKind::Newline,"Start match cases on the next line.");
    take(TokenKind::Indent,"Indent match cases by 4 spaces.");
    std::vector<ast::MatchCase> cases; ast::Block fallback;
    while(!check(TokenKind::Dedent)&&!check(TokenKind::End)){
      if(match(TokenKind::Newline)) continue;
      Token item=peek();
      if(match(TokenKind::Case)){
        auto pattern=expression();
        auto body=block();
        cases.push_back({item.pos,std::move(pattern),std::move(body)});
        continue;
      }
      if(match(TokenKind::Else)){
        fallback=block();
        while(match(TokenKind::Newline)){}
        if(!check(TokenKind::Dedent)) throw Error(peek().pos,"'else' must be the last branch in a match.");
        break;
      }
      throw Error(peek().pos,"Inside match, use 'case value' or 'else'.");
    }
    take(TokenKind::Dedent,"This match block was not closed correctly.");
    return std::make_shared<ast::Match>(start.pos,value,std::move(cases),std::move(fallback));
  }
  if(match(TokenKind::If)) return if_statement(start.pos);
  if(match(TokenKind::Try)){
    if(check(TokenKind::Newline)){
      auto body=block(); take(TokenKind::Else,"A try block needs 'else' to handle an error.");
      std::string name="error"; if(check(TokenKind::Identifier)) name=tokens_[at_++].text;
      auto no=block(); return std::make_shared<ast::Try>(start.pos,std::move(body),std::move(name),std::move(no));
    }
    --at_;
  }
  if(match(TokenKind::Fail)){ auto e=expression(); line_end(); return std::make_shared<ast::Fail>(start.pos,e); }
  if(match(TokenKind::Repeat)){ auto n=expression(); auto b=block(); return std::make_shared<ast::Repeat>(start.pos,n,std::move(b)); }
  if(match(TokenKind::For)){
    std::vector<std::string> names;
    names.push_back(take(TokenKind::Identifier,"Give the loop value a name after 'for'.").text);
    if(check(TokenKind::Identifier)) names.push_back(tokens_[at_++].text);
    take(TokenKind::In,"Use 'in' before the values to loop over."); auto values=expression(); auto b=block();
    return std::make_shared<ast::For>(start.pos,std::move(names),values,std::move(b));
  }
  if(match(TokenKind::While)){ auto c=expression(); auto b=block(); return std::make_shared<ast::While>(start.pos,c,std::move(b)); }
  if(match(TokenKind::Make)) return function(start.pos);
  if(match(TokenKind::Give)){ auto e=expression(); line_end(); return std::make_shared<ast::Give>(start.pos,e); }
  auto left=expression();
  if(match(TokenKind::Equal)){
    auto value=expression(); ast::Block init;
    if(check(TokenKind::Newline)&&peek(1).kind==TokenKind::Indent) init=block(); else line_end();
    return std::make_shared<ast::Assign>(start.pos,left,value,std::move(init));
  }
  TokenKind binary=TokenKind::End;
  if(match(TokenKind::PlusEqual)) binary=TokenKind::Plus;
  else if(match(TokenKind::MinusEqual)) binary=TokenKind::Minus;
  else if(match(TokenKind::StarEqual)) binary=TokenKind::Star;
  else if(match(TokenKind::SlashEqual)) binary=TokenKind::Slash;
  else if(match(TokenKind::PercentEqual)) binary=TokenKind::Percent;
  if(binary!=TokenKind::End){
    auto value=expression(); line_end();
    auto combined=std::make_shared<ast::Binary>(start.pos,left,binary,value);
    return std::make_shared<ast::Assign>(start.pos,left,std::move(combined));
  }
  line_end(); return std::make_shared<ast::ExprStmt>(start.pos,left);
}

int Parser::precedence(TokenKind k) const {
  switch(k){
    case TokenKind::Or:return 1; case TokenKind::And:return 2;
    case TokenKind::EqualEqual:case TokenKind::BangEqual:return 3;
    case TokenKind::Greater:case TokenKind::Less:case TokenKind::GreaterEqual:case TokenKind::LessEqual:case TokenKind::In:return 4;
    case TokenKind::Range:return 5; case TokenKind::Plus:case TokenKind::Minus:return 6;
    case TokenKind::Star:case TokenKind::Slash:case TokenKind::Percent:return 7;
    case TokenKind::Power:return 8; default:return -1;
  }
}
bool Parser::expression_start(TokenKind k) const {
  return k==TokenKind::Identifier||k==TokenKind::Integer||k==TokenKind::Number||k==TokenKind::String||k==TokenKind::Duration||
         k==TokenKind::True||k==TokenKind::False||k==TokenKind::LeftParen||k==TokenKind::LeftBracket||k==TokenKind::Ask||
         k==TokenKind::Set||k==TokenKind::Map||k==TokenKind::Try;
}

ast::ExprPtr Parser::expression(int min_prec){
  auto left=postfix(prefix());
  while(true){ int p=precedence(peek().kind); if(p<min_prec) break; Token op=tokens_[at_++]; auto right=expression(p+(op.kind==TokenKind::Power?0:1));
    if(op.kind==TokenKind::Range) left=std::make_shared<ast::Range>(op.pos,left,right); else left=std::make_shared<ast::Binary>(op.pos,left,op.kind,right);
  }
  return left;
}

std::vector<ast::ExprPtr> Parser::bracket_items(TokenKind close){
  std::vector<ast::ExprPtr> items;
  while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
  if(!check(close)){
    do {
      while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
      items.push_back(expression());
      while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
    } while(match(TokenKind::Comma));
  }
  while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
  take(close,"Close this collection with ']'."); return items;
}

ast::ExprPtr Parser::prefix(){
  Token t=peek();
  if(match(TokenKind::Integer)) return std::make_shared<ast::Literal>(t.pos,static_cast<std::int64_t>(std::stoll(t.text)));
  if(match(TokenKind::Number)) return std::make_shared<ast::Literal>(t.pos,std::stod(t.text));
  if(match(TokenKind::String)) return std::make_shared<ast::Literal>(t.pos,t.text);
  if(match(TokenKind::Duration)){
    std::size_t cut=0; while(cut<t.text.size()&&std::isdigit(static_cast<unsigned char>(t.text[cut])))++cut;
    auto n=std::stoll(t.text.substr(0,cut)); auto unit=t.text.substr(cut);
    if(unit=="s") n*=1000; else if(unit=="min") n*=60000;
    return std::make_shared<ast::Duration>(t.pos,n);
  }
  if(match(TokenKind::True)||match(TokenKind::False)) return std::make_shared<ast::Literal>(t.pos,t.kind==TokenKind::True);
  if(match(TokenKind::Identifier)) return std::make_shared<ast::Variable>(t.pos,t.text);
  if(match(TokenKind::Minus)||match(TokenKind::Not)) return std::make_shared<ast::Unary>(t.pos,t.kind,expression(8));
  if(match(TokenKind::Ask)) return std::make_shared<ast::Ask>(t.pos,expression(9));
  if(match(TokenKind::Try)) return std::make_shared<ast::TryExpr>(t.pos,expression(9));
  if(match(TokenKind::LeftParen)){ auto e=expression(); take(TokenKind::RightParen,"Close this expression with ')'."); return e; }
  if(match(TokenKind::Set)){
    take(TokenKind::LeftBracket,"Write set values inside '[' and ']'."); auto items=bracket_items(TokenKind::RightBracket); return std::make_shared<ast::Set>(t.pos,std::move(items));
  }
  if(match(TokenKind::Map)){
    take(TokenKind::LeftBracket,"Write map entries inside '[' and ']'."); std::vector<std::pair<ast::ExprPtr,ast::ExprPtr>> items;
    while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
    if(!check(TokenKind::RightBracket)){
      do {
        while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
        auto k=expression(); take(TokenKind::Colon,"Separate a map key and value with ':'."); auto v=expression(); items.emplace_back(k,v);
        while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
      } while(match(TokenKind::Comma));
    }
    while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
    take(TokenKind::RightBracket,"Close this map with ']'."); return std::make_shared<ast::Map>(t.pos,std::move(items));
  }
  if(match(TokenKind::LeftBracket)){
    while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
    if(check(TokenKind::RightBracket)){++at_;return std::make_shared<ast::List>(t.pos,std::vector<ast::ExprPtr>{});}
    auto first=expression();
    if(match(TokenKind::Colon)){
      std::vector<std::pair<ast::ExprPtr,ast::ExprPtr>> items;items.emplace_back(first,expression());
      while(true){
        while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
        if(!match(TokenKind::Comma))break;
        while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
        auto k=expression();take(TokenKind::Colon,"Separate a map key and value with ':'.");items.emplace_back(k,expression());
      }
      while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
      take(TokenKind::RightBracket,"Close this map with ']'.");return std::make_shared<ast::Map>(t.pos,std::move(items));
    }
    std::vector<ast::ExprPtr> items{first};
    while(true){
      while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
      if(!match(TokenKind::Comma))break;
      while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
      items.push_back(expression());
    }
    while(match(TokenKind::Newline)||match(TokenKind::Indent)||match(TokenKind::Dedent)){}
    take(TokenKind::RightBracket,"Close this list with ']'."); return std::make_shared<ast::List>(t.pos,std::move(items));
  }
  throw Error(t.pos,"I expected a value here.");
}

ast::ExprPtr Parser::postfix(ast::ExprPtr value){
  auto take_member_name=[&](){
    if(check(TokenKind::Identifier)||check(TokenKind::Map)) return tokens_[at_++];
    throw Error(peek().pos,"Write a member name after '.'.");
  };
  auto attach_members=[&](ast::ExprPtr arg){
    while(true){
      if(auto variable=std::dynamic_pointer_cast<ast::Variable>(arg);variable&&variable->type_args.empty()&&!variable->name.empty()&&std::isupper(static_cast<unsigned char>(variable->name.front()))&&match(TokenKind::LeftBracket)){
        if(check(TokenKind::RightBracket)) throw Error(peek().pos,"A generic type needs at least one type argument.");
        do { variable->type_args.push_back(type_ref()); } while(match(TokenKind::Comma));
        take(TokenKind::RightBracket,"Close type arguments with ']'.");
        continue;
      }
      // A following '[' after a literal argument is another low-punctuation
      // function argument, not an index. Indexing remains available for names
      // and member/index expressions, which covers SE's ordinary collection use.
      if(check(TokenKind::LeftBracket)&&(std::dynamic_pointer_cast<ast::Variable>(arg)||std::dynamic_pointer_cast<ast::Member>(arg)||std::dynamic_pointer_cast<ast::Index>(arg))){
        ++at_;auto i=expression();take(TokenKind::RightBracket,"Close this index with ']'.");arg=std::make_shared<ast::Index>(arg->pos,arg,i);continue;
      }
      if(match(TokenKind::Dot)){auto n=take_member_name();arg=std::make_shared<ast::Member>(arg->pos,arg,n.text);continue;}
      break;
    }
    return arg;
  };

  value=attach_members(value);
  if((std::dynamic_pointer_cast<ast::Variable>(value)||std::dynamic_pointer_cast<ast::Member>(value)) && expression_start(peek().kind)){
    // Bare calls deliberately allow arithmetic inside each argument:
    //     func a-1 + func a-2
    // means:
    //     func(a - 1) + func(a - 2)
    //
    // Whitespace is not significant, so `a-1` and `a - 1` behave the same.
    // An additive operator starts the outer expression when its right side
    // clearly begins another bare call (identifier followed by a value).
    auto starts_bare_call=[&](int offset){
      return peek(offset).kind==TokenKind::Identifier&&expression_start(peek(offset+1).kind);
    };
    auto bare_argument=[&](){
      auto arg=attach_members(prefix());
      while(true){
        int p=precedence(peek().kind);
        if(p<6)break; // Keep comparisons/ranges/logical operators outside the call.
        if((peek().kind==TokenKind::Plus||peek().kind==TokenKind::Minus)&&starts_bare_call(1))break;
        Token op=tokens_[at_++];
        auto right=expression(p+(op.kind==TokenKind::Power?0:1));
        arg=std::make_shared<ast::Binary>(op.pos,arg,op.kind,right);
      }
      return arg;
    };
    std::vector<ast::ExprPtr> args;
    while(expression_start(peek().kind)) args.push_back(bare_argument());
    value=std::make_shared<ast::Call>(value->pos,value,std::move(args));
  }
  return value;
}

} // namespace s