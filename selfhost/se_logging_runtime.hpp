#pragma once

inline int& se_log_threshold() { static int level = 1; return level; }
struct SELogRecord { int level; std::string message; };
inline std::vector<SELogRecord>& se_log_storage() { static std::vector<SELogRecord> values; return values; }
inline std::string se_log_level_name(int level) {
  if (level <= 0) return "debug"; if (level == 1) return "info"; if (level == 2) return "warn"; return "error";
}
inline void se_log_level(const std::string& name) {
  auto level = se_lower(name);
  if (level == "debug") se_log_threshold() = 0;
  else if (level == "info") se_log_threshold() = 1;
  else if (level == "warn" || level == "warning") se_log_threshold() = 2;
  else if (level == "error" || level == "critical") se_log_threshold() = 3;
  else throw std::runtime_error("Unknown logging level: " + name);
}
inline void se_log_emit(int level, const std::string& label, const std::string& message) {
  if (level < se_log_threshold()) return;
  static constexpr int max_records=512;
  auto& records=se_log_storage();
  records.push_back(SELogRecord{level,message});
  if(records.size()>max_records)records.erase(records.begin());
  std::clog << "[" << label << "] " << message << '\n';
}
inline void se_log_debug(const std::string& message) { se_log_emit(0, "DEBUG", message); }
inline void se_log_info(const std::string& message) { se_log_emit(1, "INFO", message); }
inline void se_log_warn(const std::string& message) { se_log_emit(2, "WARN", message); }
inline void se_log_error(const std::string& message) { se_log_emit(3, "ERROR", message); }
inline SEAny se_log_records() {
  SEList<SEAny> result;
  for(const auto& record:se_log_storage()){
    SEMap<SEAny> row;
    row.put("level",SEAny(static_cast<std::int64_t>(record.level)));
    row.put("message",SEAny(record.message));
    result.emplace_back(SEAny(row));
  }
  return SEAny(result);
}
