// Standalone bootstrap runtime tests: intentionally link NO production s_core.
#include "se_bootstrap_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
    // SE automatically invokes zero-parameter callables in value positions.
    auto zero = []() -> std::int64_t { return 42; };
    auto unary = [](std::int64_t n) -> std::int64_t { return n + 1; };
    std::int64_t mutable_value = 9;
    assert(se_implicit_call(zero) == 42);
    assert(se_implicit_call(unary)(5) == 6);
    se_implicit_call(mutable_value) += 1;
    assert(mutable_value == 10);

    auto numbers = se_list(std::int64_t{1}, std::int64_t{2});
    se_add_item(numbers, std::int64_t{3});
    assert(numbers.size() == 3);
    assert(se_index(numbers, std::int64_t{0}) == 1);
    se_index(numbers, std::int64_t{1}) = 9;
    assert(se_index(numbers, std::int64_t{1}) == 9);
    bool caught = false;
    try { (void)se_index(numbers, std::int64_t{-1}); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);
    caught = false;
    try { (void)se_index(numbers, std::int64_t{99}); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);

    auto words = se_split("alpha,beta,gamma", ",");
    assert(words.size() == 3 && words[1] == "beta");
    assert(se_join(words, ":") == "alpha:beta:gamma");
    assert(se_trim("  sample \r\n") == "sample");
    assert(se_upper("abc") == "ABC");
    assert(se_lower("ABC") == "abc");
    assert(se_contains(std::string("abc"), std::string("b")));
    assert(se_repeat("x", 3) == "xxx");
    assert(se_add("name=", std::int64_t{7}) == "name=7");
    assert(se_char(65) == "A");
    assert(se_char(20013) == "\xe4\xb8\xad");
    assert(se_char(std::string("\xe4\xb8\xad")) == "\xe4\xb8\xad");
    caught = false;
    try { (void)se_char(0xd800); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);
    assert(se_bool(std::string(" TRUE \n")));
    assert(!se_bool(std::string("FALSE")));
    assert(se_bool(true));
    caught = false;
    try { (void)se_bool(std::string("yes")); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);
    assert((se_take(numbers, 2) == std::vector<std::int64_t>{1, 9}));
    assert((se_drop(numbers, 2) == std::vector<std::int64_t>{3}));

    auto mapping = se_map(std::pair{std::string("a"), std::int64_t{1}},
                          std::pair{std::string("b"), std::int64_t{2}});
    mapping.put("a", 10);
    assert(mapping.size() == 2);
    se_assign_index(mapping,std::string("new"),std::int64_t{12});
    se_assign_index(mapping,std::string("new"),std::int64_t{13});
    assert(mapping.size()==3 && mapping.items.back().first=="new");
    assert(se_index(mapping,std::string("new"))==13);
    assert(se_index(mapping, std::string("a")) == 10);
    assert(se_contains(mapping, std::string("b")));
    assert(!se_contains(mapping, std::string("missing")));
    caught = false;
    try { (void)se_index(mapping, std::string("missing")); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);

    auto unique = se_set(std::int64_t{1}, std::int64_t{2}, std::int64_t{2});
    assert(unique.size() == 2);
    se_add_item(unique, std::int64_t{3});
    assert(unique.size() == 3);
    assert(se_contains(unique, std::int64_t{2}));
    assert(!se_contains(unique, std::int64_t{4}));

    assert(se_int(std::string("123")) == 123);
    assert(se_num(std::string("2.5")) == 2.5);
    caught = false;
    try { (void)se_int(std::string("12x")); }
    catch (const std::exception&) { caught = true; }
    assert(caught);
    caught = false;
    try { (void)se_num(std::string("nan")); }
    catch (const std::exception&) { caught = true; }
    assert(caught);

    const std::vector<std::string> ast = {
        "Atom", "a\tb", "line\nbreak", "quote\"escape", "\xe4\xb8\xad"
    };
    const auto serialized = se_json_stringify(ast);
    assert(se_json_parse(serialized) == ast);
    assert(se_json_parse(R"(["\u4e2d","\ud83d\ude00"])")[0] == "\xe4\xb8\xad");
    assert(se_json_parse(R"(["\u4e2d","\ud83d\ude00"])")[1] == "\xf0\x9f\x98\x80");
    caught = false;
    try { (void)se_json_parse("[\"unterminated\""); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);

    std::string packed_ast = "nested AST: \xe4\xb8\xad\\\n";
    packed_ast.push_back('\0');
    packed_ast += "tail";
    const auto encoded_ast = se_base64_encode(packed_ast);
    assert(se_base64_decode(encoded_ast) == packed_ast);
    assert(se_base64_encode("") == "");
    assert(se_base64_decode("") == "");
    caught = false;
    try { (void)se_base64_decode("%%%invalid%%%"); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught);

    assert(se_duration("500ms").milliseconds == 500);
    assert(se_duration("2s").milliseconds == 2000);
    assert(se_duration("1min").milliseconds == 60000);
    se_wait(se_duration("0ms"));
    caught=false;
    try{(void)se_duration("7hour");}catch(const std::runtime_error&){caught=true;}
    assert(caught);
    assert(se_random_int(7,7)==7);
    const auto randvalue=se_random_num();
    assert(randvalue>=0.0 && randvalue<=1.0);
    assert(se_path_join("tmp","sample.txt")==std::string("tmp/sample.txt"));
    assert(se_path_name("a/b.txt")=="b.txt");
    assert(se_path_ext("a/b.txt")==".txt");
    assert(se_path_parent("a/b.txt")=="a");
    assert(se_path_is_dir("selfhost"));
    assert(se_path_is_file("selfhost/sample.se"));
    assert(se_path_exists("selfhost/sample.se"));

    const auto temp_root=std::filesystem::temp_directory_path()/"se-bootstrap-runtime-test";
    const auto original=(temp_root/"original.txt").string();
    const auto copied=(temp_root/"copied.txt").string();
    const auto moved=(temp_root/"moved.txt").string();
    se_file_mkdir(temp_root.string());
    se_write(original,"first");
    se_append(original," second");
    assert(se_read(original)=="first second");
    se_file_copy(original,copied);
    assert(se_read(copied)=="first second");
    se_file_move(copied,moved);
    assert(!se_path_exists(copied) && se_read(moved)=="first second");
    se_file_remove(temp_root.string());
    assert(!se_path_exists(temp_root.string()));

    assert((se_range(3, 1) == std::vector<std::int64_t>{3, 2, 1}));
    auto rejects=[](auto operation){bool failed=false;try{operation();}catch(const std::exception&){failed=true;}assert(failed);};
    rejects([]{se_math_sqrt(-1);});
    rejects([]{se_math_clamp(0,2,1);});
    rejects([]{se_math_map_range(1,0,0,0,1);});
    rejects([]{se_math_factorial(21);});
    rejects([]{se_math_comb(67,33);});
    rejects([]{se_math_perm(3,4);});
    rejects([]{se_math_mean(std::vector<double>{});});
    rejects([]{se_statistics_variance(std::vector<double>{1});});
    rejects([]{se_regex_match("[", "text");});
    assert(se_math_comb(66,33)==7219428434016265740LL);
    assert(se_math_min(3,2.5,4)==2.5);
    assert(se_math_hypot(2,3,6)==7);
    std::cout << "SE bootstrap standalone runtime passed\n";
}
