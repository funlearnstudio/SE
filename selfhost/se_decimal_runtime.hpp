#pragma once
// Matches the production decimal API's long-double Text representation.
inline long double se_decimal_number(const std::string& value){try{std::size_t used=0;auto number=std::stold(value,&used);if(used!=value.size())throw std::runtime_error("trailing input");return number;}catch(const std::exception&){throw std::runtime_error("Expected a decimal Text value.");}}
inline std::string se_decimal_text(long double value,std::int64_t precision=18){std::ostringstream stream;stream<<std::fixed<<std::setprecision(static_cast<int>(std::clamp<std::int64_t>(precision,1,36)))<<value;auto text=stream.str();while(text.size()>1&&text.back()=='0')text.pop_back();if(!text.empty()&&text.back()=='.')text.pop_back();if(text=="-0")text="0";return text;}
inline std::string se_decimal_parse(const std::string& value){return se_decimal_text(se_decimal_number(value));}
inline std::string se_decimal_add(const std::string& a,const std::string& b){return se_decimal_text(se_decimal_number(a)+se_decimal_number(b));}
inline std::string se_decimal_sub(const std::string& a,const std::string& b){return se_decimal_text(se_decimal_number(a)-se_decimal_number(b));}
inline std::string se_decimal_mul(const std::string& a,const std::string& b){return se_decimal_text(se_decimal_number(a)*se_decimal_number(b));}
inline std::string se_decimal_div(const std::string& a,const std::string& b,std::int64_t precision=18){auto denominator=se_decimal_number(b);if(denominator==0)throw std::runtime_error("decimal.div division by zero.");return se_decimal_text(se_decimal_number(a)/denominator,precision);}
inline std::string se_decimal_quantize(const std::string& value,std::int64_t places){if(places<0||places>18)throw std::runtime_error("decimal.quantize places must be 0..18.");std::ostringstream stream;stream<<std::fixed<<std::setprecision(static_cast<int>(places))<<se_decimal_number(value);return stream.str();}
