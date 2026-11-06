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
  SEList<std::int64_t> numbers{1,2,3};
  SEList<std::int64_t> other{4,5};
  assert((se_itertools_chain(numbers,other)==SEList<std::int64_t>{1,2,3,4,5}));
  assert((se_itertools_flatten(SEList<SEList<std::int64_t>>{{1,2},{3}})==numbers));
  assert(se_itertools_chunked(numbers,2).size()==2);
  assert(se_itertools_chunked(numbers,2)[0][1]==2);
  assert((se_itertools_take(numbers,2)==SEList<std::int64_t>{1,2}));
  assert((se_itertools_drop(numbers,2)==SEList<std::int64_t>{3}));
  assert(se_itertools_windows(numbers,2).size()==2);
  assert(se_itertools_pairs(numbers).size()==2);
  assert(se_itertools_cycle(other,3).size()==6);
  assert(se_itertools_unique(SEList<std::int64_t>{1,1,2}).size()==2);
  should_fail([&]{(void)se_itertools_chunked(numbers,0);});
  should_fail([&]{(void)se_itertools_take(numbers,-1);});
  assert(se_glob_match("*.se","abc.se"));
  assert(!se_glob_match("*.se","abc.cpp"));
  assert(se_hashlib_compare("abc","abc"));
  assert(!se_hashlib_compare("abc","abcd"));
  assert(se_hashlib_to_hex("Hi")=="4869");
  assert(se_typing_is(std::int64_t{7},"int"));
  assert(se_typing_cast(std::int64_t{7},"INT")==7);
  should_fail([]{(void)se_typing_cast(std::int64_t{7},"Text");});
  assert(se_datetime_from_timestamp(0)=="1970-01-01T00:00:00Z");
  assert(se_datetime_format(0,"%Y-%m-%d")=="1970-01-01");
  assert(se_datetime_add_seconds(10,5)==15);
  assert((se_iter_range(1,8,2)==SEList<std::int64_t>{1,3,5,7}));
  assert((se_iter_range(5,0,-2)==SEList<std::int64_t>{5,3,1}));
  should_fail([]{(void)se_iter_range(0,12,0);});
  should_fail([]{(void)se_datetime_add_seconds(std::numeric_limits<std::int64_t>::max(),1);});
  const auto colors=se_enum_make(SEList<std::string>{"red","green","blue"});
  assert(colors.size()==3);
  assert(se_enum_name(colors,1)=="green");
  assert(se_enum_value(colors,"blue")==2);
  assert(se_enum_has(colors,"red"));
  assert(!se_enum_has(colors,"none"));
  should_fail([&]{(void)se_enum_name(colors,99);});
  should_fail([&]{(void)se_enum_value(colors,"none");});
  const SEList<std::int64_t> letters{1,2,3};
  const SEList<std::int64_t> digits{7,8};
  assert(se_iter_zip(letters,digits).size()==2);
  assert(se_iter_zip(letters,digits)[0][1]==7);
  assert(se_iter_product(letters,digits).size()==6);
  assert(se_iter_product(letters,digits)[5][1]==8);
  assert(se_iter_permutations(letters,2).size()==6);
  assert(se_iter_permutations(letters,2)[0][0]==1);
  assert(se_iter_combinations(letters,2).size()==3);
  assert(se_iter_combinations(letters,2)[2][0]==2);
  should_fail([&]{(void)se_iter_permutations(letters,4);});
  should_fail([&]{(void)se_iter_combinations(letters,-1);});
  auto indexed=se_iter_enumerate(SEList<std::int64_t>{1,2,3});
  assert(indexed.size()==3);
  assert(std::get<std::int64_t>(indexed[0][0])==0);
  assert(std::get<std::int64_t>(indexed[2][1])==3);
  std::filesystem::remove_all(root);
}
