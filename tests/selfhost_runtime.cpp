// Standalone bootstrap runtime tests: intentionally link NO production s_core.
#include "se_bootstrap_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
    SEAny dynamic_numbers=SEList<std::int64_t>{1,2};
    SEAny dynamic_alias=dynamic_numbers;
    se_assign_index(dynamic_alias,SEAny(std::int64_t{0}),SEAny(std::int64_t{9}));
    assert(se_int(se_index(dynamic_numbers,SEAny(std::int64_t{0})))==9);
    assert(dynamic_numbers.size()==2);
    assert(se_dynamic_type(dynamic_numbers)=="List");
    assert(se_dynamic_type(SEList<std::int64_t>{1})=="List");
    assert(se_int(SEAny(std::int64_t{7})+SEAny(std::int64_t{2}))==9);
    assert(se_num(se_operator_div(SEAny(std::int64_t{7}),SEAny(std::int64_t{2})))==3.5);
    assert(se_num(se_operator_mod(7.5,SEAny(std::int64_t{2})))==1.5);
    bool dynamic_bool_rejected=false;
    try{(void)se_divide(SEAny(true),SEAny(std::int64_t{2}));}
    catch(const std::runtime_error&){dynamic_bool_rejected=true;}
    assert(dynamic_bool_rejected);
    assert(se_string(SEAny{})=="none");
    SEAny mixed_values=SEList<SEAny>{SEAny(std::int64_t{1}),SEAny(std::string("SE"))};
    se_add_item(mixed_values,SEAny(true));
    assert(mixed_values.size()==3);
    assert(se_any_cast<bool>(se_index(mixed_values,SEAny(std::int64_t{2}))));
    SEAny nested_map=SEMap<SEList<std::int64_t>>{};
    se_assign_index(nested_map,SEAny(std::string("list")),dynamic_numbers);
    auto nested_alias=se_index(nested_map,SEAny(std::string("list")));
    se_assign_index(nested_alias,SEAny(std::int64_t{1}),SEAny(std::int64_t{8}));
    assert(se_int(se_index(dynamic_numbers,SEAny(std::int64_t{1})))==8);

    assert(se_remainder(std::int64_t{17}, std::int64_t{5}) == 2);
    assert(se_remainder(std::int64_t{-17}, std::int64_t{5}) == -2);
    assert(se_remainder(7.5, std::int64_t{2}) == 1.5);
    assert(se_remainder(std::int64_t{7}, 2.5) == 2.0);
    assert(se_remainder(std::numeric_limits<std::int64_t>::min(), std::int64_t{-1}) == 0);
    for (double divisor : {0.0, -0.0}) {
        bool zero_caught = false;
        try { (void)se_remainder(1.5, divisor); }
        catch (const std::runtime_error&) { zero_caught = true; }
        assert(zero_caught);
    }
    bool integer_zero_caught = false;
    try { (void)se_remainder(std::int64_t{1}, std::int64_t{0}); }
    catch (const std::runtime_error&) { integer_zero_caught = true; }
    assert(integer_zero_caught);

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
    assert((se_take(numbers, 2) == SEList<std::int64_t>{1, 9}));
    assert((se_drop(numbers, 2) == SEList<std::int64_t>{3}));
    auto numbers_alias=numbers;
    se_assign_index(numbers_alias,0,std::int64_t{41});
    assert(numbers[0]==41);
    const auto sorted_numbers=se_sorted(numbers);
    assert(sorted_numbers[0]==3 && numbers[0]==41);

    auto mapping = se_map(std::pair{std::string("a"), std::int64_t{1}},
                          std::pair{std::string("b"), std::int64_t{2}});
    mapping.put("a", 10);
    assert(mapping.size() == 2);
    auto mapping_alias=mapping;
    mapping_alias.put("shared",17);
    assert(mapping.size()==3);
    se_assign_index(mapping,std::string("new"),std::int64_t{12});
    se_assign_index(mapping,std::string("new"),std::int64_t{13});
    assert(mapping.size()==4 && mapping.items.back().first=="new");
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
    auto unique_alias=unique;
    unique_alias.add(5);
    assert(unique.size()==4);

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

    const SEList<std::string> ast = {
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

    const auto safe_root=std::filesystem::temp_directory_path()/"se-bootstrap-safe-root";
    const auto outside_root=std::filesystem::temp_directory_path()/"se-bootstrap-safe-outside";
    std::filesystem::remove_all(safe_root);
    std::filesystem::remove_all(outside_root);
    std::filesystem::create_directories(safe_root);
    std::filesystem::create_directories(outside_root);
    se_write((outside_root/"secret.txt").string(),"secret");
    std::filesystem::create_directory_symlink(outside_root,safe_root/"linked");
    bool path_escape_rejected=false;
    try{(void)se_static_read(safe_root.string(),"linked/secret.txt");}
    catch(const std::runtime_error&){path_escape_rejected=true;}
    assert(path_escape_rejected);
    path_escape_rejected=false;
    try{(void)se_upload_save(safe_root.string(),"linked/upload.txt","payload");}
    catch(const std::runtime_error&){path_escape_rejected=true;}
    assert(path_escape_rejected);
    std::filesystem::remove_all(safe_root);
    std::filesystem::remove_all(outside_root);

    assert((se_range(3, 1) == SEList<std::int64_t>{3, 2, 1}));
    auto rejects=[](auto operation){bool failed=false;try{operation();}catch(const std::exception&){failed=true;}assert(failed);};
    rejects([]{se_math_sqrt(-1);});
    rejects([]{se_math_clamp(0,2,1);});
    rejects([]{se_math_map_range(1,0,0,0,1);});
    rejects([]{se_math_factorial(21);});
    rejects([]{se_math_comb(67,33);});
    rejects([]{se_math_perm(3,4);});
    rejects([]{se_math_mean(SEList<double>{});});
    rejects([]{se_statistics_variance(SEList<double>{1});});
    rejects([]{se_regex_match("[", "text");});
    assert(se_math_comb(66,33)==7219428434016265740LL);
    assert(se_math_min(3,2.5,4)==2.5);
    assert(se_math_hypot(2,3,6)==7);
    std::cout << "SE bootstrap standalone runtime passed\n";
}
