#pragma once
#include <iomanip>
#include <atomic>

inline std::string se_json_number(double value){
  if(!std::isfinite(value))throw std::runtime_error("JSON cannot encode a non-finite Num.");
  std::ostringstream out;out<<std::setprecision(std::numeric_limits<double>::max_digits10)<<value;return out.str();
}
inline std::string se_json_compact(const SEAny& value);
inline std::string se_json_compact(const std::string& value){return se_json_stringify(value);}
inline std::string se_json_compact(const char* value){return se_json_stringify(std::string(value));}
inline std::string se_json_compact(bool value){return value?"true":"false";}
inline std::string se_json_compact(std::int64_t value){return std::to_string(value);}
inline std::string se_json_compact(int value){return std::to_string(value);}
inline std::string se_json_compact(double value){return se_json_number(value);}
inline std::string se_json_compact(const SEAny& value){
  return std::visit([](const auto& item)->std::string{using T=std::decay_t<decltype(item)>;
    if constexpr(std::is_same_v<T,std::monostate>)return "null";
    else if constexpr(std::is_same_v<T,std::int64_t>)return se_json_compact(item);
    else if constexpr(std::is_same_v<T,double>)return se_json_compact(item);
    else if constexpr(std::is_same_v<T,bool>)return se_json_compact(item);
    else if constexpr(std::is_same_v<T,std::string>)return se_json_compact(item);
    else if constexpr(std::is_same_v<T,SEBytes>)throw std::runtime_error("JSON cannot encode Bytes.");
    else {
      if(item->type()=="List"){std::string out="[";for(const auto& child:item->iterate()){if(out.size()>1)out+=",";out+=se_json_compact(child);}return out+"]";}
      if(item->type()=="Map"){std::string out="{";for(const auto& child:item->pairs()){if(out.size()>1)out+=",";out+=se_json_compact(child.first)+":"+se_json_compact(child.second);}return out+"}";}
      if(item->type()=="Set"){std::string out="[";for(const auto& child:item->iterate()){if(out.size()>1)out+=",";out+=se_json_compact(child);}return out+"]";}
      throw std::runtime_error("JSON cannot encode this dynamic value.");
    }
  },value);
}
template<class T> inline std::string se_json_compact(const SEList<T>& values){std::string out="[";for(const auto& value:values){if(out.size()>1)out+=",";out+=se_json_compact(value);}return out+"]";}
template<class T> inline std::string se_json_compact(const SEMap<T>& values){std::string out="{";for(const auto& value:values.items){if(out.size()>1)out+=",";out+=se_json_compact(value.first)+":"+se_json_compact(value.second);}return out+"}";}
template<class T> inline std::string se_json_compact(const SESet<T>& values){std::string out="[";for(const auto& value:values.items){if(out.size()>1)out+=",";out+=se_json_compact(value);}return out+"]";}
inline std::string se_json_pretty_impl(const SEAny& value,int depth);
template<class T> inline std::string se_json_pretty_impl(const SEList<T>& values,int depth){
  if(values.empty())return "[]";std::string out="[\n";std::size_t i=0;
  for(const auto& value:values){out+=std::string(static_cast<std::size_t>(depth+1)*2,' ')+se_json_pretty_impl(value,depth+1);if(++i<values.size())out+=",";out+="\n";}
  return out+std::string(static_cast<std::size_t>(depth)*2,' ')+"]";
}
template<class T> inline std::string se_json_pretty_impl(const SEMap<T>& values,int depth){
  if(values.items.empty())return "{}";std::string out="{\n";std::size_t i=0;
  for(const auto& value:values.items){out+=std::string(static_cast<std::size_t>(depth+1)*2,' ')+se_json_compact(value.first)+": "+se_json_pretty_impl(value.second,depth+1);if(++i<values.items.size())out+=",";out+="\n";}
  return out+std::string(static_cast<std::size_t>(depth)*2,' ') + "}";
}
template<class T> inline std::string se_json_pretty_impl(const SESet<T>& values,int depth){SEList<T> list(values.items.begin(),values.items.end());return se_json_pretty_impl(list,depth);}
inline std::string se_json_pretty_impl(const SEAny& value,int depth){
  if(auto boxed=std::get_if<std::shared_ptr<SEDynamic>>(&value)){
    if((*boxed)->type()=="List"){auto items=(*boxed)->iterate();SEList<SEAny> list(items.begin(),items.end());return se_json_pretty_impl(list,depth);}
    if((*boxed)->type()=="Map"){SEMap<SEAny> map;for(const auto& item:(*boxed)->pairs())map.put(item.first,item.second);return se_json_pretty_impl(map,depth);}
  }
  return se_json_compact(value);
}
template<class T> inline std::string se_json_pretty_impl(const T& value,int){return se_json_compact(value);}
template<class T> inline std::string se_json_pretty(const T& value){return se_json_pretty_impl(value,0);}

class SEJSONValueParser {
  const std::string& source;std::size_t position=0;
  void whitespace(){while(position<source.size()&&(source[position]==' '||source[position]=='\n'||source[position]=='\r'||source[position]=='\t'))++position;}
  unsigned hex4(){if(position+4>source.size())throw std::runtime_error("Invalid JSON Unicode escape.");unsigned value=0;for(int i=0;i<4;++i){char c=source[position++];value<<=4;if(c>='0'&&c<='9')value|=c-'0';else if(c>='a'&&c<='f')value|=c-'a'+10;else if(c>='A'&&c<='F')value|=c-'A'+10;else throw std::runtime_error("Invalid JSON Unicode escape.");}return value;}
  static void append_utf8(std::string& out,unsigned cp){if(cp<=0x7f)out.push_back(static_cast<char>(cp));else if(cp<=0x7ff){out.push_back(static_cast<char>(0xc0|(cp>>6)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}else if(cp<=0xffff){out.push_back(static_cast<char>(0xe0|(cp>>12)));out.push_back(static_cast<char>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}else{out.push_back(static_cast<char>(0xf0|(cp>>18)));out.push_back(static_cast<char>(0x80|((cp>>12)&0x3f)));out.push_back(static_cast<char>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}}
  std::string string_value(){if(position>=source.size()||source[position++]!='"')throw std::runtime_error("Expected JSON string.");std::string out;while(position<source.size()){unsigned char c=static_cast<unsigned char>(source[position++]);if(c=='"')return out;if(c<0x20)throw std::runtime_error("Control character in JSON string.");if(c!='\\'){out.push_back(static_cast<char>(c));continue;}if(position==source.size())break;char e=source[position++];switch(e){case '"':out.push_back('"');break;case '\\':out.push_back('\\');break;case '/':out.push_back('/');break;case 'b':out.push_back('\b');break;case 'f':out.push_back('\f');break;case 'n':out.push_back('\n');break;case 'r':out.push_back('\r');break;case 't':out.push_back('\t');break;case 'u':{unsigned cp=hex4();if(cp>=0xd800&&cp<=0xdbff){if(position+2>source.size()||source[position++]!='\\'||source[position++]!='u')throw std::runtime_error("Missing JSON low surrogate.");unsigned low=hex4();if(low<0xdc00||low>0xdfff)throw std::runtime_error("Invalid JSON low surrogate.");cp=0x10000+((cp-0xd800)<<10)+(low-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)throw std::runtime_error("Unexpected JSON low surrogate.");append_utf8(out,cp);break;}default:throw std::runtime_error("Invalid JSON escape.");}}throw std::runtime_error("Unterminated JSON string.");}
  SEAny value(){whitespace();if(position>=source.size())throw std::runtime_error("Unexpected end of JSON.");char c=source[position];if(c=='"')return SEAny(string_value());if(c=='['){++position;SEList<SEAny> out;whitespace();if(position<source.size()&&source[position]==']'){++position;return SEAny(out);}while(true){out.push_back(value());whitespace();if(position>=source.size())break;char delimiter=source[position++];if(delimiter==']')return SEAny(out);if(delimiter!=',')throw std::runtime_error("Expected comma in JSON array.");}}if(c=='{'){++position;SEMap<SEAny> out;whitespace();if(position<source.size()&&source[position]=='}'){++position;return SEAny(out);}while(true){whitespace();auto key=string_value();whitespace();if(position>=source.size()||source[position++]!=':')throw std::runtime_error("Expected colon in JSON object.");out.put(key,value());whitespace();if(position>=source.size())break;char delimiter=source[position++];if(delimiter=='}')return SEAny(out);if(delimiter!=',')throw std::runtime_error("Expected comma in JSON object.");}}if(source.compare(position,4,"true")==0){position+=4;return SEAny(true);}if(source.compare(position,5,"false")==0){position+=5;return SEAny(false);}if(source.compare(position,4,"null")==0){position+=4;return SEAny{};}auto start=position;if(source[position]=='-')++position;if(position>=source.size())throw std::runtime_error("Invalid JSON number.");if(source[position]=='0')++position;else{if(!std::isdigit(static_cast<unsigned char>(source[position])))throw std::runtime_error("Invalid JSON value.");while(position<source.size()&&std::isdigit(static_cast<unsigned char>(source[position])))++position;}bool floating=false;if(position<source.size()&&source[position]=='.'){floating=true;++position;auto digits=position;while(position<source.size()&&std::isdigit(static_cast<unsigned char>(source[position])))++position;if(digits==position)throw std::runtime_error("Invalid JSON fraction.");}if(position<source.size()&&(source[position]=='e'||source[position]=='E')){floating=true;++position;if(position<source.size()&&(source[position]=='+'||source[position]=='-'))++position;auto digits=position;while(position<source.size()&&std::isdigit(static_cast<unsigned char>(source[position])))++position;if(digits==position)throw std::runtime_error("Invalid JSON exponent.");}auto token=source.substr(start,position-start);try{if(!floating)return SEAny(static_cast<std::int64_t>(std::stoll(token)));return SEAny(std::stod(token));}catch(...){throw std::runtime_error("JSON number is outside the supported range.");}}
public:explicit SEJSONValueParser(const std::string& text):source(text){}SEAny parse(){auto result=value();whitespace();if(position!=source.size())throw std::runtime_error("Trailing data after JSON value.");return result;}
};
inline SEAny se_json_parse_any(const std::string& source){return SEJSONValueParser(source).parse();}
