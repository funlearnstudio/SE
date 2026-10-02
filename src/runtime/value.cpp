#include "s/value.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace s {
namespace {
bool numeric(const Value&v){return std::holds_alternative<std::int64_t>(v.data())||std::holds_alternative<double>(v.data());}
double number(const Value&v,SourcePos p){if(auto x=std::get_if<std::int64_t>(&v.data()))return static_cast<double>(*x);if(auto x=std::get_if<double>(&v.data()))return *x;throw Error(p,"This operation needs a number, but got "+v.type_name()+".");}
}

std::string Value::type_name() const {
  switch(data_.index()){
    case 0:return "None";case 1:return "Int";case 2:return "Num";case 3:return "Bool";case 4:return "Text";
    case 5:return "Duration";case 6:return "Time";case 7:return "Path";case 8:return "Error";case 9:return "List";
    case 10:return "Map";case 11:return "Set";case 12:return "Function";case 13:return "Object";case 14:return "Type";
    case 15:return "Module";case 16:return "File";case 17:return "NativeHandle";case 18:return "Bytes";default:return "Value";
  }
}

std::string Value::text() const {
  if(std::holds_alternative<std::monostate>(data_))return "none";
  if(auto x=std::get_if<std::int64_t>(&data_))return std::to_string(*x);
  if(auto x=std::get_if<double>(&data_)){std::ostringstream o;o<<std::setprecision(12)<<*x;return o.str();}
  if(auto x=std::get_if<bool>(&data_))return *x?"true":"false";
  if(auto x=std::get_if<std::string>(&data_))return *x;
  if(auto x=std::get_if<DurationData>(&data_)){if(x->milliseconds%60000==0)return std::to_string(x->milliseconds/60000)+"min";if(x->milliseconds%1000==0)return std::to_string(x->milliseconds/1000)+"s";return std::to_string(x->milliseconds)+"ms";}
  if(auto x=std::get_if<TimeData>(&data_)){auto tt=std::chrono::system_clock::to_time_t(x->point);std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm,&tt);
#else
    gmtime_r(&tt,&tm);
#endif
    std::ostringstream o;o<<std::put_time(&tm,"%Y-%m-%dT%H:%M:%SZ");return o.str();}
  if(auto x=std::get_if<PathData>(&data_))return x->path.string();
  if(auto x=std::get_if<std::shared_ptr<ErrorData>>(&data_))return (*x)->message;
  if(auto x=std::get_if<std::shared_ptr<ListData>>(&data_)){std::string r="[";for(std::size_t i=0;i<(*x)->items.size();++i){if(i)r+=", ";r+=(*x)->items[i].text();}return r+"]";}
  if(auto x=std::get_if<std::shared_ptr<MapData>>(&data_)){std::string r="[";for(std::size_t i=0;i<(*x)->items.size();++i){if(i)r+=", ";r+='"'+(*x)->items[i].first+"\": "+(*x)->items[i].second.text();}return r+"]";}
  if(auto x=std::get_if<std::shared_ptr<SetData>>(&data_)){std::string r="set [";for(std::size_t i=0;i<(*x)->items.size();++i){if(i)r+=", ";r+=(*x)->items[i].text();}return r+"]";}
  if(std::holds_alternative<std::shared_ptr<CallableData>>(data_))return "<function>";
  if(std::holds_alternative<std::shared_ptr<ObjectData>>(data_))return "<object>";
  if(std::holds_alternative<std::shared_ptr<TypeData>>(data_))return "<type>";
  if(auto x=std::get_if<std::shared_ptr<ModuleData>>(&data_))return "<module "+(*x)->name+">";
  if(auto x=std::get_if<std::shared_ptr<FileData>>(&data_))return "<file "+(*x)->path.string()+">";
  if(auto x=std::get_if<std::shared_ptr<NativeHandleData>>(&data_))return "<native "+(*x)->tag+">";
  if(auto x=std::get_if<std::shared_ptr<ByteBufferData>>(&data_))return "<bytes "+std::to_string((*x)->bytes.size())+">";
  return "value";
}

bool Value::truth(SourcePos p) const { if(auto b=std::get_if<bool>(&data_))return *b;throw Error(p,"A condition needs Bool, but got "+type_name()+"."); }

bool Environment::has(const std::string&n) const {
  if(values_.contains(n))return true;
  if((n=="self"||n=="this")&&self_)return true;
  if(self_&&self_->fields.contains(n))return true;
  return parent_&&parent_->has(n);
}
Value Environment::get(const std::string&n,SourcePos p) const {
  if(auto i=values_.find(n);i!=values_.end())return i->second;
  if((n=="self"||n=="this")&&self_)return self_;
  if(self_){if(auto i=self_->fields.find(n);i!=self_->fields.end())return i->second;}
  if(parent_)return parent_->get(n,p);
  throw Error(p,"I don't know what \""+n+"\" is.","Create it first with "+n+" = value.");
}
void Environment::define(const std::string&n,Value v){values_[n]=std::move(v);}
void Environment::set(const std::string&n,Value v){
  if(auto i=values_.find(n);i!=values_.end()){i->second=std::move(v);return;}
  if(self_){if(auto i=self_->fields.find(n);i!=self_->fields.end()){i->second=std::move(v);return;}}
  if(parent_&&parent_->has(n)){parent_->set(n,std::move(v));return;}
  values_[n]=std::move(v);
}

bool value_equal(const Value&a,const Value&b){
  if(numeric(a)&&numeric(b))return number(a,{})==number(b,{});
  if(a.data().index()!=b.data().index())return false;
  if(std::holds_alternative<std::monostate>(a.data()))return true;
  if(auto x=std::get_if<std::int64_t>(&a.data()))return *x==std::get<std::int64_t>(b.data());
  if(auto x=std::get_if<double>(&a.data()))return *x==std::get<double>(b.data());
  if(auto x=std::get_if<bool>(&a.data()))return *x==std::get<bool>(b.data());
  if(auto x=std::get_if<std::string>(&a.data()))return *x==std::get<std::string>(b.data());
  if(auto x=std::get_if<DurationData>(&a.data()))return x->milliseconds==std::get<DurationData>(b.data()).milliseconds;
  if(auto x=std::get_if<PathData>(&a.data()))return x->path==std::get<PathData>(b.data()).path;
  if(auto x=std::get_if<std::shared_ptr<ObjectData>>(&a.data()))return *x==std::get<std::shared_ptr<ObjectData>>(b.data());
  if(auto x=std::get_if<std::shared_ptr<NativeHandleData>>(&a.data()))return (*x)->resource==std::get<std::shared_ptr<NativeHandleData>>(b.data())->resource;
  if(auto x=std::get_if<std::shared_ptr<ByteBufferData>>(&a.data()))return (*x)->bytes==std::get<std::shared_ptr<ByteBufferData>>(b.data())->bytes;
  return a.text()==b.text();
}

std::string path_text(const Value&v,SourcePos p){if(auto s=std::get_if<std::string>(&v.data()))return *s;if(auto q=std::get_if<PathData>(&v.data()))return q->path.string();throw Error(p,"A file path needs Text or Path.");}

Value binary(TokenKind op,const Value&a,const Value&b,SourcePos p){
  if(op==TokenKind::EqualEqual)return value_equal(a,b);
  if(op==TokenKind::BangEqual)return !value_equal(a,b);
  if(op==TokenKind::And)return a.truth(p)&&b.truth(p);
  if(op==TokenKind::Or)return a.truth(p)||b.truth(p);
  if(op==TokenKind::In){
    if(auto l=std::get_if<std::shared_ptr<ListData>>(&b.data())){for(auto&v:(*l)->items)if(value_equal(a,v))return true;return false;}
    if(auto s=std::get_if<std::shared_ptr<SetData>>(&b.data())){for(auto&v:(*s)->items)if(value_equal(a,v))return true;return false;}
    if(auto m=std::get_if<std::shared_ptr<MapData>>(&b.data())){if(!std::holds_alternative<std::string>(a.data()))throw Error(p,"A Map key needs Text.");auto k=std::get<std::string>(a.data());for(auto&v:(*m)->items)if(v.first==k)return true;return false;}
    if(auto s=std::get_if<std::string>(&b.data())){if(!std::holds_alternative<std::string>(a.data()))throw Error(p,"Text membership needs Text.");return s->find(std::get<std::string>(a.data()))!=std::string::npos;}
    throw Error(p,"'in' works with List, Set, Map, or Text.");
  }
  if(op==TokenKind::Plus&&(std::holds_alternative<std::string>(a.data())||std::holds_alternative<std::string>(b.data())))return a.text()+b.text();
  if(op==TokenKind::Greater||op==TokenKind::Less||op==TokenKind::GreaterEqual||op==TokenKind::LessEqual){double x=number(a,p),y=number(b,p);if(op==TokenKind::Greater)return x>y;if(op==TokenKind::Less)return x<y;if(op==TokenKind::GreaterEqual)return x>=y;return x<=y;}
  bool ints=std::holds_alternative<std::int64_t>(a.data())&&std::holds_alternative<std::int64_t>(b.data()); double x=number(a,p),y=number(b,p);
  if(op==TokenKind::Plus)return ints?Value(std::get<std::int64_t>(a.data())+std::get<std::int64_t>(b.data())):Value(x+y);
  if(op==TokenKind::Minus)return ints?Value(std::get<std::int64_t>(a.data())-std::get<std::int64_t>(b.data())):Value(x-y);
  if(op==TokenKind::Star)return ints?Value(std::get<std::int64_t>(a.data())*std::get<std::int64_t>(b.data())):Value(x*y);
  if(op==TokenKind::Slash){if(y==0)throw Error(p,"Division by zero.");return x/y;}
  if(op==TokenKind::Percent){if(y==0)throw Error(p,"Division by zero.");return ints?Value(std::get<std::int64_t>(a.data())%std::get<std::int64_t>(b.data())):Value(std::fmod(x,y));}
  if(op==TokenKind::Power)return std::pow(x,y);
  throw Error(p,"This operation is not supported.");
}

} // namespace s
