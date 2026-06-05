#include "seed_lib.hpp"

struct MyInt;

using MyInt = int;

struct MyInt {

};

auto bad_type_declaration() -> void;
auto bad_return() -> int;
auto bad_scope() -> void;
void test_0_constant_folding();

auto bad_type_declaration() -> void {
const std::string nome = 123 ;
}

auto bad_return() -> int {
return std::string("string_value") ;
}

auto bad_scope() -> void {
const auto a = 1 ;
const auto a = 2 ;
const auto b = undeclared_var ;
}

void test_0_constant_folding() {
const int x = 7;
assert ( x == 7 ) ;
const signed char signed_8 = 10 ;
const short signed_16 = 1000 ;
const unsigned int unsigned_32 = 42 ;
const unsigned __int128 unsigned_128 = 999999 ;
const bool is_active = true let letter : char = 'a' ;
const MyInt alias_val = 456 ;
const std::optional<std::string> option_val = Option .Some ( std::string("value") ) ;
const std::any generic_any = 123 ;
}

int main(int argc, char** argv) {
    int passed = 0;
    int failed = 0;
    std::vector<std::pair<std::string, std::function<void()>>> test_funcs = {
        {"constant-folding", test_0_constant_folding},
    };
    for (const auto& [name, func] : test_funcs) {
        std::cout << "[RUNNING] " << name << "\n";
        try {
            func();
            std::cout << "[PASSED]  " << name << "\n";
            passed++;
        } catch (const std::exception& e) {
            std::cout << "[FAILED]  " << name << ": " << e.what() << "\n";
            failed++;
        } catch (...) {
            std::cout << "[FAILED]  " << name << ": unknown error\n";
            failed++;
        }
    }
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Total:  " << (passed + failed) << "\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    return (failed > 0) ? 1 : 0;
}
