#pragma once

struct SENetResponse { std::int64_t status = 0; std::string type; std::string body; };
inline std::string se_net_quote(const std::string& value) {
#ifdef _WIN32
  std::string out = "\""; for (char c : value) { if (c == '\"') out += "\\\""; else out += c; } return out + "\"";
#else
  std::string out = "'"; for (char c : value) { if (c == '\'') out += "'\\''"; else out += c; } return out + "'";
#endif
}
inline SENetResponse se_net_request(const std::string& method, const std::string& url,
                                   const std::string& body, const std::string& content_type) {
  const bool secure = url.rfind("https://", 0) == 0;
  const bool plain = url.rfind("http://", 0) == 0;
  if (!secure && !plain) throw std::runtime_error("Network URL must use http:// or https://.");
  if (url.find('\0') != std::string::npos || body.find('\0') != std::string::npos) throw std::runtime_error("Network request contains NUL.");
  const std::string marker = "__SE_HTTP_META_4B17__";
  std::string command = "curl -sS -L --max-time 30 -X " + se_net_quote(method) +
      " -H " + se_net_quote("Content-Type: " + content_type);
  if (!body.empty() || method == "POST" || method == "PUT" || method == "PATCH") command += " --data-binary " + se_net_quote(body);
  command += " -w " + se_net_quote("\\n" + marker + "%{http_code}|%{content_type}") + " " + se_net_quote(url) + " 2>&1";
  auto output = se_process_output(command);
  auto offset = output.rfind("\n" + marker);
  if (offset == std::string::npos) throw std::runtime_error("Network request failed. Install curl and check the URL.");
  SENetResponse response; response.body = output.substr(0, offset);
  auto metadata = output.substr(offset + marker.size() + 1); auto separator = metadata.find('|');
  try { response.status = std::stoll(metadata.substr(0, separator)); } catch (...) { response.status = 0; }
  if (separator != std::string::npos) response.type = metadata.substr(separator + 1);
  while (!response.type.empty() && (response.type.back() == '\n' || response.type.back() == '\r')) response.type.pop_back();
  if (response.status == 0) throw std::runtime_error("Network request failed. Install curl and check the URL.");
  return response;
}
inline SEMap<SEAny> se_net_response_map(const SENetResponse& response) {
  SEMap<SEAny> result; result.put("status", SEAny(response.status)); result.put("type", SEAny(response.type)); result.put("body", SEAny(response.body)); return result;
}
inline std::string se_net_get(const std::string& url) { return se_net_request("GET", url, "", "text/plain; charset=utf-8").body; }
inline std::string se_net_post(const std::string& url, const std::string& body) { return se_net_request("POST", url, body, "text/plain; charset=utf-8").body; }
inline std::string se_net_post_json(const std::string& url, const std::string& body) { return se_net_request("POST", url, body, "application/json").body; }
inline SEMap<SEAny> se_net_request_api(const std::string& method, const std::string& url, const std::string& body) {
  return se_net_response_map(se_net_request(method, url, body, "text/plain; charset=utf-8"));
}
inline void se_net_download(const std::string& url, const std::string& path) {
  if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) throw std::runtime_error("Network URL must use http:// or https://.");
  auto command = "curl -fL --max-time 60 -o " + se_net_quote(path) + " " + se_net_quote(url);
  if (se_process_run(command) != 0) throw std::runtime_error("net.download failed.");
}
inline void se_net_require_scheme(const std::string& url, const std::string& scheme) {
  if (url.rfind(scheme, 0) != 0) throw std::runtime_error("URL must use " + scheme + ".");
}
inline std::string se_http_get(const std::string& url) { se_net_require_scheme(url, "http://"); return se_net_get(url); }
inline std::string se_http_post(const std::string& url, const std::string& body) { se_net_require_scheme(url, "http://"); return se_net_post(url, body); }
inline std::string se_http_post_json(const std::string& url, const std::string& body) { se_net_require_scheme(url, "http://"); return se_net_post_json(url, body); }
inline SEMap<SEAny> se_http_request(const std::string& method, const std::string& url, const std::string& body) { se_net_require_scheme(url, "http://"); return se_net_request_api(method, url, body); }
inline std::string se_https_get(const std::string& url) { se_net_require_scheme(url, "https://"); return se_net_get(url); }
inline std::string se_https_post(const std::string& url, const std::string& body) { se_net_require_scheme(url, "https://"); return se_net_post(url, body); }
inline std::string se_https_post_json(const std::string& url, const std::string& body) { se_net_require_scheme(url, "https://"); return se_net_post_json(url, body); }
