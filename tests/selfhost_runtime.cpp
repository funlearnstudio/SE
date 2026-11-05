// Standalone bootstrap runtime tests: intentionally link NO production s_core.
#include "se_bootstrap_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
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

    auto mapping = se_map(std::pair{std::string("a"), std::int64_t{1}},
                          std::pair{std::string("b"), std::int64_t{2}});
    mapping.put("a", 10);
    assert(mapping.size() == 2);
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

    assert((se_range(3, 1) == std::vector<std::int64_t>{3, 2, 1}));
    std::cout << "SE bootstrap standalone runtime passed\n";
}
