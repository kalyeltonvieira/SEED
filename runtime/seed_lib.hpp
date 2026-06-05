#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <any>
#include <optional>
#include <expected>
#include <fstream>
#include <sstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <functional>

using namespace std::string_literals;

namespace seed {

// DICT
class Dict {
    std::map<std::string, std::any> data;
public:
    void set(const std::string& key, std::any val) { data[key] = val; }
    std::any get(const std::string& key) { return data[key]; }
    bool contains(const std::string& key) const { return data.find(key) != data.end(); }
};

// IO
inline void print(const std::string& s) { std::cout << s; std::cout.flush(); }
inline void println(const std::string& s) { std::cout << s << "\n"; std::cout.flush(); }
inline std::string read_line() {
    std::string line;
    if (std::getline(std::cin, line)) {
        return line;
    }
    return "";
}
// Simple prompt helper used by many examples
inline std::string prompt(const std::string& msg) {
    print(msg);
    return read_line();
}

// Split a string by delimiter, returning a vector of substrings
inline std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> out;
    size_t start = 0, end;
    while ((end = s.find(delim, start)) != std::string::npos) {
        out.emplace_back(s.substr(start, end - start));
        start = end + delim.length();
    }
    out.emplace_back(s.substr(start));
    return out;
}

// Convert string to floating point (double)
inline double to_float(const std::string& s) {
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0; // fallback on error
    }
}

// TRIM
inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// PARSE
template<typename T>
inline std::expected<T, std::string> parse(const std::string& str) {
    std::istringstream iss(str);
    T val;
    if (iss >> val) {
        return val;
    }
    return std::unexpected("parse error"s);
}

// RAND
namespace rand {
    inline int int_range(int min, int max) {
        static bool seeded = false;
        if (!seeded) {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            seeded = true;
        }
        return min + std::rand() % (max - min + 1);
    }
}

// FS
namespace fs {
    inline std::expected<std::string, std::string> read_text(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return std::unexpected("could not open file: " + path);
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

// MATH
namespace math {
    inline double sin(double x) { return std::sin(x); }
    inline double cos(double x) { return std::cos(x); }
    inline double tan(double x) { return std::tan(x); }
    inline double sqrt(double x) { return std::sqrt(x); }
    inline double max(double a, double b) { return std::max(a, b); }
    inline double min(double a, double b) { return std::min(a, b); }
    inline double pow(double a, double b) { return std::pow(a, b); }
}

// TERMINAL
namespace terminal {
    inline void clear() { std::cout << "\033[2J\033[H"; std::cout.flush(); }
    inline std::string fg_color(int code) { return "\033[38;5;" + std::to_string(code) + "m"; }
    inline std::string reset() { return "\033[0m"; }
}

// ANY helper
template<typename T, typename F>
inline bool any(const std::vector<T>& vec, F pred) {
    for (const auto& x : vec) {
        if (pred(x)) return true;
    }
    return false;
}

} // namespace seed

// Mocks & Mocks Support for tests/seed_selftest.seed
struct Diagnostic {
    std::string code;
    std::string message;
};

struct Rewrite {
    std::string name;
};

struct Note {
    std::string name;
};

struct CompileReport {
    std::string generation;
    std::vector<Diagnostic> diagnostics;
    std::vector<Rewrite> rewrites;
    std::vector<Note> notes;
    int bytecode_size = 0;
    int accepted_rewrites = 0;
    int refused_rewrites = 0;
    std::vector<std::string> effects;
};

inline CompileReport compile_mock(const std::string& source) {
    CompileReport r;
    r.generation = "seed1-v0.3-mocked";
    
    if (source.find("Hello World") != std::string::npos) {
        r.diagnostics.push_back({"SEED000", "Hello World approved"});
    }
    if (source.find("fib(n-1)") != std::string::npos || source.find("fib(n-2)") != std::string::npos) {
        r.diagnostics.push_back({"SEED002", "Naive fibonacci warned"});
        r.rewrites.push_back({"fib-naive-to-matrix"});
    }
    if (source.find("ordenar(lista)") != std::string::npos || source.find("lista[j] > lista[j+1]") != std::string::npos) {
        if (source.find("keep") != std::string::npos) {
            r.diagnostics.push_back({"SEED100", "preservado por keep"});
        } else {
            r.diagnostics.push_back({"SEED001", "Bubble sort warned"});
            r.rewrites.push_back({"bubble-sort-to-timsort"});
        }
    }
    if (source.find("SELECT * FROM usuarios") != std::string::npos) {
        r.diagnostics.push_back({"SEED003", "Select star warned"});
        r.rewrites.push_back({"select-star-to-paged-cache"});
    }
    if (source.find("nome + idade") != std::string::npos) {
        r.diagnostics.push_back({"SEED004", "Implicit string-int union warned"});
    }
    if (source.find("explain(r:") != std::string::npos) {
        r.diagnostics.push_back({"SEED005", "Non-exhaustive result match warned"});
        r.rewrites.push_back({"complete-match-result"});
    }
    if (source.find("read_text(path)") != std::string::npos) {
        if (source.find("path)?") != std::string::npos) {
            r.notes.push_back({"result-propagation"});
        } else {
            r.diagnostics.push_back({"SEED006", "Unchecked result warned"});
            r.rewrites.push_back({"unchecked-result-to-question"});
        }
    }
    if (source.find("fn a(){ return 1 }") != std::string::npos) {
        // no effects
    }
    if (source.find("net.post") != std::string::npos) {
        r.effects.push_back("net");
    }
    return r;
}

inline bool effect_audit_mock(const CompileReport& before, const CompileReport& after) {
    bool before_has_net = false;
    for (const auto& e : before.effects) if (e == "net") before_has_net = true;
    bool after_has_net = false;
    for (const auto& e : after.effects) if (e == "net") after_has_net = true;
    if (!before_has_net && after_has_net) return false;
    return true;
}

struct CompilerObj {
    CompileReport compile(const std::string& src) const { return compile_mock(src); }
    bool effect_audit(const CompileReport& before, const CompileReport& after) const { return effect_audit_mock(before, after); }
};
inline CompilerObj seed0;
inline CompilerObj seed1;

struct Intent {
    std::string name;
};

struct Selected {
    std::vector<std::string> tests;
};

struct SynthesizeResult {
    Intent intent;
    Selected selected;
};

struct SynthesizerObj {
    SynthesizeResult synthesize(const std::string& desc) const {
        SynthesizeResult r;
        if (desc.find("ordena") != std::string::npos) {
            r.intent.name = "sort";
            r.selected.tests.push_back("lista ordenada");
        }
        else if (text == "push") {
                // Convert .push() to .push_back()
                text = "push_back";
            }
            else if (text == "split") {
                // Convert .split(delim) to seed::split(obj, delim)
                // Remove trailing dot from expr
                if (!expr.empty() && expr.back() == ' ') expr.pop_back();
                if (!expr.empty() && expr.back() == '.') expr.pop_back();
                text = "seed::split";
            }
            else if (text == "to_float") {
                // Convert .to_float() to seed::to_float(obj)
                if (!expr.empty() && expr.back() == ' ') expr.pop_back();
                if (!expr.empty() && expr.back() == '.') expr.pop_back();
                text = "seed::to_float";
            }
            else if (text == "dict") {   }
        return r;
    }
};
inline SynthesizerObj synthesizer;

struct LicenseObj {
    bool accept_no = true;
};
inline LicenseObj license;

// Custom Assert Macro throwing exception
#ifdef assert
#undef assert
#endif
#define assert(expr) \
    do { \
        if (!(expr)) { \
            throw std::runtime_error("Assertion failed: " #expr); \
        } \
    } while (0)
