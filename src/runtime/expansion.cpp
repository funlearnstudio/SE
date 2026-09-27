#include "s/expansion.hpp"
#include "s/error.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <cstdint>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace s {
namespace {
using Args=std::vector<Value>;
using Op=std::function<Value(const Args&,SourcePos)>;
struct Entry { std::string name; std::vector<TypeInfo> params; TypeInfo result; Op op; bool fallible=false; };
TypeInfo t(TypeKind kind){return TypeInfo(kind);}
TypeInfo list_t(){TypeInfo v(TypeKind::List);v.element=std::make_shared<TypeInfo>();return v;}
TypeInfo map_t(){TypeInfo v(TypeKind::Map);v.key=std::make_shared<TypeInfo>(t(TypeKind::Text));v.value=std::make_shared<TypeInfo>();return v;}
std::string str(const Value& v,SourcePos p){if(auto q=std::get_if<std::string>(&v.data()))return *q;throw Error(p,"Expected Text.");}
double num(const Value& v,SourcePos p){if(auto q=std::get_if<double>(&v.data()))return *q;if(auto q=std::get_if<std::int64_t>(&v.data()))return static_cast<double>(*q);throw Error(p,"Expected number.");}
std::int64_t integer(const Value& v,SourcePos p){if(auto q=std::get_if<std::int64_t>(&v.data()))return *q;throw Error(p,"Expected Int.");}
const Args& items(const Value& v,SourcePos p){if(auto q=std::get_if<std::shared_ptr<ListData>>(&v.data()))return (*q)->items;throw Error(p,"Expected List.");}
Value list(std::vector<Value> a){auto q=std::make_shared<ListData>();q->items=std::move(a);return Value(q);}
Value map(std::vector<std::pair<std::string,Value>> a){auto q=std::make_shared<MapData>();q->items=std::move(a);return Value(q);}
std::string trim(std::string s){auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos)return "";return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
std::vector<double> vector_of(const Value& v,SourcePos p){std::vector<double> out;for(auto& x:items(v,p))out.push_back(num(x,p));return out;}
std::vector<std::vector<double>> matrix_of(const Value& v,SourcePos p){std::vector<std::vector<double>> out;for(auto& row:items(v,p))out.push_back(vector_of(row,p));if(out.empty()||out[0].empty())throw Error(p,"Matrix must be nonempty.");for(auto& row:out)if(row.size()!=out[0].size())throw Error(p,"Matrix rows must have equal length.");return out;}
Value matrix_value(const std::vector<std::vector<double>>& a){std::vector<Value> rows;for(auto& row:a){std::vector<Value> r;for(auto v:row)r.emplace_back(v);rows.push_back(list(std::move(r)));}return list(std::move(rows));}
std::string percent_encode(const std::string& s){std::ostringstream o;o<<std::uppercase<<std::hex<<std::setfill('0');for(unsigned char c:s){if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')o<<c;else o<<'%'<<std::setw(2)<<static_cast<int>(c);}return o.str();}
std::string percent_decode(const std::string& s,SourcePos p){std::string out;for(std::size_t i=0;i<s.size();++i){if(s[i]=='%'){if(i+2>=s.size()||!std::isxdigit(static_cast<unsigned char>(s[i+1]))||!std::isxdigit(static_cast<unsigned char>(s[i+2])))throw Error(p,"Invalid percent encoding.");out+=static_cast<char>(std::stoi(s.substr(i+1,2),nullptr,16));i+=2;}else out+=s[i];}return out;}
std::vector<Entry> entries(const std::string& name){
 const auto txt=t(TypeKind::Text), n=t(TypeKind::Num), i=t(TypeKind::Int), l=list_t(), m=map_t(), b=t(TypeKind::Bool);
 if(name=="url")return {
  {"encode",{txt},txt,[](const Args&a,SourcePos p){return Value(percent_encode(str(a[0],p)));}},
  {"decode",{txt},txt,[](const Args&a,SourcePos p){return Value(percent_decode(str(a[0],p),p));},true},
  {"query",{m},txt,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"Expected Map.");std::string out;for(auto& [key,val]:(*q)->items){if(!out.empty())out+='&';out+=percent_encode(key)+"="+percent_encode(str(val,p));}return Value(out);}},
  {"parse_query",{txt},m,[](const Args&a,SourcePos p){std::string s=str(a[0],p);if(!s.empty()&&s[0]=='?')s.erase(0,1);std::vector<std::pair<std::string,Value>> out;std::istringstream in(s);std::string pair;while(std::getline(in,pair,'&')){if(pair.empty())continue;auto eq=pair.find('=');auto key=percent_decode(pair.substr(0,eq),p);auto val=eq==std::string::npos?"":percent_decode(pair.substr(eq+1),p);std::replace(key.begin(),key.end(),'+',' ');std::replace(val.begin(),val.end(),'+',' ');out.emplace_back(key,Value(val));}return map(std::move(out));},true}
 };
 if(name=="encoding")return {
  {"hex",{txt},txt,[](const Args&a,SourcePos p){std::ostringstream out;out<<std::hex<<std::setfill('0');for(unsigned char c:str(a[0],p))out<<std::setw(2)<<int(c);return Value(out.str());}},
  {"unhex",{txt},txt,[](const Args&a,SourcePos p){auto s=str(a[0],p);if(s.size()%2)throw Error(p,"Hex needs an even number of digits.");std::string out;for(std::size_t j=0;j<s.size();j+=2){if(!std::isxdigit(static_cast<unsigned char>(s[j]))||!std::isxdigit(static_cast<unsigned char>(s[j+1])))throw Error(p,"Invalid hex digit.");out+=static_cast<char>(std::stoi(s.substr(j,2),nullptr,16));}return Value(out);},true},
  {"utf8_valid",{txt},b,[](const Args&a,SourcePos p){auto s=str(a[0],p);for(std::size_t j=0;j<s.size();){unsigned char c=s[j];std::size_t count=c<0x80?1:(c>=0xC2&&c<=0xDF?2:(c>=0xE0&&c<=0xEF?3:(c>=0xF0&&c<=0xF4?4:0)));if(!count||j+count>s.size())return Value(false);if(count>1){for(std::size_t k=1;k<count;++k)if((static_cast<unsigned char>(s[j+k])&0xC0)!=0x80)return Value(false);unsigned char next=s[j+1];if((c==0xE0&&next<0xA0)||(c==0xED&&next>=0xA0)||(c==0xF0&&next<0x90)||(c==0xF4&&next>=0x90))return Value(false);}j+=count;}return Value(true);}}
 };
 if(name=="dotenv"||name=="config")return {
  {"parse",{txt},m,[](const Args&a,SourcePos p){std::istringstream in(str(a[0],p));std::vector<std::pair<std::string,Value>> out;std::string line;while(std::getline(in,line)){line=trim(line);if(line.empty()||line[0]=='#')continue;if(line.rfind("export ",0)==0)line=trim(line.substr(7));auto pos=line.find('=');if(pos==std::string::npos)throw Error(p,"Expected KEY=VALUE.");auto key=trim(line.substr(0,pos)),val=trim(line.substr(pos+1));if(key.empty()||!(std::isalpha(static_cast<unsigned char>(key[0]))||key[0]=='_')||!std::all_of(key.begin()+1,key.end(),[](unsigned char c){return std::isalnum(c)||c=='_';}))throw Error(p,"Invalid configuration key.");if(val.size()>=2&&((val.front()=='"'&&val.back()=='"')||(val.front()=='\''&&val.back()=='\'')))val=val.substr(1,val.size()-2);out.emplace_back(key,Value(val));}return map(std::move(out));},true},
  {"get",{m,txt},txt,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"Expected Map.");auto key=str(a[1],p);for(auto& [k,v]:(*q)->items)if(k==key)return v;throw Error(p,"Missing configuration key: "+key);},true}
 };
 if(name=="array"||name=="series")return {
  {"sum",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);return Value(std::accumulate(v.begin(),v.end(),0.0));}},
  {"mean",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);if(v.empty())throw Error(p,"Mean needs values.");return Value(std::accumulate(v.begin(),v.end(),0.0)/v.size());},true},
  {"slice",{l,i,i},l,[](const Args&a,SourcePos p){auto& v=items(a[0],p);auto from=integer(a[1],p),to=integer(a[2],p);if(from<0||to<from||static_cast<std::size_t>(to)>v.size())throw Error(p,"Invalid slice bounds.");return list(std::vector<Value>(v.begin()+from,v.begin()+to));},true}
 };
 if(name=="matrix"||name=="linear")return {
  {"transpose",{l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p);std::vector<std::vector<double>> r(x[0].size(),std::vector<double>(x.size()));for(std::size_t y=0;y<x.size();++y)for(std::size_t z=0;z<x[0].size();++z)r[z][y]=x[y][z];return matrix_value(r);},true},
  {"multiply",{l,l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p),y=matrix_of(a[1],p);if(x[0].size()!=y.size())throw Error(p,"Matrix dimensions do not match.");std::vector<std::vector<double>> r(x.size(),std::vector<double>(y[0].size()));for(std::size_t row=0;row<x.size();++row)for(std::size_t col=0;col<y[0].size();++col)for(std::size_t k=0;k<y.size();++k)r[row][col]+=x[row][k]*y[k][col];return matrix_value(r);},true},
  {"dot",{l,l},n,[](const Args&a,SourcePos p){auto x=vector_of(a[0],p),y=vector_of(a[1],p);if(x.size()!=y.size())throw Error(p,"Vector dimensions do not match.");return Value(std::inner_product(x.begin(),x.end(),y.begin(),0.0));},true}
 };
 if(name=="probability")return {
  {"factorial",{i},i,[](const Args&a,SourcePos p){auto v=integer(a[0],p);if(v<0||v>20)throw Error(p,"Factorial needs an Int from 0 to 20.");std::int64_t out=1;for(std::int64_t j=2;j<=v;++j)out*=j;return Value(out);},true},
  {"choose",{i,i},i,[](const Args&a,SourcePos p){auto v=integer(a[0],p),k=integer(a[1],p);if(v<0||k<0||k>v||v>66)throw Error(p,"Choose requires 0 <= k <= n <= 66.");k=std::min(k,v-k);std::int64_t out=1;for(std::int64_t j=1;j<=k;++j){auto top=v-k+j, bottom=j;auto common=std::gcd(top,bottom);top/=common;bottom/=common;common=std::gcd(out,bottom);out/=common;bottom/=common;if(bottom!=1||out>INT64_MAX/top)throw Error(p,"Choose result overflows Int.");out*=top;}return Value(out);},true}
 };
 return {};
}
}
bool is_expansion_builtin(const std::string& name){static const std::set<std::string> names={"url","encoding","dotenv","config","array","series","matrix","linear","probability"};return names.contains(name);}
TypeInfo expansion_builtin_type(const std::string& name){TypeInfo module(TypeKind::Module);module.name=name;for(auto& e:entries(name)){TypeInfo function(TypeKind::Function);auto sig=std::make_shared<FunctionSig>();sig->params=e.params;sig->result=e.result;sig->fallible=e.fallible;function.callable=sig;module.members[e.name]=function;}return module;}
std::shared_ptr<ModuleData> expansion_builtin_module(const std::string& name,Interpreter&){auto module=std::make_shared<ModuleData>();module->name=name;for(auto& e:entries(name)){auto fn=std::make_shared<CallableData>();fn->name=name+"."+e.name;fn->min_args=fn->max_args=e.params.size();fn->call=e.op;module->exports[e.name]=Value(fn);}return module;}
}
