#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cctype>
#include <unordered_map>
#include <windows.h>

using namespace std;

// ==========================================
// LEXER
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
    SeedToken last_type = SeedToken::Unknown;

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

    void skip_whitespace() {
        while (true) {
            char c = peek();
            if (c == '\n') break; // Handle newline in next_token for ASI
            if (isspace(c)) { advance(); continue; }
            if (c == '/' && peek(1) == '/') {
                while (peek() != '\n' && peek() != '\0') advance();
                continue;
            }
            if (c == '/' && peek(1) == '*') {
                advance(); advance();
                while (peek() != '\0' && !(peek() == '*' && peek(1) == '/')) advance();
                advance(); advance();
                continue;
            }
            break;
        }
    }

public:
    Lexer(string source) : src(source) {}

    Token next_token() {
        Token t = _next_token();
        last_type = t.type;
        return t;
    }

    Token _next_token() {
        skip_whitespace();
        if (pos >= src.length()) return {SeedToken::Eof, "", line, col};

        int start_line = line;
        int start_col = col;
        char c = peek();

        if (c == '\n') {
            advance();
            if (last_type == SeedToken::Identifier || last_type == SeedToken::Number ||
                last_type == SeedToken::String || last_type == SeedToken::Char ||
                last_type == SeedToken::RParen || last_type == SeedToken::RBracket ||
                last_type == SeedToken::RBrace || last_type == SeedToken::Question) {
                return {SeedToken::Semicolon, ";", start_line, start_col};
            }
            return _next_token();
        }

        if (isalpha(c) || c == '_') {
            string text;
            while (isalnum(peek()) || peek() == '_') text += advance();
            SeedToken type = SeedToken::Identifier;
            if (text == "fn" || text == "let" || text == "mut" || text == "const" ||
                text == "if" || text == "else" || text == "while" || text == "for" ||
                text == "in" || text == "loop" || text == "break" || text == "continue" ||
                text == "return" || text == "match" || text == "type" || text == "struct" ||
                text == "impl" || text == "enum" || text == "trait" || text == "mod" ||
                text == "use" || text == "pub" || text == "async" || text == "await" ||
                text == "spawn" || text == "macro" || text == "true" || text == "false") {
                type = SeedToken::Keyword;
            }
            return {type, text, start_line, start_col};
        }

        if (isdigit(c)) {
            string text;
            while (isdigit(peek()) || peek() == '.') {
                if (peek() == '.' && peek(1) == '.') break; // avoid consuming Range
                text += advance();
            }
            return {SeedToken::Number, text, start_line, start_col};
        }

        if (c == '"') {
            advance();
            string text;
            while (peek() != '"' && peek() != '\0') {
                if (peek() == '\\') { text += advance(); }
                text += advance();
            }
            advance();
            return {SeedToken::String, "\"" + text + "\"", start_line, start_col};
        }

        if (c == '\'') {
            advance();
            string text;
            if (peek() == '\\') { text += advance(); }
            text += advance();
            advance();
            return {SeedToken::Char, "'" + text + "'", start_line, start_col};
        }

        // Operators & punctuation
        string op(1, advance());
        char n = peek();
        if (op == "=" && n == '=') { op += advance(); return {SeedToken::Equal, op, start_line, start_col}; }
        if (op == "!" && n == '=') { op += advance(); return {SeedToken::NotEqual, op, start_line, start_col}; }
        if (op == "<" && n == '=') { op += advance(); return {SeedToken::LessEqual, op, start_line, start_col}; }
        if (op == ">" && n == '=') { op += advance(); return {SeedToken::GreaterEqual, op, start_line, start_col}; }
        if (op == "+" && n == '=') { op += advance(); return {SeedToken::PlusAssign, op, start_line, start_col}; }
        if (op == "-" && n == '=') { op += advance(); return {SeedToken::MinusAssign, op, start_line, start_col}; }
        if (op == "*" && n == '=') { op += advance(); return {SeedToken::StarAssign, op, start_line, start_col}; }
        if (op == "/" && n == '=') { op += advance(); return {SeedToken::SlashAssign, op, start_line, start_col}; }
        if (op == "&" && n == '&') { op += advance(); return {SeedToken::And, op, start_line, start_col}; }
        if (op == "|" && n == '|') { op += advance(); return {SeedToken::Or, op, start_line, start_col}; }
        if (op == "<" && n == '<') { op += advance(); return {SeedToken::Shl, op, start_line, start_col}; }
        if (op == ">" && n == '>') { op += advance(); return {SeedToken::Shr, op, start_line, start_col}; }
        if (op == "-" && n == '>') { op += advance(); return {SeedToken::Arrow, op, start_line, start_col}; }
        if (op == "|" && n == '>') { op += advance(); return {SeedToken::Pipe, op, start_line, start_col}; }
        if (op == ":" && n == ':') { op += advance(); return {SeedToken::DoubleColon, op, start_line, start_col}; }
        if (op == "." && n == '.') { 
            op += advance();
            if (peek() == '=') { op += advance(); return {SeedToken::RangeIncl, op, start_line, start_col}; }
            return {SeedToken::Range, op, start_line, start_col}; 
        }

        if (op == "=") return {SeedToken::Assign, op, start_line, start_col};
        if (op == "+") return {SeedToken::Plus, op, start_line, start_col};
        if (op == "-") return {SeedToken::Minus, op, start_line, start_col};
        if (op == "*") return {SeedToken::Star, op, start_line, start_col};
        if (op == "/") return {SeedToken::Slash, op, start_line, start_col};
        if (op == "%") return {SeedToken::Percent, op, start_line, start_col};
        if (op == "<") return {SeedToken::Less, op, start_line, start_col};
        if (op == ">") return {SeedToken::Greater, op, start_line, start_col};
        if (op == "!") return {SeedToken::Not, op, start_line, start_col};
        if (op == "&") return {SeedToken::BitAnd, op, start_line, start_col};
        if (op == "|") return {SeedToken::BitOr, op, start_line, start_col};
        if (op == "^") return {SeedToken::BitXor, op, start_line, start_col};
        if (op == "~") return {SeedToken::BitNot, op, start_line, start_col};
        if (op == ".") return {SeedToken::Dot, op, start_line, start_col};
        if (op == ",") return {SeedToken::Comma, op, start_line, start_col};
        if (op == ":") return {SeedToken::Colon, op, start_line, start_col};
        if (op == ";") return {SeedToken::Semicolon, op, start_line, start_col};
        if (op == "?") return {SeedToken::Question, op, start_line, start_col};
        if (op == "(") return {SeedToken::LParen, op, start_line, start_col};
        if (op == ")") return {SeedToken::RParen, op, start_line, start_col};
        if (op == "{") return {SeedToken::LBrace, op, start_line, start_col};
        if (op == "}") return {SeedToken::RBrace, op, start_line, start_col};
        if (op == "[") return {SeedToken::LBracket, op, start_line, start_col};
        if (op == "]") return {SeedToken::RBracket, op, start_line, start_col};

        // Check if attribute #
        if (op == "#") {
            if (peek() == '[') {
                string attr = "#";
                while (peek() != ']' && peek() != '\0') attr += advance();
                if (peek() == ']') attr += advance();
                return {SeedToken::Keyword, attr, start_line, start_col}; // Just treat attr as keyword to skip
            }
        }

        return {SeedToken::Unknown, op, start_line, start_col};
    }
    
    vector<Token> tokenize() {
        vector<Token> tokens;
        Token t;
        do {
            t = next_token();
            tokens.push_back(t);
        } while (t.type != SeedToken::Eof);
        return tokens;
    }
};

// ==========================================
// AST NODES
// ==========================================

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual string to_cpp() = 0;
};

// We will implement an incredibly relaxed CodeGen that just maps SEED tokens to C++ equivalents where needed,
// effectively writing a structural transpiler!
// Because full semantic AST resolution is complex, we will parse into block levels and translate.

