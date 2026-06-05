#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <memory>
#include <variant>
#include <optional>
#include <expected>

using namespace std;

// ==========================================
// BUG #9 FIX: Lexer agora incluído inline
// ==========================================

enum class SeedToken {
    Identifier, Keyword, Number, String, Char,
    Plus, Minus, Star, Slash, Percent,
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign,
    Equal, NotEqual, Less, Greater, LessEqual, GreaterEqual,
    And, Or, Not, BitAnd, BitOr, BitXor, BitNot, Shl, Shr,
    Dot, Comma, Colon, DoubleColon, Semicolon,
    Arrow, Pipe, Range, RangeIncl, Question,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Eof, Unknown
};

struct Token {
    SeedToken type;
    string text;
    int line;
    int col;
};

class Lexer {
    string src;
    int pos = 0;
    int line = 1;
    int col = 1;

    char peek(int offset = 0) {
        if (pos + offset >= src.length()) return '\0';
        return src[pos + offset];
    }

    char advance() {
        if (pos >= src.length()) return '\0';
        char c = src[pos++];
        if (c == '\n') { line++; col = 1; }
        else { col++; }
        return c;
    }

public:
    Lexer(string source) : src(source) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        while (pos < src.length()) {
            // Skip whitespace
            while (pos < src.length() && isspace(peek())) advance();
            if (pos >= src.length()) break;

            int start_line = line, start_col = col;
            char c = peek();

            // Numbers
            if (isdigit(c)) {
                string num;
                while (isdigit(peek()) || peek() == '.') num += advance();
                tokens.push_back({SeedToken::Number, num, start_line, start_col});
            }
            // Identifiers/Keywords
            else if (isalpha(c) || c == '_') {
                string ident;
                while (isalnum(peek()) || peek() == '_') ident += advance();
                SeedToken type = SeedToken::Identifier;
                if (ident == "fn" || ident == "let" || ident == "if" || ident == "match" ||
                    ident == "loop" || ident == "while" || ident == "for" || ident == "return" ||
                    ident == "data" || ident == "type") {
                    type = SeedToken::Keyword;
                }
                tokens.push_back({type, ident, start_line, start_col});
            }
            // Strings
            else if (c == '"') {
                advance();
                string str;
                while (peek() != '"' && peek() != '\0') {
                    if (peek() == '\\') str += advance();
                    str += advance();
                }
                advance();
                tokens.push_back({SeedToken::String, "\"" + str + "\"", start_line, start_col});
            }
            // Operators
            else {
                string op(1, advance());
                if (op == "=" && peek() == '=') { op += advance(); tokens.push_back({SeedToken::Equal, op, start_line, start_col}); }
                else if (op == "!" && peek() == '=') { op += advance(); tokens.push_back({SeedToken::NotEqual, op, start_line, start_col}); }
                else if (op == "<" && peek() == '=') { op += advance(); tokens.push_back({SeedToken::LessEqual, op, start_line, start_col}); }
                else if (op == ">" && peek() == '=') { op += advance(); tokens.push_back({SeedToken::GreaterEqual, op, start_line, start_col}); }
                else if (op == "-" && peek() == '>') { op += advance(); tokens.push_back({SeedToken::Arrow, op, start_line, start_col}); }
                else if (op == "=" && peek() != '=') tokens.push_back({SeedToken::Assign, op, start_line, start_col});
                else if (op == "+") tokens.push_back({SeedToken::Plus, op, start_line, start_col});
                else if (op == "-") tokens.push_back({SeedToken::Minus, op, start_line, start_col});
                else if (op == "*") tokens.push_back({SeedToken::Star, op, start_line, start_col});
                else if (op == "/") tokens.push_back({SeedToken::Slash, op, start_line, start_col});
                else if (op == "(") tokens.push_back({SeedToken::LParen, op, start_line, start_col});
                else if (op == ")") tokens.push_back({SeedToken::RParen, op, start_line, start_col});
                else if (op == "{") tokens.push_back({SeedToken::LBrace, op, start_line, start_col});
                else if (op == "}") tokens.push_back({SeedToken::RBrace, op, start_line, start_col});
                else if (op == "[") tokens.push_back({SeedToken::LBracket, op, start_line, start_col});
                else if (op == "]") tokens.push_back({SeedToken::RBracket, op, start_line, start_col});
                else if (op == ".") tokens.push_back({SeedToken::Dot, op, start_line, start_col});
                else if (op == ",") tokens.push_back({SeedToken::Comma, op, start_line, start_col});
                else if (op == ";") tokens.push_back({SeedToken::Semicolon, op, start_line, start_col});
                else if (op == "?") tokens.push_back({SeedToken::Question, op, start_line, start_col});
                else if (op == ":") tokens.push_back({SeedToken::Colon, op, start_line, start_col});
                else tokens.push_back({SeedToken::Unknown, op, start_line, start_col});
            }
        }
        tokens.push_back({SeedToken::Eof, "", line, col});
        return tokens;
    }
};

// ==========================================
// BUG #1, #10 FIX: Proper Match Code Generation
// ==========================================

struct MatchArm {
    string pattern;      // "Ok", "Err", etc
    string binding;      // "value", "_", etc
    string body;         // C++ code
    string return_type;  // inferred type
};

string generate_match_cpp(const string& value_expr, const vector<MatchArm>& arms, bool is_result) {
    string cpp = "{\n";
    cpp += "auto __match_value = " + value_expr + ";\n";
    
    if (is_result) {
        cpp += "if (__match_value) {\n";
        for (const auto& arm : arms) {
            if (arm.pattern == "Ok" || arm.pattern == "Ok()") {
                cpp += "auto " + arm.binding + " = __match_value.value();\n";
                cpp += arm.body + "\n";
            }
        }
        cpp += "} else {\n";
        for (const auto& arm : arms) {
            if (arm.pattern == "Err" || arm.pattern == "Err()") {
                cpp += "auto " + arm.binding + " = __match_value.error();\n";
                cpp += arm.body + "\n";
            }
        }
        cpp += "}\n";
    }
    cpp += "}\n";
    return cpp;
}

// ==========================================
// BUG #2, #7 FIX: Type System
// ==========================================

enum class SeedType {
    Int, Float, String, Bool, Unit, Unknown
};

class TypeChecker {
    map<string, SeedType> variables;

public:
    SeedType infer_type(const string& expr) {
        // Numeric literal
        if (isdigit(expr[0])) {
            return expr.find('.') != string::npos ? SeedType::Float : SeedType::Int;
        }
        // String literal
        if (expr[0] == '"') return SeedType::String;
        // Variable lookup
        if (variables.count(expr)) return variables[expr];
        return SeedType::Unknown;
    }

    bool are_compatible(SeedType a, SeedType b) {
        if (a == b) return true;
        if (a == SeedType::Unknown || b == SeedType::Unknown) return true;
        if ((a == SeedType::Int || a == SeedType::Float) && 
            (b == SeedType::Int || b == SeedType::Float)) return true;
        return false;
    }

    void declare_var(const string& name, SeedType type) {
        variables[name] = type;
    }

    SeedType check_match_branches(const vector<MatchArm>& arms) {
        SeedType unified_type = SeedType::Unknown;
        
        for (const auto& arm : arms) {
            SeedType arm_type = infer_type(arm.body);
            
            if (unified_type == SeedType::Unknown) {
                unified_type = arm_type;
            } else if (!are_compatible(unified_type, arm_type)) {
                cerr << "ERROR: Match branch returns incompatible type\n";
                cerr << "Expected: " << (int)unified_type << ", Got: " << (int)arm_type << "\n";
                return SeedType::Unknown;
            }
        }
        return unified_type;
    }
};

// ==========================================
// BUG #3 FIX: Loop Support
// ==========================================

string translate_loop(const string& block) {
    return "while (true) " + block;
}

// ==========================================
// BUG #4, #12 FIX: String Methods & I/O
// ==========================================

namespace seed_builtins {
    inline double to_float(const string& s) {
        try { return stod(s); }
        catch (...) { return 0.0; }
    }

    inline string to_string(double d) {
        string s = std::to_string(d);
        while (s.back() == '0' && s.find('.') != string::npos) s.pop_back();
        if (s.back() == '.') s.pop_back();
        return s;
    }

    inline string prompt(const string& msg) {
        std::cout << msg;
        string line;
        std::getline(std::cin, line);
        return line;
    }

    inline string read_line() {
        string line;
        std::getline(std::cin, line);
        return line;
    }

    vector<string> split(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        for (char c : s) {
            if (c == delimiter) {
                tokens.push_back(token);
                token.clear();
            } else {
                token += c;
            }
        }
        tokens.push_back(token);
        return tokens;
    }

    // BUG #12 FIX: Error propagation
    template<typename T>
    struct Result {
        optional<T> value;
        string error;
        
        bool has_value() const { return value.has_value(); }
        T unwrap() const { return value.value(); }
        const string& get_error() const { return error; }
    };
}

// ==========================================
// BUG #5 FIX: Namespace qualification
// ==========================================

// All builtins properly namespaced as seed::
namespace seed {
    using namespace seed_builtins;
    
    inline void print(const string& s) { std::cout << s; }
    inline void println(const string& s) { std::cout << s << "\n"; }
    inline string read_line() { return seed_builtins::read_line(); }
}

// ==========================================
// BUG #6 FIX: Proper C++ Code Formatting
// ==========================================

class CodeFormatter {
    string trim(const string& s) {
        size_t start = 0, end = s.size();
        while (start < end && isspace(s[start])) start++;
        while (end > start && isspace(s[end-1])) end--;
        return s.substr(start, end - start);
    }

public:
    string format_expression(const string& expr) {
        // Remove extra spaces
        string result;
        bool last_was_space = false;
        for (char c : expr) {
            if (isspace(c)) {
                if (!last_was_space && !result.empty()) {
                    result += ' ';
                    last_was_space = true;
                }
            } else {
                result += c;
                last_was_space = false;
            }
        }
        return trim(result);
    }
};

// ==========================================
// BUG #8 FIX: Compiler Detection
// ==========================================

class CompilerDetector {
public:
    enum class Compiler { MSVC, Clang, GCC, Unknown };

    static Compiler detect() {
#ifdef _MSC_VER
        return Compiler::MSVC;
#elif __clang__
        return Compiler::Clang;
#elif __GNUC__
        return Compiler::GCC;
#else
        return Compiler::Unknown;
#endif
    }

    static string get_compile_command(const string& source_file, const string& output_file) {
        Compiler compiler = detect();
        
        switch (compiler) {
            case Compiler::MSVC:
                return "cl.exe /std:c++latest /O2 /I\"runtime\" \"" + source_file + "\" /Fe\"" + output_file + "\"";
            case Compiler::Clang:
                return "clang++ -std=c++23 -O2 -I./runtime \"" + source_file + "\" -o \"" + output_file + "\"";
            case Compiler::GCC:
                return "g++ -std=c++23 -O2 -I./runtime \"" + source_file + "\" -o \"" + output_file + "\"";
            default:
                return "clang++ -std=c++23 -O2 -I\"runtime\" \"" + source_file + "\" -o \"" + output_file + "\"";
        }
    }
};

// ==========================================
// BUG #10 FIX: ADT Code Generation with std::variant
// ==========================================

class ADTGenerator {
public:
    struct ADTVariant {
        string name;
        vector<string> fields;
    };

    struct ADTDef {
        string name;
        vector<ADTVariant> variants;
    };

    static string generate_cpp(const ADTDef& adt) {
        string cpp = "// ADT: " + adt.name + "\n";
        
        // Generate variant type
        cpp += "using " + adt.name + "_variant = std::variant<\n";
        for (size_t i = 0; i < adt.variants.size(); ++i) {
            cpp += "    std::tuple<"; // Each variant as a tuple
            for (size_t j = 0; j < adt.variants[i].fields.size(); ++j) {
                cpp += adt.variants[i].fields[j];
                if (j + 1 < adt.variants[i].fields.size()) cpp += ", ";
            }
            cpp += ">";
            if (i + 1 < adt.variants.size()) cpp += ",\n";
        }
        cpp += "\n>;\n\n";
        
        // Generate wrapper struct
        cpp += "struct " + adt.name + " {\n";
        cpp += "    " + adt.name + "_variant value;\n";
        cpp += "};\n";
        
        return cpp;
    }
};

// ==========================================
// BUG #11 FIX: Effect Audit
// ==========================================

enum class Effect {
    Pure, IO, FS, Net, Crypto, MutateCode
};

class EffectValidator {
    map<string, set<Effect>> function_effects;

public:
    void register_function(const string& func_name, const set<Effect>& effects) {
        function_effects[func_name] = effects;
    }

    bool validate_call(const string& caller, const string& called, const set<Effect>& caller_effects) {
        if (function_effects.count(called) == 0) {
            cerr << "Warning: Function '" << called << "' effect not registered\n";
            return false;
        }
        
        const auto& called_effects = function_effects[called];
        
        for (const auto& effect : called_effects) {
            if (caller_effects.find(effect) == caller_effects.end()) {
                cerr << "ERROR: " << caller << " cannot call " << called << "\n";
                cerr << "Caller missing effect: " << (int)effect << "\n";
                return false;
            }
        }
        return true;
    }
};
