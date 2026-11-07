#pragma once
#include <mutex>

struct SEDatabaseState {
  std::mutex mutex;
  std::filesystem::path path;
  SEMap<std::string> values;
};
struct SEDatabase {
  std::shared_ptr<SEDatabaseState> state = std::make_shared<SEDatabaseState>();
  std::string __se_type_name() const { return "Database"; }
};
inline void se_db_save_locked(const std::shared_ptr<SEDatabaseState>& state) {
  auto temporary = state->path; temporary += ".se-tmp";
  { std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("db.save could not create a temporary file.");
    for (const auto& pair : state->values.items) out << se_encoding_hex(pair.first) << '\t' << se_encoding_hex(pair.second) << '\n';
    out.close(); if (!out) throw std::runtime_error("db.save could not finish writing."); }
#ifdef _WIN32
  std::error_code error; std::filesystem::remove(state->path, error); error.clear();
  std::filesystem::rename(temporary, state->path, error);
#else
  std::error_code error; std::filesystem::rename(temporary, state->path, error);
#endif
  if (error) { std::filesystem::remove(temporary); throw std::runtime_error("db.save could not replace database: " + error.message()); }
}
inline SEDatabase se_db_open(const std::string& path) {
  if (path.find('\0') != std::string::npos) throw std::runtime_error("Database path contains NUL.");
  SEDatabase db; db.state->path = path;
  std::ifstream in(path, std::ios::binary);
  if (!in) { if (std::filesystem::exists(path)) throw std::runtime_error("db.open could not read the database."); return db; }
  std::string line;
  while (std::getline(in, line)) {
    auto tab = line.find('\t'); if (tab == std::string::npos) throw std::runtime_error("db.open found malformed database data.");
    db.state->values.put(se_encoding_unhex(line.substr(0, tab)), se_encoding_unhex(line.substr(tab + 1)));
  }
  if (in.bad()) throw std::runtime_error("db.open could not read the database.");
  return db;
}
inline void se_db_set(const SEDatabase& db, const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(db.state->mutex); auto next = db.state->values.clone(); next.put(key, value);
  auto old = db.state->values; db.state->values = std::move(next);
  try { se_db_save_locked(db.state); } catch (...) { db.state->values = std::move(old); throw; }
}
inline SEOption se_db_get(const SEDatabase& db, const std::string& key) {
  std::lock_guard<std::mutex> lock(db.state->mutex);
  for (const auto& pair : db.state->values.items) if (pair.first == key) return se_option_some(pair.second);
  return se_option_none();
}
inline bool se_db_has(const SEDatabase& db, const std::string& key) {
  std::lock_guard<std::mutex> lock(db.state->mutex);
  return std::any_of(db.state->values.items.begin(), db.state->values.items.end(), [&](const auto& pair) { return pair.first == key; });
}
inline bool se_db_remove(const SEDatabase& db, const std::string& key) {
  std::lock_guard<std::mutex> lock(db.state->mutex); auto next = db.state->values.clone(); bool found = false;
  for (auto it = next.items.begin(); it != next.items.end();) if (it->first == key) { it = next.items.erase(it); found = true; } else ++it;
  if (!found) return false; auto old = db.state->values; db.state->values = std::move(next);
  try { se_db_save_locked(db.state); } catch (...) { db.state->values = std::move(old); throw; }
  return true;
}
inline SEList<std::string> se_db_keys(const SEDatabase& db) {
  std::lock_guard<std::mutex> lock(db.state->mutex); SEList<std::string> keys;
  for (const auto& pair : db.state->values.items) keys.push_back(pair.first); return keys;
}
inline void se_db_save(const SEDatabase& db) { std::lock_guard<std::mutex> lock(db.state->mutex); se_db_save_locked(db.state); }
inline std::string se_dynamic_type(const SEDatabase&) { return "Database"; }
inline std::ostream& operator<<(std::ostream& out, const SEDatabase&) { return out << "<database>"; }
