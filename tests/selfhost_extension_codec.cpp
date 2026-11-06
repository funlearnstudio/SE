#include "se_bootstrap_runtime.hpp"
#include <cassert>
#include <stdexcept>
#include <string>

int main() {
  assert(se_url_encode("a b/+") == "a%20b%2F%2B");
  assert(se_url_decode("a%20b%2F%2B") == "a b/+");
  assert(se_url_decode("%E4%B8%AD") == "\xE4\xB8\xAD");
  SEMap<std::string> args;
  args.put("a b", "c+d");
  args.put("percent", "%");
  assert(se_url_query(args) == "a%20b=c%2Bd&percent=%25");
  const auto parsed = se_url_parse_query("?a+b=c%2Bd&percent=%25&empty");
  assert(parsed.at("a b") == "c+d");
  assert(parsed.at("percent") == "%");
  assert(parsed.at("empty") == "");
  assert(se_encoding_hex("SE") == "5345");
  assert(se_encoding_unhex("5345") == "SE");
  assert(se_encoding_hex(std::string("\0\xff", 2)) == "00ff");
  assert(se_encoding_unhex("00FF") == std::string("\0\xff", 2));
  assert(se_encoding_utf8_valid("SE"));
  assert(se_encoding_utf8_valid("\xE4\xB8\xAD"));
  assert(!se_encoding_utf8_valid("\xC0\xAF"));
  assert(!se_encoding_utf8_valid("\xED\xA0\x80"));
  assert(!se_encoding_utf8_valid("\xF4\x90\x80\x80"));
  assert(!se_encoding_utf8_valid("\xE4\xB8"));
  auto rejects = [](auto function) {
    try { function(); }
    catch (const std::runtime_error&) { return true; }
    return false;
  };
  assert(rejects([] { (void)se_url_decode("%XZ"); }));
  assert(rejects([] { (void)se_url_parse_query("a=%ZZ"); }));
  assert(rejects([] { (void)se_encoding_unhex("0"); }));
  assert(rejects([] { (void)se_encoding_unhex("GG"); }));
}
