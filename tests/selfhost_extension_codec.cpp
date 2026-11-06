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
  const auto cookies=se_cookie_parse("a=1; b=two ; empty=");
  assert(cookies.at("a")=="1");
  assert(cookies.at("b")=="two");
  assert(cookies.at("empty")=="");
  assert(se_cookie_set("session","ok")=="session=ok; Path=/; HttpOnly; SameSite=Lax");
  assert(rejects([] { (void)se_cookie_set("bad;name","ok"); }));
  assert(rejects([] { (void)se_cookie_set("safe","bad\r\nHeader: value"); }));
  assert(se_cors_allow_origin("https://example.test").at("Vary")=="Origin");
  assert(se_cors_preflight("https://example.test","GET, POST").at(
      "Access-Control-Allow-Methods")=="GET, POST");
  assert(rejects([] { (void)se_cors_allow_origin("x\r\nInjected: yes"); }));
  assert(rejects([] { (void)se_cors_preflight("x","GET\nInjected"); }));
  assert(se_template_escape("<&>'\\\"")=="&lt;&amp;&gt;&#39;&quot;");
  SEMap<std::string> context;
  context.put("name","<SE>");
  assert(se_template_render("Hello {{ name }}!",context)=="Hello &lt;SE&gt;!");
  assert(rejects([&] { (void)se_template_render("{{ missing }}",context); }));
  assert(rejects([&] { (void)se_template_render("{{ name",context); }));
}
