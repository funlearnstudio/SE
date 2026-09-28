#include "s/expansion.hpp"
#include "s/error.hpp"
#include "s/platform.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <cstdint>
#include <complex>
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
std::string html_escape(const std::string& s){std::string out;for(char c:s){switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;case '\'':out+="&#39;";break;default:out+=c;}}return out;}
std::string js_literal(const std::string& s){std::ostringstream out;out<<'"'<<std::hex<<std::setfill('0');for(unsigned char c:s){if(c=='"'||c=='\\')out<<'\\'<<c;else if(c<32||c=='<'||c=='>'||c=='&')out<<"\\u"<<std::setw(4)<<static_cast<unsigned>(c);else out<<c;}return out.str()+'"';}
std::filesystem::path safe_file(const std::string& root,const std::string& path,SourcePos p){namespace fs=std::filesystem;auto base=fs::weakly_canonical(fs::path(root));auto raw=fs::path(path);if(raw.empty()||raw.is_absolute())throw Error(p,"Expected a relative file path.");auto file=fs::weakly_canonical(base/raw);auto relative=file.lexically_relative(base);if(relative.empty()||relative=="."||*relative.begin()=="..")throw Error(p,"Path escapes the root directory.");return file;}
bool is_web_package(const std::string& name){return name=="http_server"||name=="router";}
const std::set<std::string>& web_package_members(const std::string& name){
 static const std::map<std::string,std::set<std::string>> members={
  {"http_server",{"get","post","put","patch","delete","listen","text","json","response","method","path","query","body","header","param","handle","route_count"}},
  {"router",{"get","post","put","delete","path","param","handle","handle_status","route_count"}}
 };
 return members.at(name);
}
bool is_game_package(const std::string& name){static const std::set<std::string> names={"gui","window","canvas","input","sprite","physics","sound","keyboard","mouse","animation","scene","collision","image","audio"};return names.contains(name);}
const std::set<std::string>& game_package_members(const std::string& name){
 static const std::map<std::string,std::set<std::string>> members={
  {"gui",{"new","rect","circle","text","show","save","html"}},
  {"window",{"new","fullscreen","show"}},
  {"canvas",{"new","background","clear","rect","circle","line","text","show","save","html"}},
  {"input",{"key_move","follow_mouse"}},
  {"sprite",{"image","sprite","sprite_color","position","move","velocity","animate"}},
  {"physics",{"velocity","rect_hit","circle_hit","distance","vector","particles","camera"}},
  {"sound",{"sound","play","stop"}},
  {"keyboard",{"key_move"}},
  {"mouse",{"follow_mouse","camera"}},
  {"animation",{"animate","move","velocity"}},
  {"scene",{"new","background","clear","html","save","show"}},
  {"collision",{"rect_hit","circle_hit","distance","vector"}},
  {"image",{"image","sprite"}},
  {"audio",{"sound","play","stop"}}
 };
 return members.at(name);
}
std::vector<Entry> entries(const std::string& name){
 const auto txt=t(TypeKind::Text), n=t(TypeKind::Num), i=t(TypeKind::Int), l=list_t(), m=map_t(), b=t(TypeKind::Bool), unknown=TypeInfo{};
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
 if(name=="config")return {
  {"parse",{txt},m,[](const Args&a,SourcePos p){std::istringstream in(str(a[0],p));std::vector<std::pair<std::string,Value>> out;std::string section,line;while(std::getline(in,line)){line=trim(line);if(line.empty()||line[0]=='#'||line[0]==';')continue;if(line.front()=='['&&line.back()==']'){section=trim(line.substr(1,line.size()-2));if(section.empty())throw Error(p,"Config section name cannot be empty.");continue;}auto pos=line.find('=');if(pos==std::string::npos)throw Error(p,"Expected key = value in config.");auto key=trim(line.substr(0,pos)),value=trim(line.substr(pos+1));if(key.empty())throw Error(p,"Config key cannot be empty.");if(value.size()>=2&&((value.front()=='"'&&value.back()=='"')||(value.front()=='\''&&value.back()=='\'')))value=value.substr(1,value.size()-2);out.emplace_back(section.empty()?key:section+"."+key,Value(value));}return map(std::move(out));},true},
  {"get",{m,txt},txt,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"config.get needs a Map.");auto key=str(a[1],p);for(auto& [k,v]:(*q)->items)if(k==key)return v;throw Error(p,"Missing config key: "+key);},true},
  {"get_int",{m,txt},i,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"config.get_int needs a Map.");auto key=str(a[1],p);for(auto& [k,v]:(*q)->items)if(k==key){try{std::size_t used=0;auto raw=str(v,p);auto n=std::stoll(raw,&used);if(used!=raw.size())throw std::runtime_error("trailing");return Value(static_cast<std::int64_t>(n));}catch(...){throw Error(p,"Config value is not an Int: "+key);}}throw Error(p,"Missing config key: "+key);},true},
  {"get_bool",{m,txt},b,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"config.get_bool needs a Map.");auto key=str(a[1],p);for(auto& [k,v]:(*q)->items)if(k==key){auto x=str(v,p);std::transform(x.begin(),x.end(),x.begin(),[](unsigned char ch){return static_cast<char>(std::tolower(ch));});if(x=="true"||x=="yes"||x=="1")return Value(true);if(x=="false"||x=="no"||x=="0")return Value(false);throw Error(p,"Config value is not a Bool: "+key);}throw Error(p,"Missing config key: "+key);},true},
  {"section",{m,txt},m,[](const Args&a,SourcePos p){auto q=std::get_if<std::shared_ptr<MapData>>(&a[0].data());if(!q)throw Error(p,"config.section needs a Map.");auto prefix=str(a[1],p)+".";std::vector<std::pair<std::string,Value>> out;for(auto& [k,v]:(*q)->items)if(k.rfind(prefix,0)==0)out.emplace_back(k.substr(prefix.size()),v);return map(std::move(out));}},
  {"merge",{m,m},m,[](const Args&a,SourcePos p){auto x=std::get_if<std::shared_ptr<MapData>>(&a[0].data()),y=std::get_if<std::shared_ptr<MapData>>(&a[1].data());if(!x||!y)throw Error(p,"config.merge needs two Maps.");auto out=(*x)->items;for(auto& kv:(*y)->items){auto it=std::find_if(out.begin(),out.end(),[&](const auto& item){return item.first==kv.first;});if(it==out.end())out.push_back(kv);else it->second=kv.second;}return map(std::move(out));}}
 };
 if(name=="series")return {
  {"sum",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);return Value(std::accumulate(v.begin(),v.end(),0.0));}},
  {"mean",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);if(v.empty())throw Error(p,"Mean needs values.");return Value(std::accumulate(v.begin(),v.end(),0.0)/v.size());},true},
  {"min",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);return Value(*std::min_element(v.begin(),v.end()));}},
  {"max",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);return Value(*std::max_element(v.begin(),v.end()));}},
  {"diff",{l},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);std::vector<Value> out;for(std::size_t k=1;k<v.size();++k)out.emplace_back(v[k]-v[k-1]);return list(std::move(out));}},
  {"lag",{l,i},l,[](const Args&a,SourcePos p){auto& v=items(a[0],p);auto k=integer(a[1],p);if(k<0)throw Error(p,"series.lag needs a non-negative lag.");std::vector<Value> out;for(std::size_t j=static_cast<std::size_t>(k);j<v.size();++j)out.push_back(v[j-static_cast<std::size_t>(k)]);return list(std::move(out));},true},
  {"moving_average",{l,i},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);auto w=integer(a[1],p);if(w<=0||static_cast<std::size_t>(w)>v.size())throw Error(p,"Window must be between 1 and series length.");std::vector<Value> out;double sum=std::accumulate(v.begin(),v.begin()+w,0.0);out.emplace_back(sum/w);for(std::size_t j=static_cast<std::size_t>(w);j<v.size();++j){sum+=v[j]-v[j-static_cast<std::size_t>(w)];out.emplace_back(sum/w);}return list(std::move(out));},true},
  {"cumulative_sum",{l},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);std::vector<Value> out;double sum=0;for(auto x:v){sum+=x;out.emplace_back(sum);}return list(std::move(out));}},
  {"returns",{l},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);std::vector<Value> out;for(std::size_t j=1;j<v.size();++j){if(v[j-1]==0)throw Error(p,"Cannot calculate return from a zero value.");out.emplace_back(v[j]/v[j-1]-1);}return list(std::move(out));},true},
  {"normalize",{l},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);if(v.empty())return list({});auto lo=*std::min_element(v.begin(),v.end()),hi=*std::max_element(v.begin(),v.end());if(lo==hi)throw Error(p,"Cannot normalize a constant series.");std::vector<Value> out;for(auto x:v)out.emplace_back((x-lo)/(hi-lo));return list(std::move(out));},true}
 };
 if(name=="linear")return {
  {"transpose",{l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p);std::vector<std::vector<double>> r(x[0].size(),std::vector<double>(x.size()));for(std::size_t y=0;y<x.size();++y)for(std::size_t z=0;z<x[0].size();++z)r[z][y]=x[y][z];return matrix_value(r);},true},
  {"multiply",{l,l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p),y=matrix_of(a[1],p);if(x[0].size()!=y.size())throw Error(p,"Matrix dimensions do not match.");std::vector<std::vector<double>> r(x.size(),std::vector<double>(y[0].size()));for(std::size_t row=0;row<x.size();++row)for(std::size_t col=0;col<y[0].size();++col)for(std::size_t k=0;k<y.size();++k)r[row][col]+=x[row][k]*y[k][col];return matrix_value(r);},true},
  {"dot",{l,l},n,[](const Args&a,SourcePos p){auto x=vector_of(a[0],p),y=vector_of(a[1],p);if(x.size()!=y.size())throw Error(p,"Vector dimensions do not match.");return Value(std::inner_product(x.begin(),x.end(),y.begin(),0.0));},true},
  {"add",{l,l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p),y=matrix_of(a[1],p);if(x.size()!=y.size()||x[0].size()!=y[0].size())throw Error(p,"Matrix dimensions do not match.");for(std::size_t r=0;r<x.size();++r)for(std::size_t q=0;q<x[0].size();++q)x[r][q]+=y[r][q];return matrix_value(x);},true},
  {"subtract",{l,l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p),y=matrix_of(a[1],p);if(x.size()!=y.size()||x[0].size()!=y[0].size())throw Error(p,"Matrix dimensions do not match.");for(std::size_t r=0;r<x.size();++r)for(std::size_t q=0;q<x[0].size();++q)x[r][q]-=y[r][q];return matrix_value(x);},true},
  {"scale",{l,n},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p);auto factor=num(a[1],p);for(auto& row:x)for(auto& v:row)v*=factor;return matrix_value(x);}},
  {"identity",{i},l,[](const Args&a,SourcePos p){auto n=integer(a[0],p);if(n<=0||n>128)throw Error(p,"linear.identity size must be 1..128.");std::vector<std::vector<double>> x(static_cast<std::size_t>(n),std::vector<double>(static_cast<std::size_t>(n)));for(std::size_t j=0;j<x.size();++j)x[j][j]=1;return matrix_value(x);},true},
  {"determinant",{l},n,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p);if(x.size()!=x[0].size())throw Error(p,"Determinant needs a square matrix.");double det=1;for(std::size_t c=0;c<x.size();++c){std::size_t pivot=c;for(std::size_t r=c+1;r<x.size();++r)if(std::fabs(x[r][c])>std::fabs(x[pivot][c]))pivot=r;if(std::fabs(x[pivot][c])<1e-12)return Value(0.0);if(pivot!=c){std::swap(x[pivot],x[c]);det=-det;}auto d=x[c][c];det*=d;for(std::size_t r=c+1;r<x.size();++r){auto factor=x[r][c]/d;for(std::size_t k=c+1;k<x.size();++k)x[r][k]-=factor*x[c][k];}}return Value(det);},true},
  {"inverse",{l},l,[](const Args&a,SourcePos p){auto x=matrix_of(a[0],p);if(x.size()!=x[0].size())throw Error(p,"Inverse needs a square matrix.");auto n=x.size();std::vector<std::vector<double>> y(n,std::vector<double>(n));for(std::size_t j=0;j<n;++j)y[j][j]=1;for(std::size_t c=0;c<n;++c){std::size_t pivot=c;for(std::size_t r=c+1;r<n;++r)if(std::fabs(x[r][c])>std::fabs(x[pivot][c]))pivot=r;if(std::fabs(x[pivot][c])<1e-12)throw Error(p,"Matrix is singular.");std::swap(x[pivot],x[c]);std::swap(y[pivot],y[c]);auto div=x[c][c];for(std::size_t k=0;k<n;++k){x[c][k]/=div;y[c][k]/=div;}for(std::size_t r=0;r<n;++r)if(r!=c){auto factor=x[r][c];for(std::size_t k=0;k<n;++k){x[r][k]-=factor*x[c][k];y[r][k]-=factor*y[c][k];}}}return matrix_value(y);},true},
  {"norm",{l},n,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);double q=0;for(auto x:v)q+=x*x;return Value(std::sqrt(q));}},
  {"normalize",{l},l,[](const Args&a,SourcePos p){auto v=vector_of(a[0],p);double q=0;for(auto x:v)q+=x*x;q=std::sqrt(q);if(q==0)throw Error(p,"Cannot normalize the zero vector.");std::vector<Value> out;for(auto x:v)out.emplace_back(x/q);return list(std::move(out));},true}
 };
 if(name=="dataset")return {
  {"row_count",{l},i,[](const Args&a,SourcePos p){return Value(static_cast<std::int64_t>(items(a[0],p).size()));}},
  {"select",{l,l},l,[](const Args&a,SourcePos p){std::vector<Value> out;for(auto& row:items(a[0],p)){auto rec=std::get_if<std::shared_ptr<MapData>>(&row.data());if(!rec)throw Error(p,"Dataset rows must be Maps.");std::vector<std::pair<std::string,Value>> fields;for(auto& field:items(a[1],p)){auto key=str(field,p);auto it=std::find_if((*rec)->items.begin(),(*rec)->items.end(),[&](const auto& kv){return kv.first==key;});if(it==(*rec)->items.end())throw Error(p,"Missing dataset column: "+key);fields.push_back(*it);}out.push_back(map(std::move(fields)));}return list(std::move(out));},true},
  {"describe",{l},m,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);std::vector<Value> cols;if(!rows.empty()){auto rec=std::get_if<std::shared_ptr<MapData>>(&rows[0].data());if(!rec)throw Error(p,"Dataset rows must be Maps.");for(auto&kv:(*rec)->items)cols.emplace_back(kv.first);}return map({{"row_count",Value(static_cast<std::int64_t>(rows.size()))},{"columns",list(std::move(cols))}});}},
  {"train_test_split",{l,n},l,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);auto ratio=num(a[1],p);if(ratio<=0||ratio>=1)throw Error(p,"Split ratio must be between 0 and 1.");auto cut=static_cast<std::size_t>(rows.size()*ratio);return list({list(std::vector<Value>(rows.begin(),rows.begin()+static_cast<std::ptrdiff_t>(cut))),list(std::vector<Value>(rows.begin()+static_cast<std::ptrdiff_t>(cut),rows.end()))});},true},
  {"columns",{l},l,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);std::vector<Value> out;if(rows.empty())return list(out);auto row=std::get_if<std::shared_ptr<MapData>>(&rows[0].data());if(!row)throw Error(p,"Dataset rows must be Maps.");for(auto& kv:(*row)->items)out.emplace_back(kv.first);return list(std::move(out));},true},
  {"filter_eq",{l,txt,unknown},l,[](const Args&a,SourcePos p){auto key=str(a[1],p);std::vector<Value> out;for(auto& row:items(a[0],p)){auto rec=std::get_if<std::shared_ptr<MapData>>(&row.data());if(!rec)throw Error(p,"Dataset rows must be Maps.");for(auto& kv:(*rec)->items)if(kv.first==key&&kv.second.text()==a[2].text())out.push_back(row);}return list(std::move(out));},true},
  {"unique",{l,txt},l,[](const Args&a,SourcePos p){auto key=str(a[1],p);std::vector<Value> out;std::set<std::string> seen;for(auto& row:items(a[0],p)){auto rec=std::get_if<std::shared_ptr<MapData>>(&row.data());if(!rec)throw Error(p,"Dataset rows must be Maps.");bool kept=false;for(auto& kv:(*rec)->items)if(kv.first==key){auto v=kv.second.text();if(seen.insert(v).second)out.push_back(row);kept=true;break;}if(!kept)throw Error(p,"Missing dataset column: "+key);}return list(std::move(out));},true},
  {"split",{l,n},l,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);auto ratio=num(a[1],p);if(ratio<=0||ratio>=1)throw Error(p,"Split ratio must be between 0 and 1.");std::size_t cut=static_cast<std::size_t>(rows.size()*ratio);std::vector<Value> first(rows.begin(),rows.begin()+static_cast<std::ptrdiff_t>(cut)),second(rows.begin()+static_cast<std::ptrdiff_t>(cut),rows.end());return list({list(std::move(first)),list(std::move(second))});},true}
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
 if(name=="fraction")return {
  {"make",{i,i},l,[](const Args&a,SourcePos p){auto top=integer(a[0],p),bottom=integer(a[1],p);if(bottom==0)throw Error(p,"Fraction denominator cannot be zero.");if(top==INT64_MIN||bottom==INT64_MIN)throw Error(p,"Fraction exceeds supported range.");if(bottom<0){top=-top;bottom=-bottom;}auto d=std::gcd(top,bottom);return list({Value(top/d),Value(bottom/d)});},true},
  {"decimal",{l},n,[](const Args&a,SourcePos p){auto& v=items(a[0],p);if(v.size()!=2||integer(v[1],p)==0)throw Error(p,"Expected [numerator, nonzero denominator].");return Value(static_cast<double>(integer(v[0],p))/static_cast<double>(integer(v[1],p)));},true}
 };
 if(name=="complex")return {
  {"make",{n,n},l,[](const Args&a,SourcePos p){return list({Value(num(a[0],p)),Value(num(a[1],p))});}},
  {"add",{l,l},l,[](const Args&a,SourcePos p){auto x=vector_of(a[0],p),y=vector_of(a[1],p);if(x.size()!=2||y.size()!=2)throw Error(p,"Complex values need [real, imaginary].");return list({Value(x[0]+y[0]),Value(x[1]+y[1])});},true},
  {"multiply",{l,l},l,[](const Args&a,SourcePos p){auto x=vector_of(a[0],p),y=vector_of(a[1],p);if(x.size()!=2||y.size()!=2)throw Error(p,"Complex values need [real, imaginary].");auto z=std::complex<double>(x[0],x[1])*std::complex<double>(y[0],y[1]);return list({Value(z.real()),Value(z.imag())});},true},
  {"magnitude",{l},n,[](const Args&a,SourcePos p){auto x=vector_of(a[0],p);if(x.size()!=2)throw Error(p,"Complex values need [real, imaginary].");return Value(std::hypot(x[0],x[1]));},true}
 };
 if(name=="calculus")return {
  {"polynomial",{l,n},n,[](const Args&a,SourcePos p){auto coefficients=vector_of(a[0],p);auto x=num(a[1],p),out=0.0;for(auto it=coefficients.rbegin();it!=coefficients.rend();++it)out=out*x+*it;return Value(out);}},
  {"derivative",{l,n},n,[](const Args&a,SourcePos p){auto c=vector_of(a[0],p);auto x=num(a[1],p),out=0.0;for(std::size_t k=c.size();k>1;--k)out=out*x+static_cast<double>(k-1)*c[k-1];return Value(out);}},
  {"integral",{l,n,n},n,[](const Args&a,SourcePos p){auto c=vector_of(a[0],p);auto start=num(a[1],p),end=num(a[2],p);double out=0;for(std::size_t k=0;k<c.size();++k)out+=c[k]*(std::pow(end,static_cast<double>(k+1))-std::pow(start,static_cast<double>(k+1)))/static_cast<double>(k+1);return Value(out);}}
 };
 if(name=="units")return {
  {"convert",{n,txt,txt},n,[](const Args&a,SourcePos p){static const std::map<std::string,std::pair<std::string,double>> scale={{"m",{"length",1}},{"km",{"length",1000}},{"cm",{"length",0.01}},{"mm",{"length",0.001}},{"s",{"time",1}},{"min",{"time",60}},{"h",{"time",3600}},{"kg",{"mass",1}},{"g",{"mass",0.001}},{"lb",{"mass",0.45359237}}};auto from=scale.find(str(a[1],p)),to=scale.find(str(a[2],p));if(from==scale.end()||to==scale.end()||from->second.first!=to->second.first)throw Error(p,"Unknown or incompatible units.");return Value(num(a[0],p)*from->second.second/to->second.second);},true},
  {"celsius_to_fahrenheit",{n},n,[](const Args&a,SourcePos p){return Value(num(a[0],p)*1.8+32);}},
  {"fahrenheit_to_celsius",{n},n,[](const Args&a,SourcePos p){return Value((num(a[0],p)-32)/1.8);}}
 };
 if(name=="table"||name=="dataset")return {
  {"column",{l,txt},l,[](const Args&a,SourcePos p){auto key=str(a[1],p);std::vector<Value> out;for(auto& row:items(a[0],p)){auto q=std::get_if<std::shared_ptr<MapData>>(&row.data());if(!q)throw Error(p,"Table rows must be Maps.");auto found=std::find_if((*q)->items.begin(),(*q)->items.end(),[&](const auto& entry){return entry.first==key;});if(found==(*q)->items.end())throw Error(p,"Missing table column: "+key);out.push_back(found->second);}return list(std::move(out));},true},
  {"row_count",{l},i,[](const Args&a,SourcePos p){return Value(static_cast<std::int64_t>(items(a[0],p).size()));}},
  {"select",{l,l},l,[](const Args&a,SourcePos p){std::vector<Value> result;for(auto& row:items(a[0],p)){auto record=std::get_if<std::shared_ptr<MapData>>(&row.data());if(!record)throw Error(p,"Table rows must be Maps.");std::vector<std::pair<std::string,Value>> selected;for(auto& field:items(a[1],p)){auto key=str(field,p);auto it=std::find_if((*record)->items.begin(),(*record)->items.end(),[&](const auto& entry){return entry.first==key;});if(it==(*record)->items.end())throw Error(p,"Missing table column: "+key);selected.push_back(*it);}result.push_back(map(std::move(selected)));}return list(std::move(result));},true}
 };
 if(name=="cookie")return {
  {"parse",{txt},m,[](const Args&a,SourcePos p){std::istringstream in(str(a[0],p));std::string part;std::vector<std::pair<std::string,Value>> result;while(std::getline(in,part,';')){auto eq=part.find('=');if(eq==std::string::npos)continue;result.emplace_back(trim(part.substr(0,eq)),Value(trim(part.substr(eq+1))));}return map(std::move(result));}},
  {"set",{txt,txt},txt,[](const Args&a,SourcePos p){auto key=str(a[0],p),value=str(a[1],p);if(key.empty()||key.find_first_of(";=\r\n \t")!=std::string::npos||value.find_first_of(";\r\n")!=std::string::npos)throw Error(p,"Invalid cookie name or value.");return Value(key+"="+value+"; Path=/; HttpOnly; SameSite=Lax");},true}
 };
 if(name=="cors")return {
  {"allow_origin",{txt},m,[](const Args&a,SourcePos p){auto origin=str(a[0],p);if(origin.find_first_of("\r\n")!=std::string::npos)throw Error(p,"Invalid origin.");return map({{"Access-Control-Allow-Origin",Value(origin)},{"Vary",Value("Origin")}});},true},
  {"preflight",{txt,txt},m,[](const Args&a,SourcePos p){auto origin=str(a[0],p),methods=str(a[1],p);if(origin.find_first_of("\r\n")!=std::string::npos||methods.find_first_of("\r\n")!=std::string::npos)throw Error(p,"Invalid CORS header value.");return map({{"Access-Control-Allow-Origin",Value(origin)},{"Access-Control-Allow-Methods",Value(methods)},{"Vary",Value("Origin")}});},true}
 };
 if(name=="template")return {
  {"escape",{txt},txt,[](const Args&a,SourcePos p){return Value(html_escape(str(a[0],p)));}},
  {"render",{txt,m},txt,[](const Args&a,SourcePos p){auto source=str(a[0],p);auto values=std::get_if<std::shared_ptr<MapData>>(&a[1].data());if(!values)throw Error(p,"Template data must be a Map.");std::string out;for(std::size_t pos=0;pos<source.size();){auto left=source.find("{{",pos);if(left==std::string::npos){out+=source.substr(pos);break;}out+=source.substr(pos,left-pos);auto right=source.find("}}",left+2);if(right==std::string::npos)throw Error(p,"Template placeholder is not closed.");auto key=trim(source.substr(left+2,right-left-2));auto it=std::find_if((*values)->items.begin(),(*values)->items.end(),[&](const auto& kv){return kv.first==key;});if(it==(*values)->items.end())throw Error(p,"Missing template value: "+key);out+=html_escape(str(it->second,p));pos=right+2;}return Value(out);},true}
 };
 if(name=="static")return {
  {"mime",{txt},txt,[](const Args&a,SourcePos p){auto path=str(a[0],p);auto dot=path.find_last_of('.');auto ext=dot==std::string::npos?std::string{}:path.substr(dot);static const std::map<std::string,std::string> types={{".html","text/html; charset=utf-8"},{".css","text/css; charset=utf-8"},{".js","text/javascript; charset=utf-8"},{".json","application/json"},{".svg","image/svg+xml"},{".png","image/png"},{".jpg","image/jpeg"},{".txt","text/plain; charset=utf-8"}};auto it=types.find(ext);return Value(it==types.end()?std::string("application/octet-stream"):it->second);}},
  {"read",{txt,txt},txt,[](const Args&a,SourcePos p){auto path=safe_file(str(a[0],p),str(a[1],p),p);std::ifstream in(path,std::ios::binary);if(!in)throw Error(p,"Could not read static file.");return Value(std::string(std::istreambuf_iterator<char>(in),{}));},true}
 };
 if(name=="upload")return {
  {"save",{txt,txt,txt},txt,[](const Args&a,SourcePos p){auto root=str(a[0],p),name=str(a[1],p),body=str(a[2],p);auto path=safe_file(root,name,p);if(!std::filesystem::is_directory(std::filesystem::path(root)))throw Error(p,"Upload directory does not exist.");std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out)throw Error(p,"Could not write upload.");out.write(body.data(),static_cast<std::streamsize>(body.size()));if(!out)throw Error(p,"Could not finish upload.");return Value(path.string());},true}
 };
 if(name=="tilemap")return {
  {"parse",{txt},l,[](const Args&a,SourcePos p){std::istringstream in(str(a[0],p));std::string line;std::vector<Value> out;std::size_t width=0;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(out.empty())width=line.size();if(line.size()!=width)throw Error(p,"Tilemap rows must have equal width.");out.emplace_back(line);}if(out.empty()||width==0)throw Error(p,"Tilemap cannot be empty.");return list(std::move(out));},true},
  {"at",{l,i,i},txt,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);auto x=integer(a[1],p),y=integer(a[2],p);if(y<0||static_cast<std::size_t>(y)>=rows.size())throw Error(p,"Tile coordinate out of range.");auto row=str(rows[static_cast<std::size_t>(y)],p);if(x<0||static_cast<std::size_t>(x)>=row.size())throw Error(p,"Tile coordinate out of range.");return Value(row.substr(static_cast<std::size_t>(x),1));},true},
  {"size",{l},l,[](const Args&a,SourcePos p){auto& rows=items(a[0],p);if(rows.empty())throw Error(p,"Tilemap cannot be empty.");return list({Value(static_cast<std::int64_t>(str(rows[0],p).size())),Value(static_cast<std::int64_t>(rows.size()))});},true}
 };
 return {};
}
}
bool is_expansion_builtin(const std::string& name){static const std::set<std::string> names={"url","encoding","dotenv","config","array","series","matrix","linear","probability","fraction","complex","calculus","units","table","dataset","cookie","cors","http_server","router","dns","physics","collision","template","static","upload","tilemap","gui","window","canvas","input","sprite","sound","keyboard","mouse","animation","scene","image","audio","video","camera"};return names.contains(name);}
TypeInfo expansion_builtin_type(const std::string& name){
 if(is_web_package(name)){auto type=platform_builtin_type("web");const auto& allowed=web_package_members(name);for(auto it=type.members.begin();it!=type.members.end();)if(!allowed.contains(it->first))it=type.members.erase(it);else ++it;type.name=name;return type;}
 if(name=="dns")return ecosystem_builtin_type("dns");
 if(is_game_package(name)){auto type=ecosystem_builtin_type("game");extend_game_type(type);const auto& allowed=game_package_members(name);for(auto it=type.members.begin();it!=type.members.end();)if(!allowed.contains(it->first))it=type.members.erase(it);else ++it;type.name=name;return type;}
 if(name=="video"||name=="camera"){
  TypeInfo module(TypeKind::Module),int_t(TypeKind::Int),text_t(TypeKind::Text),num_t(TypeKind::Num),none(TypeKind::None);module.name=name;
  auto signature=[&](std::vector<TypeInfo> params){TypeInfo f(TypeKind::Function);auto sig=std::make_shared<FunctionSig>();sig->params=std::move(params);sig->result=none;f.callable=sig;return f;};
  module.members[name=="video"?"add":"start"]=name=="video"?signature({int_t,text_t,num_t,num_t,num_t,num_t}):signature({int_t,num_t,num_t,num_t,num_t});
  module.members["stop"]=signature({int_t});return module;
 }
 TypeInfo module(TypeKind::Module);module.name=name;for(auto& e:entries(name)){TypeInfo function(TypeKind::Function);auto sig=std::make_shared<FunctionSig>();sig->params=e.params;sig->result=e.result;sig->fallible=e.fallible;function.callable=sig;module.members[e.name]=function;}return module;
}
std::shared_ptr<ModuleData> expansion_builtin_module(const std::string& name,Interpreter& vm){
 if(is_web_package(name)){auto module=platform_builtin_module("web",vm);const auto& allowed=web_package_members(name);for(auto it=module->exports.begin();it!=module->exports.end();)if(!allowed.contains(it->first))it=module->exports.erase(it);else ++it;module->name=name;return module;}
 if(name=="dns")return ecosystem_builtin_module("dns",vm);
 if(is_game_package(name)){auto module=ecosystem_builtin_module("game",vm);extend_game_module(module,vm);const auto& allowed=game_package_members(name);for(auto it=module->exports.begin();it!=module->exports.end();)if(!allowed.contains(it->first))it=module->exports.erase(it);else ++it;module->name=name;return module;}
 if(name=="video"||name=="camera"){
  auto module=std::make_shared<ModuleData>();module->name=name;
  auto game=ecosystem_builtin_module("game",vm);auto script_fn=std::get<std::shared_ptr<CallableData>>(game->exports.at("script").data());
  auto add=std::make_shared<CallableData>();add->name=name+(name=="video"?".add":".start");add->min_args=add->max_args=name=="video"?6:5;
  add->call=[name,script_fn](const Args&a,SourcePos p){auto scene=integer(a[0],p);std::size_t pos=name=="video"?2:1;auto x=num(a[pos],p),y=num(a[pos+1],p),w=num(a[pos+2],p),h=num(a[pos+3],p);if(w<=0||h<=0)throw Error(p,"Video dimensions must be positive.");std::ostringstream js;js<<"{const v=document.createElement('video');v.autoplay=true;v.muted=true;v.playsInline=true;";
   if(name=="video")js<<"v.src="<<js_literal(str(a[1],p))<<";v.loop=true;v.play().catch(console.error);SEGame.video=v;";
   else js<<"SEGame.cameraVideo=v;navigator.mediaDevices.getUserMedia({video:true,audio:false}).then(stream=>{v.srcObject=stream;SEGame.cameraStream=stream;v.play();}).catch(e=>{SEGame.cameraVideo=null;console.error(e);});";
   js<<"function frame(){if(v.readyState>=2)ctx.drawImage(v,"<<x<<','<<y<<','<<w<<','<<h<<");if(SEGame."<<(name=="video"?"video":"cameraVideo")<<"===v)requestAnimationFrame(frame);}requestAnimationFrame(frame);}";
   script_fn->call({Value(scene),Value(js.str())},p);return Value{};};module->exports[name=="video"?"add":"start"]=Value(add);
  auto stop=std::make_shared<CallableData>();stop->name=name+".stop";stop->min_args=stop->max_args=1;stop->call=[name,script_fn](const Args&a,SourcePos p){auto scene=integer(a[0],p);std::string js=name=="video"?"if(SEGame.video){SEGame.video.pause();SEGame.video.removeAttribute('src');SEGame.video.load();SEGame.video=null;}":"if(SEGame.cameraStream){SEGame.cameraStream.getTracks().forEach(t=>t.stop());SEGame.cameraStream=null;}if(SEGame.cameraVideo){SEGame.cameraVideo.srcObject=null;SEGame.cameraVideo=null;}";script_fn->call({Value(scene),Value(js)},p);return Value{};};module->exports["stop"]=Value(stop);return module;
 }
 auto module=std::make_shared<ModuleData>();module->name=name;for(auto& e:entries(name)){auto fn=std::make_shared<CallableData>();fn->name=name+"."+e.name;fn->min_args=fn->max_args=e.params.size();fn->call=e.op;module->exports[e.name]=Value(fn);}return module;
}
}
