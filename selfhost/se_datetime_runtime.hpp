#pragma once
inline std::tm se_datetime_utc(std::int64_t timestamp){
  if(!std::in_range<std::time_t>(timestamp))throw std::runtime_error("Timestamp is out of range.");
  auto raw=static_cast<std::time_t>(timestamp);std::tm result{};
#ifdef _WIN32
  if(gmtime_s(&result,&raw)!=0)throw std::runtime_error("Timestamp is out of range.");
#else
  if(!gmtime_r(&raw,&result))throw std::runtime_error("Timestamp is out of range.");
#endif
  return result;
}
inline std::string se_datetime_format(std::int64_t timestamp,const std::string& pattern){auto tm=se_datetime_utc(timestamp);std::ostringstream stream;stream<<std::put_time(&tm,pattern.c_str());if(!stream)throw std::runtime_error("Datetime formatting failed.");return stream.str();}
inline std::int64_t se_datetime_timestamp(){return static_cast<std::int64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));}
inline std::string se_datetime_from_timestamp(std::int64_t timestamp){return se_datetime_format(timestamp,"%Y-%m-%dT%H:%M:%SZ");}
inline std::string se_datetime_now(){return se_datetime_from_timestamp(se_datetime_timestamp());}
inline std::int64_t se_datetime_add_seconds(std::int64_t timestamp,std::int64_t offset){if((offset>0&&timestamp>INT64_MAX-offset)||(offset<0&&timestamp<INT64_MIN-offset))throw std::runtime_error("Datetime timestamp overflow.");return timestamp+offset;}
