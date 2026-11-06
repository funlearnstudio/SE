#include "s/component_web.hpp"
#include "s/error.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace s {
namespace {

using Bindings=std::unordered_map<std::string,ast::ExprPtr>;

struct Component {
  std::shared_ptr<ast::Function> function;
  std::vector<std::shared_ptr<ast::WebSection>> html;
  std::vector<std::shared_ptr<ast::WebSection>> css;
  std::vector<std::shared_ptr<ast::WebSection>> js;
  std::vector<ast::StmtPtr> simple;
};

std::string html_escape(const std::string& value){
  std::string out;
  for(char c:value){
    if(c=='&')out+="&amp;";
    else if(c=='<')out+="&lt;";
    else if(c=='>')out+="&gt;";
    else if(c=='\"')out+="&quot;";
    else out+=c;
  }
  return out;
}

std::string js_quote(const std::string& value){
  std::ostringstream out;out<<'\"';
  for(unsigned char c:value){
    switch(c){
      case '\\':out<<"\\\\";break;
      case '\"':out<<"\\\"";break;
      case '\n':out<<"\\n";break;
      case '\r':out<<"\\r";break;
      case '\t':out<<"\\t";break;
      default:
        if(c<0x20)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<static_cast<int>(c)<<std::dec;
        else out<<static_cast<char>(c);
    }
  }
  out<<'\"';return out.str();
}

ast::ExprPtr resolve(const ast::ExprPtr& value,const Bindings& bindings){
  if(auto variable=std::dynamic_pointer_cast<ast::Variable>(value)){
    auto found=bindings.find(variable->name);
    if(found!=bindings.end())return resolve(found->second,bindings);
  }
  return value;
}

std::string text_value(const ast::ExprPtr& raw,const Bindings& bindings,const std::string& what){
  auto value=resolve(raw,bindings);
  if(auto literal=std::dynamic_pointer_cast<ast::Literal>(value)){
    if(auto text=std::get_if<std::string>(&literal->value))return *text;
    if(auto integer=std::get_if<std::int64_t>(&literal->value))return std::to_string(*integer);
    if(auto number=std::get_if<double>(&literal->value)){std::ostringstream out;out<<std::setprecision(15)<<*number;return out.str();}
    if(auto boolean=std::get_if<bool>(&literal->value))return *boolean?"true":"false";
  }
  throw Error(raw->pos,what+" needs a value that can be known while building this page.","Pass Text, a number, Bool, or a component parameter with one of those values.");
}

std::string css_property(std::string name){std::replace(name.begin(),name.end(),'_','-');return name;}

bool unitless_css(const std::string& name){
  static const std::unordered_set<std::string> names={"opacity","z-index","font-weight","line-height","order","flex","flex-grow","flex-shrink","scale","zoom"};
  return names.contains(css_property(name));
}

std::string css_value(const ast::ExprPtr& raw,const Bindings& bindings,const std::string& property){
  auto value=resolve(raw,bindings);
  if(auto literal=std::dynamic_pointer_cast<ast::Literal>(value)){
    if(auto text=std::get_if<std::string>(&literal->value))return *text;
    if(auto integer=std::get_if<std::int64_t>(&literal->value))return std::to_string(*integer)+(unitless_css(property)?"":"px");
    if(auto number=std::get_if<double>(&literal->value)){std::ostringstream out;out<<std::setprecision(15)<<*number;if(!unitless_css(property))out<<"px";return out.str();}
    if(auto boolean=std::get_if<bool>(&literal->value))return *boolean?"true":"false";
  }
  throw Error(raw->pos,"CSS value for '"+property+"' must be known while building the page.");
}

bool is_web_component(const std::shared_ptr<ast::Function>& function){
  for(const auto& statement:function->body)if(std::dynamic_pointer_cast<ast::WebSection>(statement))return true;
  return false;
}

Component component_from(const std::shared_ptr<ast::Function>& function){
  Component out;out.function=function;
  for(const auto& statement:function->body){
    if(auto section=std::dynamic_pointer_cast<ast::WebSection>(statement)){
      if(section->kind=="html")out.html.push_back(section);
      else if(section->kind=="css"||section->kind=="style")out.css.push_back(section);
      else if(section->kind=="js")out.js.push_back(section);
    }else out.simple.push_back(statement);
  }
  return out;
}

std::string root_tag(const Component& component){
  for(const auto& section:component.html)if(!section->elements.empty()&&section->elements.front().tag!="native"&&section->elements.front().tag!="text")return section->elements.front().tag;
  return "div";
}

std::string render_element(const ast::WebElement& element,const Bindings& bindings,const std::string& component_class,const std::string& instance,bool root){
  if(element.tag=="native"){
    if(element.values.size()!=1)throw Error(element.pos,"native HTML needs exactly one Text value.");
    return text_value(element.values.front(),bindings,"native HTML");
  }
  if(element.tag=="text"){
    if(element.values.empty())return "";
    return html_escape(text_value(element.values.front(),bindings,"HTML text"));
  }
  std::ostringstream out;out<<'<'<<element.tag;
  bool first_is_map=false;
  if(root)out<<" class=\""<<component_class<<"\" data-se-instance=\""<<instance<<"\"";
  if(!element.values.empty()){
    auto possible_map=std::dynamic_pointer_cast<ast::Map>(resolve(element.values.front(),bindings));
    if(possible_map){
      first_is_map=true;
      for(const auto& [key,value]:possible_map->items){
        auto name=text_value(key,bindings,"HTML attribute name");
        auto val=text_value(value,bindings,"HTML attribute value");
        out<<' '<<name<<"=\""<<html_escape(val)<<"\"";
      }
    }
  }
  out<<'>';
  std::size_t content_index=first_is_map?1:0;
  if(element.values.size()>content_index)out<<html_escape(text_value(element.values[content_index],bindings,"HTML content"));
  for(const auto& child:element.children)out<<render_element(child,bindings,component_class,instance,false);
  static const std::unordered_set<std::string> void_tags={"area","base","br","col","embed","hr","img","input","link","meta","param","source","track","wbr"};
  if(!void_tags.contains(element.tag))out<<"</"<<element.tag<<'>';
  return out.str();
}

std::string simple_component_html(const Component& component,const Bindings& bindings,const std::string& component_class,const std::string& instance){
  std::ostringstream content;
  for(const auto& statement:component.simple){
    auto expression_statement=std::dynamic_pointer_cast<ast::ExprStmt>(statement);
    auto call=expression_statement?std::dynamic_pointer_cast<ast::Call>(expression_statement->value):nullptr;
    auto callee=call?std::dynamic_pointer_cast<ast::Variable>(call->callee):nullptr;
    if(callee&&callee->name=="text"&&call->args.size()==1){content<<html_escape(text_value(call->args.front(),bindings,"component text"));continue;}
    if(expression_statement)throw Error(statement->pos,"This simple component line is not a Web content form.","Use 'text value', or add an explicit html section.");
  }
  return "<div class=\""+component_class+"\" data-se-instance=\""+instance+"\">"+content.str()+"</div>";
}

std::string component_html(const Component& component,const Bindings& bindings,const std::string& instance){
  const auto class_name="se-"+component.function->name;
  if(component.html.empty())return simple_component_html(component,bindings,class_name,instance);
  std::vector<ast::WebElement> roots;
  for(const auto& section:component.html)roots.insert(roots.end(),section->elements.begin(),section->elements.end());
  if(roots.empty())return "<div class=\""+class_name+"\" data-se-instance=\""+instance+"\"></div>";
  if(roots.size()==1)return render_element(roots.front(),bindings,class_name,instance,true);
  std::ostringstream out;out<<"<div class=\""<<class_name<<"\" data-se-instance=\""<<instance<<"\">";
  for(const auto& item:roots)out<<render_element(item,bindings,class_name,instance,false);
  out<<"</div>";return out.str();
}

void emit_css_items(std::ostringstream& out,const std::vector<ast::WebCssItem>& items,const std::string& selector,const Bindings& bindings,const std::string& root_element){
  std::vector<const ast::WebCssItem*> declarations;
  for(const auto& item:items)if(item.children.empty())declarations.push_back(&item);
  if(!declarations.empty()){
    out<<selector<<" {\n";
    for(const auto* item:declarations){
      if(item->name=="native"){
        if(item->values.size()!=1)throw Error(item->pos,"native CSS needs exactly one Text value.");
        out<<"  "<<text_value(item->values.front(),bindings,"native CSS")<<"\n";continue;
      }
      if(item->values.size()!=1)throw Error(item->pos,"CSS property '"+item->name+"' needs one value.");
      out<<"  "<<css_property(item->name)<<": "<<css_value(item->values.front(),bindings,item->name)<<";\n";
    }
    out<<"}\n\n";
  }
  for(const auto& item:items)if(!item.children.empty()){
    std::string nested=item.name;
    if(nested.rfind(root_element+":",0)==0)nested=selector+nested.substr(root_element.size());
    else if(nested.rfind("&",0)==0)nested=selector+nested.substr(1);
    else nested=selector+" "+nested;
    emit_css_items(out,item.children,nested,bindings,root_element);
  }
}

std::string expression_js(const ast::ExprPtr& raw,const Bindings& bindings){
  auto e=resolve(raw,bindings);
  if(auto literal=std::dynamic_pointer_cast<ast::Literal>(e)){
    if(auto integer=std::get_if<std::int64_t>(&literal->value))return std::to_string(*integer);
    if(auto number=std::get_if<double>(&literal->value)){std::ostringstream out;out<<std::setprecision(17)<<*number;return out.str();}
    if(auto text=std::get_if<std::string>(&literal->value))return js_quote(*text);
    return std::get<bool>(literal->value)?"true":"false";
  }
  if(auto duration=std::dynamic_pointer_cast<ast::Duration>(e))return std::to_string(duration->milliseconds);
  if(auto variable=std::dynamic_pointer_cast<ast::Variable>(e))return variable->name;
  if(auto sized=std::dynamic_pointer_cast<ast::SizedList>(e)){
    std::string dims="["+expression_js(sized->count,bindings);
    for(std::size_t i=0;i<sized->other_counts.size();++i)dims+=","+expression_js(sized->other_counts[i],bindings);
    dims+="]";
    const std::string leaf=sized->element_type=="Text"?"\"\"":sized->element_type=="Bool"?"false":sized->element_type=="Num"?"0.0":"0";
    return "(()=>{const d="+dims+";let total=1,layer=1;for(const n of d){if(!Number.isSafeInteger(n)||n<0||n>1000000||(layer&&n>Math.floor(1000000/layer)))throw new Error('Invalid sized List dimensions.');layer*=n;total+=layer;if(total>1000000)throw new Error('A sized List exceeds 1000000 slots.');}const build=i=>i===d.length?"+leaf+":Array.from({length:d[i]},()=>build(i+1));return build(0);})()";
  }
  if(auto list=std::dynamic_pointer_cast<ast::List>(e)){
    std::string out="[";for(std::size_t i=0;i<list->items.size();++i){if(i)out+=",";out+=expression_js(list->items[i],bindings);}return out+"]";
  }
  if(auto map=std::dynamic_pointer_cast<ast::Map>(e)){
    std::string out="({";for(std::size_t i=0;i<map->items.size();++i){if(i)out+=",";out+="["+expression_js(map->items[i].first,bindings)+"]:"+expression_js(map->items[i].second,bindings);}return out+"})";
  }
  if(auto set=std::dynamic_pointer_cast<ast::Set>(e)){
    std::string out="new Set([";for(std::size_t i=0;i<set->items.size();++i){if(i)out+=",";out+=expression_js(set->items[i],bindings);}return out+"])";
  }
  if(auto index=std::dynamic_pointer_cast<ast::Index>(e))return expression_js(index->value,bindings)+"["+expression_js(index->index,bindings)+"]";
  if(auto range=std::dynamic_pointer_cast<ast::Range>(e))return "Array.from({length:("+expression_js(range->end,bindings)+")-("+expression_js(range->start,bindings)+")+1},(_,i)=>("+expression_js(range->start,bindings)+")+i)";
  if(auto unary=std::dynamic_pointer_cast<ast::Unary>(e))return std::string(unary->op==TokenKind::Not?"!":"-")+"("+expression_js(unary->value,bindings)+")";
  if(auto binary=std::dynamic_pointer_cast<ast::Binary>(e)){
    std::string op;
    switch(binary->op){
      case TokenKind::Plus:op="+";break;case TokenKind::Minus:op="-";break;case TokenKind::Star:op="*";break;case TokenKind::Slash:op="/";break;case TokenKind::Percent:op="%";break;case TokenKind::Power:op="**";break;
      case TokenKind::EqualEqual:op="===";break;case TokenKind::BangEqual:op="!==";break;case TokenKind::Greater:op=">";break;case TokenKind::Less:op="<";break;case TokenKind::GreaterEqual:op=">=";break;case TokenKind::LessEqual:op="<=";break;case TokenKind::And:op="&&";break;case TokenKind::Or:op="||";break;
      default:throw Error(e->pos,"This expression is not supported in component JavaScript yet.");
    }
    return "("+expression_js(binary->left,bindings)+" "+op+" "+expression_js(binary->right,bindings)+")";
  }
  if(auto member=std::dynamic_pointer_cast<ast::Member>(e))return expression_js(member->value,bindings)+"."+member->name;
  if(auto call=std::dynamic_pointer_cast<ast::Call>(e)){
    if(auto member=std::dynamic_pointer_cast<ast::Member>(call->callee)){
      auto base=std::dynamic_pointer_cast<ast::Variable>(member->value);
      if(base&&base->name=="async"&&member->name=="await"){
        if(call->args.size()!=1)throw Error(call->pos,"async.await needs one value in browser code.");
        return "await ("+expression_js(call->args.front(),bindings)+")";
      }
    }
    std::string out=expression_js(call->callee,bindings)+"(";
    for(std::size_t i=0;i<call->args.size();++i){if(i)out+=",";out+=expression_js(call->args[i],bindings);}return out+")";
  }
  if(auto attempt=std::dynamic_pointer_cast<ast::TryExpr>(e))return expression_js(attempt->value,bindings);
  throw Error(e->pos,"This SE value cannot be lowered to browser JavaScript yet.");
}

class EventEmitter {
public:
  explicit EventEmitter(Bindings bindings):bindings_(std::move(bindings)){}
  std::string block(const ast::Block& body,int depth=0){for(const auto& statement:body)emit(statement,depth);return out_.str();}
private:
  Bindings bindings_;
  std::ostringstream out_;
  std::unordered_set<std::string> locals_;
  void indent(int depth){for(int i=0;i<depth;++i)out_<<"  ";}
  void emit(const ast::StmtPtr& statement,int depth){
    if(auto say=std::dynamic_pointer_cast<ast::Say>(statement)){indent(depth);out_<<"console.log("<<expression_js(say->value,bindings_)<<");\n";return;}
    if(auto expr=std::dynamic_pointer_cast<ast::ExprStmt>(statement)){indent(depth);if(auto variable=std::dynamic_pointer_cast<ast::Variable>(expr->value))out_<<variable->name<<"();\n";else out_<<expression_js(expr->value,bindings_)<<";\n";return;}
    if(auto assign=std::dynamic_pointer_cast<ast::Assign>(statement)){
      auto variable=std::dynamic_pointer_cast<ast::Variable>(assign->target);indent(depth);
      if(variable&&!bindings_.contains(variable->name)&&!locals_.contains(variable->name)){locals_.insert(variable->name);out_<<"let ";}
      out_<<expression_js(assign->target,bindings_)<<" = "<<expression_js(assign->value,bindings_)<<";\n";return;
    }
    if(auto branch=std::dynamic_pointer_cast<ast::If>(statement)){
      indent(depth);out_<<"if("<<expression_js(branch->condition,bindings_)<<"){\n";
      for(const auto& item:branch->then_block){emit(item,depth+1);}
      indent(depth);out_<<"}";
      if(!branch->else_block.empty()){
        out_<<"else{\n";
        for(const auto& item:branch->else_block){emit(item,depth+1);}
        indent(depth);out_<<"}";
      }
      out_<<"\n";return;
    }
    if(auto repeat=std::dynamic_pointer_cast<ast::Repeat>(statement)){
      auto name="__seRepeat"+std::to_string(locals_.size());indent(depth);out_<<"for(let "<<name<<"=0;"<<name<<"<"<<expression_js(repeat->count,bindings_)<<";"<<name<<"+=1){\n";
      for(const auto& item:repeat->body){emit(item,depth+1);}
      indent(depth);out_<<"}\n";return;
    }
    if(auto loop=std::dynamic_pointer_cast<ast::While>(statement)){
      indent(depth);out_<<"while("<<expression_js(loop->condition,bindings_)<<"){\n";
      for(const auto& item:loop->body){emit(item,depth+1);}
      indent(depth);out_<<"}\n";return;
    }
    if(auto loop=std::dynamic_pointer_cast<ast::For>(statement)){
      indent(depth);
      if(loop->names.size()==1)out_<<"for(const "<<loop->names[0]<<" of "<<expression_js(loop->values,bindings_)<<"){\n";
      else if(loop->names.size()==2)out_<<"for(const ["<<loop->names[0]<<","<<loop->names[1]<<"] of Object.entries("<<expression_js(loop->values,bindings_)<<")){\n";
      else throw Error(loop->pos,"Browser for loops support one value or key/value names.");
      for(const auto& item:loop->body){emit(item,depth+1);}
      indent(depth);out_<<"}\n";return;
    }
    if(auto attempt=std::dynamic_pointer_cast<ast::Try>(statement)){
      indent(depth);out_<<"try{\n";
      for(const auto& item:attempt->body){emit(item,depth+1);}
      indent(depth);out_<<"}catch("<<attempt->error_name<<"){\n";
      for(const auto& item:attempt->else_block){emit(item,depth+1);}
      indent(depth);out_<<"}\n";return;
    }
    if(auto match=std::dynamic_pointer_cast<ast::Match>(statement)){
      auto name="__seMatch"+std::to_string(locals_.size());indent(depth);out_<<"const "<<name<<" = "<<expression_js(match->value,bindings_)<<";\n";
      for(std::size_t i=0;i<match->cases.size();++i){
        indent(depth);out_<<(i?"else if(":"if(")<<name<<" === "<<expression_js(match->cases[i].pattern,bindings_)<<"){\n";
        for(const auto& item:match->cases[i].body){emit(item,depth+1);}
        indent(depth);out_<<"}";
        if(i+1==match->cases.size()&&match->else_block.empty())out_<<"\n";
      }
      if(!match->else_block.empty()){
        out_<<"else{\n";
        for(const auto& item:match->else_block){emit(item,depth+1);}
        indent(depth);out_<<"}\n";
      }
      return;
    }
    if(auto give=std::dynamic_pointer_cast<ast::Give>(statement)){indent(depth);out_<<"return "<<expression_js(give->value,bindings_)<<";\n";return;}
    if(auto fail=std::dynamic_pointer_cast<ast::Fail>(statement)){indent(depth);out_<<"throw new Error(String("<<expression_js(fail->value,bindings_)<<"));\n";return;}
    throw Error(statement->pos,"This SE statement is not supported inside a component event yet.");
  }
};

Bindings bind_component(const Component& component,const std::shared_ptr<ast::Call>& call){
  if(call->args.size()!=component.function->params.size())throw Error(call->pos,"Component '"+component.function->name+"' needs "+std::to_string(component.function->params.size())+" value(s), but got "+std::to_string(call->args.size())+".");
  Bindings bindings;for(std::size_t i=0;i<call->args.size();++i)bindings[component.function->params[i]]=call->args[i];return bindings;
}

std::shared_ptr<ast::Call> component_call(const ast::StmtPtr& statement){
  auto expression=std::dynamic_pointer_cast<ast::ExprStmt>(statement);
  if(!expression)return nullptr;
  if(auto call=std::dynamic_pointer_cast<ast::Call>(expression->value))return call;
  if(auto variable=std::dynamic_pointer_cast<ast::Variable>(expression->value))return std::make_shared<ast::Call>(variable->pos,variable,std::vector<ast::ExprPtr>{});
  return nullptr;
}
std::string component_name(const std::shared_ptr<ast::Call>& call){if(!call)return {};auto variable=std::dynamic_pointer_cast<ast::Variable>(call->callee);return variable?variable->name:std::string{};}

std::string browser_runtime(){
  return R"JS(
class SEBrowserError extends Error {
  constructor(kind, message, details = {}) { super(message); this.name = "SEBrowserError"; this.kind = kind; Object.assign(this, details); }
}
const __seActiveRequests = new Map();
const __seSleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const __seRetryStatuses = new Set([408, 425, 429, 500, 502, 503, 504]);
const __seNormalizeRoute = route => { const path = new URL(route, location.href).pathname || "/"; return path.length > 1 && path.endsWith("/") ? path.slice(0, -1) : path; };
function __seShowRoute(route) {
  const normalized = __seNormalizeRoute(route);
  if (!__seRoutes.has(normalized)) return false;
  document.querySelectorAll("[data-se-page]").forEach(page => {
    const active = page.dataset.sePage === normalized;
    page.hidden = !active;
    page.setAttribute("aria-hidden", active ? "false" : "true");
  });
  return true;
}
function __seHeadersObject(headers) { const out = {}; headers.forEach((value, key) => { out[key] = value; }); return out; }
function __seRetryAfter(headers, fallback) {
  const value = headers["retry-after"];
  if (!value) return fallback;
  const seconds = Number(value);
  if (Number.isFinite(seconds)) return Math.max(0, seconds * 1000);
  const date = Date.parse(value);
  return Number.isFinite(date) ? Math.max(0, date - Date.now()) : fallback;
}
async function __seParseResponse(response, expect) {
  const text = response.status === 204 ? "" : await response.text();
  const headers = __seHeadersObject(response.headers);
  const contentType = response.headers.get("content-type") || "";
  let data = text;
  let json = null;
  let parse_error = null;
  const wantsJson = expect === "json" || contentType.includes("application/json") || contentType.includes("+json");
  if (wantsJson && text !== "") {
    try { json = JSON.parse(text); data = json; }
    catch (error) { parse_error = String(error && error.message ? error.message : error); if (expect === "json") throw new SEBrowserError("parse", "API returned invalid JSON.", { status: response.status, text, cause: error }); }
  }
  return { ok: response.ok, status: response.status, status_text: response.statusText, url: response.url, headers, text, json, data, content_type: contentType, parse_error };
}
async function __seRequest(url, options = {}) {
  const method = String(options.method || "GET").toUpperCase();
  const headers = new Headers(options.headers || {});
  let body = options.body;
  if (Object.prototype.hasOwnProperty.call(options, "json")) { body = JSON.stringify(options.json); if (!headers.has("content-type")) headers.set("content-type", "application/json"); }
  const timeout = Math.max(0, Number(options.timeout ?? 15000));
  const retries = Math.max(0, Number(options.retries ?? 0));
  const retryDelay = Math.max(0, Number(options.retry_delay ?? 300));
  const dedupe = options.dedupe ?? (method === "GET" || method === "HEAD");
  const key = String(options.key || `${method}:${url}:${typeof body === "string" ? body : ""}`);
  if (dedupe && __seActiveRequests.has(key)) return __seActiveRequests.get(key).promise;
  const record = { controller: null, cancelled: false, promise: null };
  const run = async () => {
    for (let attempt = 0; attempt <= retries; attempt += 1) {
      if (record.cancelled) throw new SEBrowserError("cancelled", "Request was cancelled.", { key });
      if (typeof navigator !== "undefined" && navigator.onLine === false) throw new SEBrowserError("offline", "Browser is offline.", { key });
      const controller = new AbortController(); record.controller = controller;
      let timedOut = false;
      const timer = timeout > 0 ? setTimeout(() => { timedOut = true; controller.abort(); }, timeout) : null;
      try {
        const response = await fetch(url, { method, headers, body, credentials: options.credentials || "same-origin", mode: options.mode, cache: options.cache, redirect: options.redirect, signal: controller.signal });
        if (timer) clearTimeout(timer);
        const result = await __seParseResponse(response, options.expect || "auto");
        if (__seRetryStatuses.has(result.status) && attempt < retries) { await __seSleep(__seRetryAfter(result.headers, retryDelay * (2 ** attempt))); continue; }
        if (options.throw_http && !result.ok) throw new SEBrowserError("http", `HTTP ${result.status} ${result.status_text}`.trim(), { response: result, status: result.status });
        return result;
      } catch (error) {
        if (timer) clearTimeout(timer);
        if (error instanceof SEBrowserError && (error.kind === "http" || error.kind === "parse")) throw error;
        if (record.cancelled) throw new SEBrowserError("cancelled", "Request was cancelled.", { key, cause: error });
        const kind = timedOut ? "timeout" : (typeof navigator !== "undefined" && navigator.onLine === false ? "offline" : "network");
        if (attempt < retries) { await __seSleep(retryDelay * (2 ** attempt)); continue; }
        const message = kind === "timeout" ? "Request timed out." : kind === "offline" ? "Browser is offline." : "Network request failed. This can also be caused by CORS.";
        throw new SEBrowserError(kind, message, { key, cause: error });
      }
    }
    throw new SEBrowserError("network", "Request failed.", { key });
  };
  record.promise = run().finally(() => { if (__seActiveRequests.get(key) === record) __seActiveRequests.delete(key); });
  __seActiveRequests.set(key, record);
  return record.promise;
}
function __seElement(selector) { const element = typeof selector === "string" ? document.querySelector(selector) : selector; if (!element) throw new SEBrowserError("dom", `Element not found: ${selector}`); return element; }
const browser = Object.freeze({
  request: __seRequest,
  get: (url, options = {}) => __seRequest(url, { ...options, method: "GET" }),
  get_json: (url, options = {}) => __seRequest(url, { ...options, method: "GET", expect: "json" }),
  post: (url, body, options = {}) => __seRequest(url, { ...options, method: "POST", body }),
  post_json: (url, json, options = {}) => __seRequest(url, { ...options, method: "POST", json }),
  put_json: (url, json, options = {}) => __seRequest(url, { ...options, method: "PUT", json }),
  patch_json: (url, json, options = {}) => __seRequest(url, { ...options, method: "PATCH", json }),
  delete: (url, options = {}) => __seRequest(url, { ...options, method: "DELETE" }),
  submit_json: (url, selector, options = {}) => __seRequest(url, { ...options, method: "POST", json: browser.form_json(selector) }),
  upload: (url, selector, options = {}) => __seRequest(url, { ...options, method: options.method || "POST", body: new FormData(__seElement(selector)) }),
  cancel: key => { const record = __seActiveRequests.get(String(key)); if (!record) return false; record.cancelled = true; if (record.controller) record.controller.abort(); return true; },
  cancel_all: () => { for (const [key, record] of __seActiveRequests) { record.cancelled = true; if (record.controller) record.controller.abort(); __seActiveRequests.delete(key); } },
  query: (url, values = {}) => { const target = new URL(url, location.href); for (const [key, value] of Object.entries(values)) { if (Array.isArray(value)) value.forEach(item => target.searchParams.append(key, String(item))); else if (value !== null && value !== undefined) target.searchParams.set(key, String(value)); } return target.origin === location.origin ? target.pathname + target.search + target.hash : target.toString(); },
  go: route => { const target = new URL(route, location.href); if (target.origin !== location.origin || !__seShowRoute(target.pathname)) { location.assign(target.toString()); return; } history.pushState({}, "", target.pathname + target.search + target.hash); },
  replace: route => { const target = new URL(route, location.href); if (target.origin !== location.origin || !__seShowRoute(target.pathname)) { location.replace(target.toString()); return; } history.replaceState({}, "", target.pathname + target.search + target.hash); },
  back: () => history.back(), forward: () => history.forward(), reload: () => location.reload(), open: (url, target = "_blank") => window.open(url, target, "noopener,noreferrer"),
  text: (selector, value) => { __seElement(selector).textContent = String(value ?? ""); },
  html: (selector, value) => { __seElement(selector).innerHTML = String(value ?? ""); },
  value: selector => __seElement(selector).value,
  set_value: (selector, value) => { __seElement(selector).value = value ?? ""; },
  attr: (selector, name, value) => { const element = __seElement(selector); if (value === null || value === false) element.removeAttribute(name); else element.setAttribute(name, String(value)); },
  show: selector => { __seElement(selector).hidden = false; }, hide: selector => { __seElement(selector).hidden = true; },
  disable: selector => { __seElement(selector).disabled = true; }, enable: selector => { __seElement(selector).disabled = false; },
  form_json: selector => Object.fromEntries(new FormData(__seElement(selector)).entries()),
  pretty: value => JSON.stringify(value, null, 2),
  online: () => typeof navigator === "undefined" ? true : navigator.onLine
});
window.addEventListener("popstate", () => { __seShowRoute(location.pathname); });
document.addEventListener("click", event => {
  if (event.defaultPrevented || event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
  const anchor = event.target.closest ? event.target.closest("a[href]") : null;
  if (!anchor || anchor.target === "_blank" || anchor.hasAttribute("download")) return;
  const target = new URL(anchor.href, location.href);
  if (target.origin === location.origin && __seRoutes.has(__seNormalizeRoute(target.pathname))) { event.preventDefault(); browser.go(target.pathname + target.search + target.hash); }
});
window.addEventListener("online", () => document.dispatchEvent(new CustomEvent("se:online")));
window.addEventListener("offline", () => document.dispatchEvent(new CustomEvent("se:offline")));
__seShowRoute(location.pathname) || __seShowRoute("/");
)JS";
}

} // namespace

bool has_component_web(const ast::Program& program){
  for(const auto& statement:program.statements){
    if(std::dynamic_pointer_cast<ast::Page>(statement))return true;
    if(auto function=std::dynamic_pointer_cast<ast::Function>(statement))if(is_web_component(function))return true;
  }
  return false;
}

WebBundle ComponentWebCompiler::generate(const ast::Program& program){
  std::unordered_map<std::string,Component> components;
  std::vector<std::shared_ptr<ast::Page>> pages;
  std::vector<std::shared_ptr<ast::WebSection>> global_css;
  for(const auto& statement:program.statements){
    if(auto function=std::dynamic_pointer_cast<ast::Function>(statement);function&&is_web_component(function))components.emplace(function->name,component_from(function));
    else if(auto page=std::dynamic_pointer_cast<ast::Page>(statement))pages.push_back(page);
    else if(auto section=std::dynamic_pointer_cast<ast::WebSection>(statement);section&&(section->kind=="css"||section->kind=="style"))global_css.push_back(section);
  }
  if(pages.empty())throw Error({1,1},"This component-style web build needs a page block.","Add page \"/\" and place your components inside it.");

  std::ostringstream body,css,js;
  std::size_t instance_number=0;
  std::unordered_set<std::string> emitted_css;
  std::unordered_set<std::string> routes;
  for(const auto& section:global_css)emit_css_items(css,section->css,"body",{},"body");
  css<<"[data-se-page][hidden] { display: none !important; }\n\n";

  for(const auto& page:pages){
    auto route_literal=std::dynamic_pointer_cast<ast::Literal>(page->route);
    auto route=route_literal?std::get_if<std::string>(&route_literal->value):nullptr;
    if(!route)throw Error(page->pos,"page route must be literal Text, like page \"/\".");
    std::string normalized=*route;
    if(normalized.empty()||normalized.front()!='/')throw Error(page->pos,"page route must begin with '/'.");
    if(normalized.size()>1&&normalized.back()=='/')normalized.pop_back();
    if(routes.contains(normalized))throw Error(page->pos,"Duplicate page route '"+normalized+"'.");
    routes.insert(normalized);
    body<<"  <main data-se-page=\""<<html_escape(normalized)<<"\" aria-hidden=\"true\" hidden>\n";
    for(const auto& statement:page->body){
      auto call=component_call(statement);auto name=component_name(call);
      if(name.empty())throw Error(statement->pos,"A page currently contains component calls.","Define a component with make, then write ComponentName values inside page.");
      auto found=components.find(name);
      if(found==components.end())throw Error(statement->pos,"'"+name+"' is not a Web component.","Give it an html, css/style, or js section inside make.");
      auto bindings=bind_component(found->second,call);
      auto instance="se-"+name+"-"+std::to_string(++instance_number);
      body<<"    "<<component_html(found->second,bindings,instance)<<"\n";
      if(!emitted_css.contains(name)){
        const auto selector=".se-"+name;const auto tag=root_tag(found->second);
        for(const auto& section:found->second.css)emit_css_items(css,section->css,selector,{},tag);
        emitted_css.insert(name);
      }
      for(const auto& section:found->second.js){
        for(const auto& native:section->native)js<<text_value(native,bindings,"native JavaScript")<<"\n";
        for(const auto& event:section->events){
          const auto selector="[data-se-instance=\""+instance+"\"]";
          js<<"{ const element = document.querySelector("<<js_quote(selector)<<"); if(element) element.addEventListener("<<js_quote(event.event)<<", async event => {\n";
          js<<EventEmitter(bindings).block(event.body,1);
          js<<"}); }\n";
        }
      }
    }
    body<<"  </main>\n";
  }

  std::ostringstream route_js;route_js<<"const __seRoutes = new Set([";std::size_t route_index=0;for(const auto& route:routes){if(route_index++)route_js<<',';route_js<<js_quote(route);}route_js<<"]);\n";
  std::ostringstream html;
  html<<"<!doctype html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n  <title>SE Web</title>\n  <link rel=\"stylesheet\" href=\"./style.css\">\n</head>\n<body>\n";
  html<<body.str();
  html<<"  <script type=\"module\" src=\"./app.js\"></script>\n</body>\n</html>\n";
  std::string banner="// Generated by SE component web build. Edit the .se source, not this file.\n";
  auto javascript=banner+route_js.str()+browser_runtime()+js.str();
  auto typescript="// TypeScript-compatible output generated from SE.\n"+javascript;
  return {html.str(),css.str(),std::move(javascript),std::move(typescript)};
}

} // namespace s
