// Independently compiled extension API regression tests: no production SE VM.
#include "se_bootstrap_runtime.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>

template<class F> void should_fail(F&& action) {
  bool failed=false;
  try {action();}catch(const std::exception&){failed=true;}
  assert(failed);
}

int main() {
  assert(se_url_encode("a b/+")== "a%20b%2F%2B");
  assert(se_url_decode("a%20b%2F%2B")=="a b/+");
  should_fail([]{(void)se_url_decode("%GG");});
  assert(se_encoding_hex("SE")=="5345");
  assert(se_encoding_unhex("5345")=="SE");
  assert(se_encoding_utf8_valid("SE"));
  assert(!se_encoding_utf8_valid("\xED\xA0\x80"));

  auto dotenv=se_dotenv_parse("export NAME = \"SE\"\nCOUNT=42\n");
  assert(se_dotenv_get(dotenv,"NAME")=="SE");
  assert(se_dotenv_get(dotenv,"COUNT")=="42");
  should_fail([]{(void)se_dotenv_parse("BAD-KEY=a\n");});
  auto config=se_config_parse("[server]\nport=8080\nactive=yes\nname=\"SE\"\n");
  assert(se_config_get_int(config,"server.port")==8080);
  assert(se_config_get_bool(config,"server.active"));
  assert(se_config_get(config,"server.name")=="SE");
  const auto section=se_config_section(config,"server");
  assert(section.at("name")=="SE");
  SEMap<std::string> override_values;
  override_values.put("server.name","new");
  const auto merged=se_config_merge(config,override_values);
  assert(merged.at("server.name")=="new");
  assert(config.at("server.name")=="SE");
  should_fail([&]{(void)se_config_get_int(config,"server.active");});

  auto cookies=se_cookie_parse("x=1; y = two");
  assert(cookies.at("x")=="1");
  assert(cookies.at("y")=="two");
  assert(se_cookie_set("token","ok")=="token=ok; Path=/; HttpOnly; SameSite=Lax");
  should_fail([]{(void)se_cookie_set("x","inject\r\nheader");});
  assert(se_cors_allow_origin("https://example.com").at("Vary")=="Origin");
  assert(se_cors_preflight("https://example.com","GET, POST").at("Access-Control-Allow-Methods")=="GET, POST");
  should_fail([]{(void)se_cors_allow_origin("bad\ninjected");});

  assert(se_template_escape("<&>")=="&lt;&amp;&gt;");
  SEMap<std::string> values;
  values.put("name","<SE>");
  assert(se_template_render("Hello {{ name }}!",values)=="Hello &lt;SE&gt;!");
  should_fail([&]{(void)se_template_render("{{missing}}",values);});
  assert(se_static_mime("site.css")=="text/css; charset=utf-8");
  assert(se_static_mime("unknown.x")=="application/octet-stream");

  auto root=std::filesystem::temp_directory_path()/"se-selfhost-extension-tests";
  std::filesystem::create_directories(root);
  auto path=se_upload_save(root.string(),"sample.txt","Hello, SE");
  assert(se_static_read(root.string(),"sample.txt")=="Hello, SE");
  assert(std::filesystem::is_regular_file(path));
  should_fail([&]{(void)se_upload_save(root.string(),"../outside.txt","BAD");});
  should_fail([&]{(void)se_static_read(root.string(),"../../etc/passwd");});
  std::filesystem::remove_all(root);
}
