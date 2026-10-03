#include "s/lexer.hpp"
#include "s/error.hpp"
#include <cctype>
#include <unordered_map>

namespace s {

std::vector<Token> Lexer::scan() {
  std::vector<Token> out;
  std::vector<int> indents{0};
  const std::unordered_map<std::string, TokenKind> words{
    {"true",TokenKind::True},{"false",TokenKind::False},{"say",TokenKind::Say},
    {"ask",TokenKind::Ask},{"if",TokenKind::If},{"else",TokenKind::Else},
    {"repeat",TokenKind::Repeat},{"for",TokenKind::For},{"in",TokenKind::In},
    {"while",TokenKind::While},{"make",TokenKind::Make},{"give",TokenKind::Give},
    {"type",TokenKind::Type},{"use",TokenKind::Use},{"try",TokenKind::Try},
    {"fail",TokenKind::Fail},{"set",TokenKind::Set},{"map",TokenKind::Map},
    {"match",TokenKind::Match},{"case",TokenKind::Case},
    {"and",TokenKind::And},{"or",TokenKind::Or},{"not",TokenKind::Not}
  };

  std::size_t i = 0;
  int line = 1;
  bool line_start = true;
  int bracket_depth = 0;
  auto add = [&](TokenKind kind, std::string text, int column) {
    out.push_back({kind, std::move(text), {line, column}});
  };

  while (i < source_.size()) {
    if (line_start && bracket_depth == 0) {
      int spaces = 0;
      std::size_t p = i;
      while (p < source_.size() && (source_[p] == ' ' || source_[p] == '\t')) {
        spaces += source_[p] == '\t' ? 4 : 1;
        ++p;
      }
      if (p < source_.size() && source_[p] != '\n' && source_[p] != '#') {
        if (spaces % 4 != 0)
          throw Error({line, 1}, "Indentation must use groups of 4 spaces.", "Add or remove spaces so this line lines up with its block.");
        if (spaces > indents.back()) {
          indents.push_back(spaces); add(TokenKind::Indent, "", 1);
        } else {
          while (spaces < indents.back()) { indents.pop_back(); add(TokenKind::Dedent, "", 1); }
          if (spaces != indents.back()) throw Error({line, 1}, "This indentation does not match an earlier block.");
        }
      }
      i = p;
      line_start = false;
      if (i >= source_.size()) break;
    }

    char c = source_[i];
    int col = 1;
    for (std::size_t p=i; p>0 && source_[p-1]!='\n'; --p) ++col;
    if (c == ' ' || c == '\t' || c == '\r') { ++i; continue; }
    if (c == '#') { while (i < source_.size() && source_[i] != '\n') ++i; continue; }
    if (c == '\n') {
      if (bracket_depth == 0 && (out.empty() || out.back().kind != TokenKind::Newline)) add(TokenKind::Newline, "", col);
      ++i; ++line; line_start = true; continue;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      std::size_t start=i++;
      while (i<source_.size() && (std::isalnum(static_cast<unsigned char>(source_[i])) || source_[i]=='_')) ++i;
      auto text=source_.substr(start,i-start); auto it=words.find(text);
      auto after_dot=!out.empty()&&out.back().kind==TokenKind::Dot;
      // `elif condition` is syntax sugar for `else if condition`. Lower it at
      // the lexer boundary so every existing parser/checker/backend path keeps
      // exactly the same nested If AST and behavior.
      if(!after_dot&&text=="elif"){
        add(TokenKind::Else,"else",col);
        add(TokenKind::If,"if",col);
        continue;
      }
      add(after_dot?TokenKind::Identifier:(it==words.end()?TokenKind::Identifier:it->second),text,col); continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::size_t start=i++; bool dot=false;
      while (i<source_.size()) {
        if (std::isdigit(static_cast<unsigned char>(source_[i]))) { ++i; continue; }
        if (source_[i]=='.' && i+1<source_.size() && source_[i+1]!='.' && std::isdigit(static_cast<unsigned char>(source_[i+1]))) { dot=true; ++i; continue; }
        break;
      }
      if(!dot){
        std::size_t unit=i;
        while(unit<source_.size()&&std::isalpha(static_cast<unsigned char>(source_[unit])))++unit;
        auto suffix=source_.substr(i,unit-i);
        if(suffix=="ms"||suffix=="s"||suffix=="min"){
          auto raw=source_.substr(start,i-start)+suffix; add(TokenKind::Duration,raw,col); i=unit; continue;
        }
      }
      add(dot?TokenKind::Number:TokenKind::Integer,source_.substr(start,i-start),col); continue;
    }
    if (c == '"') {
      ++i; std::string value;
      while (i<source_.size() && source_[i]!='"') {
        if (source_[i]=='\n') throw Error({line,col},"This text is missing its closing quote.");
        if (source_[i]=='\\' && i+1<source_.size()) { char e=source_[++i]; value += e=='n'?'\n':e=='t'?'\t':e; }
        else value += source_[i];
        ++i;
      }
      if (i>=source_.size()) throw Error({line,col},"This text is missing its closing quote.");
      ++i; add(TokenKind::String,value,col); continue;
    }
    auto two = i+1<source_.size()?source_.substr(i,2):"";
    if (two=="**"||two=="=="||two=="!="||two==">="||two=="<="||two==".."||two=="->"||two=="+="||two=="-="||two=="*="||two=="/="||two=="%=") {
      TokenKind k=two=="**"?TokenKind::Power:
        two=="=="?TokenKind::EqualEqual:two=="!="?TokenKind::BangEqual:
        two==">="?TokenKind::GreaterEqual:two=="<="?TokenKind::LessEqual:
        two==".."?TokenKind::Range:two=="->"?TokenKind::Arrow:
        two=="+="?TokenKind::PlusEqual:two=="-="?TokenKind::MinusEqual:
        two=="*="?TokenKind::StarEqual:two=="/="?TokenKind::SlashEqual:TokenKind::PercentEqual;
      add(k,two,col); i+=2; continue;
    }
    TokenKind k;
    switch(c) {
      case '+':k=TokenKind::Plus;break; case '-':k=TokenKind::Minus;break;
      case '*':k=TokenKind::Star;break; case '/':k=TokenKind::Slash;break;
      case '%':k=TokenKind::Percent;break; case '=':k=TokenKind::Equal;break;
      case '>':k=TokenKind::Greater;break; case '<':k=TokenKind::Less;break;
      case '(':k=TokenKind::LeftParen;++bracket_depth;break;
      case ')':k=TokenKind::RightParen;--bracket_depth;break;
      case '[':k=TokenKind::LeftBracket;++bracket_depth;break;
      case ']':k=TokenKind::RightBracket;--bracket_depth;break;
      case ',':k=TokenKind::Comma;break; case ':':k=TokenKind::Colon;break; case '.':k=TokenKind::Dot;break;
      default: throw Error({line,col},std::string("I don't understand the character '")+c+"'.");
    }
    if (bracket_depth<0) throw Error({line,col},"This closing bracket has no matching opening bracket.");
    add(k,std::string(1,c),col); ++i;
  }
  if (bracket_depth != 0) throw Error({line,1},"An opening bracket was not closed.");
  if (out.empty() || out.back().kind != TokenKind::Newline) add(TokenKind::Newline,"",1);
  while (indents.size()>1) { indents.pop_back(); add(TokenKind::Dedent,"",1); }
  add(TokenKind::End,"",1);
  return out;
}

} // namespace s