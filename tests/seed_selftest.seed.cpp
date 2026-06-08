#include "stdlib_stub.h"



void test_0_hello_world_approved();
void test_1_naive_fibonacci_warned();
void test_2_bubble_sort_needs_negotiation();
void test_3_keep_compiles_original();
void test_4_select_star_expires();
void test_5_implicit_string_int_negotiates();
void test_6_non_exhaustive_result_match_warned();
void test_7_unchecked_result_needs_question_or_match();
void test_8_question_mark_documents_result_flow();
void test_9_genitor_produces_search_plan();
void test_10_effects_cannot_grow_silently();
void test_11_license_accepts_no();

void test_0_hello_world_approved() {
const auto p = seed1 . compile ( std::string("fn main() { print(\"Hello World\") }") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED000") ; } ) ) ;
}

void test_1_naive_fibonacci_warned() {
const auto p = seed1 . compile ( std::string("fn fib(n) { math.fib_matrix(n) }") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED002") ; } ) ) ;
assert ( seed::any(p.rewrites, [&](auto r) { return r . name == std::string("fib-naive-to-matrix") ; } ) ) ;
}

void test_2_bubble_sort_needs_negotiation() {
const auto src = std::string(R"raw(
    fn ordenar(lista) {
        for i in 0..len(lista) {
            for j in 0..len(lista)-1 {
                if lista[j] > lista[j+1] { trocar(lista, j, j+1) }
            }
    }
lista
}
)raw") ;
const auto p = seed1 . compile ( src ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED001") ; } ) ) ;
assert ( seed::any(p.rewrites, [&](auto r) { return r . name == std::string("bubble-sort-to-timsort") ; } ) ) ;
}

void test_3_keep_compiles_original() {
const auto src = std::string("#[keep] fn ordenar(lista) { for i in 0..len(lista) { for j in 0..len(lista)-1 { if lista[j] > lista[j+1]{ swap(lista,j,j+1) } } } lista }") ;
const auto p = seed1 . compile ( src ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . message . contains ( std::string("preservado") ) ; } ) ) ;
}

void test_4_select_star_expires() {
const auto p = seed1 . compile ( std::string("fn buscar() { sql_query(\"SELECT * FROM usuarios LIMIT 100\")? }") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED003") ; } ) ) ;
assert ( seed::any(p.rewrites, [&](auto r) { return r . name == std::string("select-star-to-paged-cache") ; } ) ) ;
}

void test_5_implicit_string_int_negotiates() {
const auto p = seed1 . compile ( std::string("let nome: string = \"Joao\"; let idade: int = 30; let r = nome + idade") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED004") ; } ) ) ;
}

void test_6_non_exhaustive_result_match_warned() {
const auto p = seed1 . compile ( std::string("fn explain(r: Result[Int,Error]) -> String { match r { Ok(v) => to_string(v), Err(e) => ") error std::string(" } }") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED005") ; } ) ) ;
assert ( seed::any(p.rewrites, [&](auto r) { return r . name == std::string("complete-match-result") ; } ) ) ;
}

void test_7_unchecked_result_needs_question_or_match() {
const auto p = seed1 . compile ( std::string("fn load(path: String) effect fs -> Result[String,Error] { let body = fs.read_text(path)?; return Ok(body) }") ) ;
assert ( seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED006") ; } ) ) ;
assert ( seed::any(p.rewrites, [&](auto r) { return r . name == std::string("unchecked-result-to-question") ; } ) ) ;
}

void test_8_question_mark_documents_result_flow() {
const auto p = seed1 . compile ( std::string("fn load(path: String) effect fs -> Result[String,Error] { let body = fs.read_text(path)?; return Ok(body) }") ) ;
assert ( ! seed::any(p.diagnostics, [&](auto d) { return d . code == std::string("SEED006") ; } ) ) ;
assert ( seed::any(p.notes, [&](auto o) { return o . name == std::string("result-propagation") ; } ) ) ;
}

void test_9_genitor_produces_search_plan() {
const auto result = synthesizer . synthesize ( std::string("crie uma funcao que ordena uma lista de numeros") ) ;
assert ( result . intent . name == std::string("sort") ) ;
assert ( seed::any(result.selected.tests, [&](auto t) { return t . contains ( std::string("ordenada") ) ; } ) ) ;
}

void test_10_effects_cannot_grow_silently() {
const auto before = seed0 . compile ( std::string("fn a() { return 1 }") ) ;
const auto after = seed0 . compile ( std::string("fn a() { net.post(\"x\", \"y\") }") ) ;
assert ( seed1 . effect_audit ( before , after ) == false ) ;
}

void test_11_license_accepts_no() {
assert ( license . accept_no == true ) ;
}

int main(int argc, char** argv) {
    int passed = 0;
    int failed = 0;
    std::vector<std::pair<std::string, std::function<void()>>> test_funcs = {
        {"hello-world-approved", test_0_hello_world_approved},
        {"naive-fibonacci-warned", test_1_naive_fibonacci_warned},
        {"bubble-sort-needs-negotiation", test_2_bubble_sort_needs_negotiation},
        {"keep-compiles-original", test_3_keep_compiles_original},
        {"select-star-expires", test_4_select_star_expires},
        {"implicit-string-int-negotiates", test_5_implicit_string_int_negotiates},
        {"non-exhaustive-result-match-warned", test_6_non_exhaustive_result_match_warned},
        {"unchecked-result-needs-question-or-match", test_7_unchecked_result_needs_question_or_match},
        {"question-mark-documents-result-flow", test_8_question_mark_documents_result_flow},
        {"genitor-produces-search-plan", test_9_genitor_produces_search_plan},
        {"effects-cannot-grow-silently", test_10_effects_cannot_grow_silently},
        {"license-accepts-no", test_11_license_accepts_no},
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
