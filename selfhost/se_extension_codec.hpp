#pragma once
// Independent C++20 implementations of SE's url and encoding modules.
// Included after SEMap is defined by se_bootstrap_runtime.hpp.
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

inline std::string se_url_encode(const std::string& input) {
  constexpr char digits[] = "0123456789ABCDEF";
  std::string output;
  for (unsigned char byte : input) {
    if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
        (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' ||
        byte == '.' || byte == '~') {
      output += static_cast<char>(byte);
    } else {
      output += '%';
      output += digits[byte >> 4];
      output += digits[byte & 15];
    }
  }
  return output;
}
inline int se_codec_hex_digit(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
inline std::string se_url_decode(const std::string& input) {
  std::string output;
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '%') {
      if (i + 2 >= input.size()) throw std::runtime_error("Invalid percent encoding.");
      const int hi = se_codec_hex_digit(static_cast<unsigned char>(input[i + 1]));
      const int lo = se_codec_hex_digit(static_cast<unsigned char>(input[i + 2]));
      if (hi < 0 || lo < 0) throw std::runtime_error("Invalid percent encoding.");
      output += static_cast<char>((hi << 4) | lo);
      i += 2;
    } else {
      output += input[i];
    }
  }
  return output;
}
inline std::string se_url_query(const SEMap<std::string>& params) {
  std::string result;
  for (const auto& item : params.items) {
    if (!result.empty()) result += '&';
    result += se_url_encode(item.first);
    result += '=';
    result += se_url_encode(item.second);
  }
  return result;
}
inline SEMap<std::string> se_url_parse_query(std::string input) {
  SEMap<std::string> output;
  if (!input.empty() && input.front() == '?') input.erase(input.begin());
  std::size_t cursor = 0;
  while (cursor < input.size()) {
    const auto end = input.find('&', cursor);
    const auto pair = input.substr(cursor,
        end == std::string::npos ? std::string::npos : end - cursor);
    if (!pair.empty()) {
      const auto separator = pair.find('=');
      auto key = se_url_decode(pair.substr(0, separator));
      auto value = separator == std::string::npos
          ? std::string{} : se_url_decode(pair.substr(separator + 1));
      for (auto& c : key) if (c == '+') c = ' ';
      for (auto& c : value) if (c == '+') c = ' ';
      output.put(key, value);
    }
    if (end == std::string::npos) break;
    cursor = end + 1;
  }
  return output;
}
inline std::string se_encoding_hex(const std::string& input) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(input.size() * 2);
  for (unsigned char c : input) {
    result += digits[c >> 4];
    result += digits[c & 15];
  }
  return result;
}
inline std::string se_encoding_unhex(const std::string& input) {
  if (input.size() % 2) throw std::runtime_error("Hex needs an even number of digits.");
  std::string output;
  output.reserve(input.size() / 2);
  for (std::size_t i = 0; i < input.size(); i += 2) {
    const int hi = se_codec_hex_digit(static_cast<unsigned char>(input[i]));
    const int lo = se_codec_hex_digit(static_cast<unsigned char>(input[i + 1]));
    if (hi < 0 || lo < 0) throw std::runtime_error("Invalid hex digit.");
    output += static_cast<char>((hi << 4) | lo);
  }
  return output;
}
inline bool se_encoding_utf8_valid(const std::string& input) {
  for (std::size_t i = 0; i < input.size();) {
    const auto c = static_cast<unsigned char>(input[i]);
    const std::size_t length =
        c < 0x80 ? 1 : c >= 0xC2 && c <= 0xDF ? 2 :
        c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 0;
    if (!length || length > input.size() - i) return false;
    if (length > 1) {
      for (std::size_t k = 1; k < length; ++k)
        if ((static_cast<unsigned char>(input[i + k]) & 0xC0) != 0x80) return false;
      const auto next = static_cast<unsigned char>(input[i + 1]);
      if ((c == 0xE0 && next < 0xA0) || (c == 0xED && next >= 0xA0) ||
          (c == 0xF0 && next < 0x90) || (c == 0xF4 && next >= 0x90)) return false;
    }
    i += length;
  }
  return true;
}
