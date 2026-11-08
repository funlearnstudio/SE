#pragma once

inline std::string se_glob_regex(const std::string& pattern) {
  std::string out = "^";
  for (std::size_t i = 0; i < pattern.size(); ++i) {
    const char c = pattern[i];
    if (c == '*') {
      if (i + 1 < pattern.size() && pattern[i + 1] == '*') { out += ".*"; ++i; }
      else out += "[^/]*";
    } else if (c == '?') out += "[^/]";
    else {
      if (std::string(".^$|()[]{}+\\").find(c) != std::string::npos) out += '\\';
      out += c;
    }
  }
  return out + "$";
}
inline bool se_glob_match(const std::string& pattern, const std::string& text) {
  try { return std::regex_match(text, std::regex(se_glob_regex(pattern))); }
  catch (const std::regex_error& error) { throw std::runtime_error(std::string("glob.match: ") + error.what()); }
}
inline SEList<std::string> se_glob_find(const std::string& pattern) {
  const auto wild = pattern.find_first_of("*?");
  std::filesystem::path root = ".";
  if (wild == std::string::npos) {
    auto path = std::filesystem::path(pattern); root = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
  } else {
    auto prefix = std::filesystem::path(pattern.substr(0, wild)); root = prefix.has_parent_path() ? prefix.parent_path() : std::filesystem::path(".");
  }
  SEList<std::string> matches; std::error_code error;
  if (!std::filesystem::exists(root, error)) return matches;
  const auto matcher = std::regex(se_glob_regex(std::filesystem::path(pattern).generic_string()));
  for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, error), end;
       it != end && !error; it.increment(error)) {
    auto candidate = it->path().generic_string();
    if (std::regex_match(candidate, matcher)) matches.push_back(std::move(candidate));
  }
  if (error) throw std::runtime_error("glob.find: " + error.message());
  return matches;
}
