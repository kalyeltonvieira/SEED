#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cctype>
#include <unordered_map>

using namespace std;

// (Lexer inserido no arquivo final via script ou merge)

// ==========================================
#include "ast.h" // Include AST definitions
#include "stdlib_stub.h" // Stub for stdlib modules
// AST & PARSER BASE
// ==========================================

class Parser {
    vector<Token> tokens;
    int pos = 0;

public:
    Parser(vector<Token> t) : tokens(t) {}

    Token peek(int offset = 0) {
        if (pos + offset >= tokens.size()) return {SeedToken::Eof, "", 0, 0};
        return tokens[pos + offset];
    }

    Token advance() {
        if (pos >= tokens.size()) return {SeedToken::Eof, "", 0, 0};
        return tokens[pos++];
    }

    bool match(SeedToken type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }

    bool match_text(string text) {
        if (peek().text == text) {
            advance();
            return true;
        }
        return false;
    }

    void expect(SeedToken type, string msg) {
        if (!match(type)) {
            cerr << "Parse error line " << peek().line << ": " << msg << " (got '" << peek().text << "')" << endl;
            exit(1);
        }
    }

    // Pass 1 and Pass 2 can be done by capturing blocks of tokens.
    // We will parse TopLevelDeclarations: Structs, Impls, Functions.
};

// We need a robust transpiler strategy.
struct TypeDef {
    string name;
    string body_cpp;
    vector<string> methods_decls;
    vector<string> methods_impls;
};

struct FuncDef {
    string signature;
    string body_cpp;
};

class Transpiler {
    vector<Token> tokens;
    int pos = 0;
    
    unordered_map<string, TypeDef> types;
    vector<FuncDef> functions;
    vector<string> global_statements;
    // New AST storage for function declarations
    std::vector<std::unique_ptr<seed::FunctionDecl>> function_ast;

    Token peek(int offset = 0) {
        if (pos + offset >= tokens.size()) return {SeedToken::Eof, "", 0, 0};
        return tokens[pos + offset];
    }

    Token advance() {
        if (pos >= tokens.size()) return {SeedToken::Eof, "", 0, 0};
        return tokens[pos++];
    }

    string map_type(string seed_type) {
        if (seed_type == "int") return "int";
        if (seed_type == "float") return "double";
        if (seed_type == "string") return "std::string";
        if (seed_type == "bool") return "bool";
        if (seed_type == "char") return "char";
        if (seed_type == "dict") return "seed::Dict";
        if (seed_type.find("Option<") == 0) return "std::optional<" + map_type(seed_type.substr(7, seed_type.length() - 8)) + ">";
        if (seed_type.find("Result<") == 0) return "std::expected<" + map_type(seed_type.substr(7, seed_type.length() - 8)) + ", std::string>"; // Simplification
        if (seed_type.find("[") == 0) return "std::vector<" + map_type(seed_type.substr(1, seed_type.length() - 2)) + ">";
        
        if (seed_type.find("&mut ") == 0) return map_type(seed_type.substr(5)) + "&";
        if (seed_type.find("&") == 0) return "const " + map_type(seed_type.substr(1)) + "&";
        
        return seed_type;
    }

    // Helper to consume a type definition (e.g., `int`, `[int]`, `Option<string>`)
    string parse_type_str() {
        string t;
        if (peek().type == SeedToken::LBracket) {
            advance();
            t = "[" + parse_type_str() + "]";
            advance(); // RBracket
        } else {
            t = advance().text;
            if (peek().type == SeedToken::Less) {
                t += advance().text; // <
                t += parse_type_str();
                if (peek().type == SeedToken::Comma) {
                    t += advance().text; // ,
                    t += parse_type_str();
                }
                t += advance().text; // >
            }
        }
        return t;
    }

    // Collect an expression until delimiter
    string parse_expr(SeedToken end_tok, SeedToken end_tok2 = SeedToken::Unknown) {
        string expr;
        int depth_paren = 0;
        int depth_brace = 0;
        int depth_bracket = 0;
        
        while (peek().type != SeedToken::Eof) {
            Token t = peek();
            
            if (depth_paren == 0 && depth_brace == 0 && depth_bracket == 0) {
                if (t.type == end_tok || t.type == end_tok2) break;
            }
            
            if (t.type == SeedToken::LParen) depth_paren++;
            else if (t.type == SeedToken::RParen) depth_paren--;
            else if (t.type == SeedToken::LBrace) depth_brace++;
            else if (t.type == SeedToken::RBrace) depth_brace--;
            else if (t.type == SeedToken::LBracket) depth_bracket++;
            else if (t.type == SeedToken::RBracket) depth_bracket--;
            
            // Map builtins and operators
            string text = t.text;
            if (text == "print") text = "seed::print";
            else if (text == "println") text = "seed::println";
            else if (text == "string") text = "std::string";
            else if (text == "[") {
                if (peek(1).type == SeedToken::RBracket) {
                    text = "{}";
                    advance(); // skip [
                    advance(); // skip ]
                    depth_bracket--;
                    expr += text + " ";
                    continue;
                }
            }
            else if (text == "match" && peek().type == SeedToken::Identifier) {
                // simple match expression (only handles empty match or translates to lambda if needed)
                advance(); // match
                string match_expr = parse_expr(SeedToken::LBrace);
                advance(); // {
                string blk = "";
                while (peek().type != SeedToken::Eof && peek().type != SeedToken::RBrace) {
                    blk += advance().text + " ";
                }
                advance(); // }
                // fallback translation for match until full pattern matching is done
                expr += "[&](){ /* match */ return " + match_expr + "; }() ";
                continue;
            }
            else if (text == "if" && (end_tok == SeedToken::Semicolon || end_tok == SeedToken::RBrace)) {
                // Inline if expression (ternary)
                advance(); // if
                string cond = parse_expr(SeedToken::LBrace);
                advance(); // {
                string val1 = parse_expr(SeedToken::RBrace);
                advance(); // }
                string val2 = "";
                if (peek().text == "else") {
                    advance(); // else
                    advance(); // {
                    val2 = parse_expr(SeedToken::RBrace);
                    advance(); // }
                }
                text = "((" + cond + ") ? (" + val1 + ") : (" + val2 + "))";
                expr += text + " ";
                continue;
            }
            else if (text == "math") {
                if (peek(1).type == SeedToken::Dot) {
                    advance(); advance();
                    string meth = advance().text;
                    if (meth == "PI") text = "3.141592653589793";
                    else text = "std::" + meth;
                }
            }
            
            expr += text + " ";
            advance();
        }
        return expr;
    }

    string parse_block() {
        string blk = "{\n";
        advance(); // LBrace
        while (peek().type != SeedToken::Eof && peek().type != SeedToken::RBrace) {
            blk += parse_statement() + "\n";
        }
        advance(); // RBrace
        blk += "}";
        return blk;
    }

    string parse_statement() {
        Token t = peek();
        
        if (t.text == "let") {
            advance();
            bool is_mut = false;
            if (peek().text == "mut") { is_mut = true; advance(); }
            string name = advance().text;
            string type_str = "auto";
            if (peek().type == SeedToken::Colon) {
                advance();
                type_str = map_type(parse_type_str());
            }
            advance(); // Assign
            string expr = parse_expr(SeedToken::Semicolon, SeedToken::RBrace); // SEED optional semicolon
            if (peek().type == SeedToken::Semicolon) advance();
            return (is_mut ? "" : "const ") + type_str + " " + name + " = " + expr + ";";
        }
        
        if (t.text == "if") {
            advance();
            string cond = parse_expr(SeedToken::LBrace);
            string block = parse_block();
            string else_block = "";
            if (peek().text == "else") {
                advance();
                if (peek().text == "if") {
                    else_block = " else " + parse_statement();
                } else {
                    else_block = " else " + parse_block();
                }
            }
            return "if (" + cond + ") " + block + else_block;
        }
        
        if (t.text == "while") {
            advance();
            string cond = parse_expr(SeedToken::LBrace);
            string block = parse_block();
            return "while (" + cond + ") " + block;
        }
        
        if (t.text == "for") {
            advance();
            string var = advance().text; // i or name
            advance(); // in
            string iter = parse_expr(SeedToken::LBrace);
            // Quick check if range
            size_t range_pos = iter.find("..");
            if (range_pos != string::npos) {
                string start = iter.substr(0, range_pos);
                string end = iter.substr(range_pos + 2);
                string block = parse_block();
                return "for (int " + var + " = " + start + "; " + var + " < " + end + "; ++" + var + ") " + block;
            }
            string block = parse_block();
            return "for (auto& " + var + " : " + iter + ") " + block;
        }
        else if (t.text == "loop") {
            advance();
            string block = parse_block();
            return "while (true) " + block;
        }
        
        else if (t.text == "return") {
            advance();
            string expr = parse_expr(SeedToken::Semicolon, SeedToken::RBrace);
            if (peek().type == SeedToken::Semicolon) advance();
            return "return " + expr + ";";
        }
        else if (t.text == "break") {
            advance();
            return "break;";
        }
        else if (t.text == "continue") {
            advance();
            return "continue;";
        }

        // Expression statement
        string expr = parse_expr(SeedToken::Semicolon, SeedToken::RBrace);
        if (peek().type == SeedToken::Semicolon) advance();
        return expr + ";";
    }

public:
    Transpiler(vector<Token> t) : tokens(t) {}
    
    void parse_top_level() {
        while (peek().type != SeedToken::Eof) {
            Token t = peek();
            if (t.text == "type" || t.text == "struct") {
                advance();
                string name = "AnonType";
                if (peek().type == SeedToken::Identifier) name = advance().text;
                if (peek().type == SeedToken::LBrace) advance(); // LBrace
                string body;
                while (peek().type != SeedToken::Eof && peek().type != SeedToken::RBrace) {
                    string field_name = advance().text;
                    if (peek().type == SeedToken::Colon) advance(); // Colon
                    string type_str = map_type(parse_type_str());
                    // comma or semicolon or newline
                    if (peek().type == SeedToken::Comma || peek().type == SeedToken::Semicolon) advance();
                    body += type_str + " " + field_name + ";\n";
                }
                if (peek().type == SeedToken::RBrace) advance(); // RBrace
                types[name] = {name, body, {}, {}};
            }
            else if (t.text == "impl") {
                advance();
                string name = "AnonImpl";
                if (peek().type == SeedToken::Identifier) name = advance().text;
                if (peek().type == SeedToken::LBrace) advance(); // LBrace
                while (peek().type != SeedToken::Eof && peek().type != SeedToken::RBrace) {
                    if (peek().text == "fn" || peek().text == "pub") {
                        if (peek().text == "pub") advance();
                        advance(); // fn
                        string meth_name = advance().text;
                        advance(); // LParen
                        string params = "";
                        bool has_self = false;
                        while (peek().type != SeedToken::RParen) {
                            if (peek().text == "self" || peek().text == "mut") {
                                has_self = true;
                                if (peek().text == "mut") advance();
                                advance(); // self
                            } else {
                                string pname = advance().text;
                                advance(); // colon
                                string ptype = map_type(parse_type_str());
                                params += ptype + " " + pname;
                                if (peek().type == SeedToken::Comma) { advance(); params += ", "; }
                            }
                        }
                        advance(); // RParen
                        string ret_type = "void";
                        if (peek().type == SeedToken::Arrow) {
                            advance();
                            ret_type = map_type(parse_type_str());
                        }
                        string block = parse_block();
                        
                        if (has_self) {
                            block.insert(2, "auto& self = *this;\n");
                        }
                        
                        string decl = (has_self ? "" : "static ") + ret_type + " " + meth_name + "(" + params + ");";
                        string def = ret_type + " " + name + "::" + meth_name + "(" + params + ") " + block;
                        
                        types[name].methods_decls.push_back(decl);
                        types[name].methods_impls.push_back(def);
                    } else {
                        advance(); // Skip unexpected tokens in impl block
                    }
                }
                if (peek().type == SeedToken::RBrace) advance(); // RBrace
            }
            else if (t.text == "fn" || t.text == "pub") {
                // Optional visibility modifier
                if (t.text == "pub") advance(); // skip 'pub'
                // Optional inline keyword
                bool is_inline = false;
                if (peek().text == "inline") { advance(); is_inline = true; }
                // Expect 'fn'
                advance(); // consume 'fn'
                string func_name = advance().text;
                // Parse optional generic parameters <T, U>
                string generic_part = "";
                if (peek().type == SeedToken::Less) {
                    string gen_params;
                    advance(); // '<'
                    while (peek().type != SeedToken::Greater && peek().type != SeedToken::Eof) {
                        gen_params += advance().text;
                        if (peek().type == SeedToken::Comma) { gen_params += ", "; advance(); }
                    }
                    if (peek().type == SeedToken::Greater) advance(); // '>'
                    generic_part = "template<" + gen_params + "> ";
                }
                // Parameter list
                advance(); // '('
                string params = "";
                bool first = true;
                while (peek().type != SeedToken::RParen) {
                    // Variadic parameter detection
                    if (peek().text == "...") { advance(); params += "..."; break; }
                    string pname = advance().text;
                    string ptype = "auto";
                    // Optional type annotation
                    if (peek().type == SeedToken::Colon) {
                        advance(); // ':'
                        ptype = map_type(parse_type_str());
                    }
                    // Optional default value
                    string default_val = "";
                    if (peek().type == SeedToken::Assign) {
                        advance(); // '='
                        default_val = " = " + parse_expr(SeedToken::Comma, SeedToken::RParen);
                    }
                    if (!first) params += ", ";
                    params += ptype + " " + pname + default_val;
                    first = false;
                    if (peek().type == SeedToken::Comma) advance();
                }
                advance(); // ')'
                // Return type
                string ret_type = "void";
                if (peek().type == SeedToken::Arrow) {
                    advance();
                    ret_type = map_type(parse_type_str());
                }
                string block = parse_block();
                string sig = generic_part + (is_inline ? "inline " : "") + "auto " + func_name + "(" + params + ") -> " + ret_type;
                functions.push_back({sig, block});
            }
                    string ptype = map_type(parse_type_str());
                    params += ptype + " " + pname;
                    if (peek().type == SeedToken::Comma) { advance(); params += ", "; }
                }
                advance(); // RParen
                string ret_type = "void";
                if (peek().type == SeedToken::Arrow) {
                    advance();
                    ret_type = map_type(parse_type_str());
                }
                string block = parse_block();
                
                string sig = "auto " + func_name + "(" + params + ") -> " + ret_type;
                functions.push_back({sig, block});
            }
            else if (t.text == "use" || t.text == "mod" || t.text == "import") {
                // skip for now; import will be handled via stub header
                while (peek().type != SeedToken::Semicolon && peek().line == t.line) advance();
            }
            else {
                // global statement or comment
                advance();
            }
        }
    }
        string generate_cpp() {
        string cpp;
        cpp += "#include \"stdlib_stub.h\"\n";
        // cpp += "#include \"seed_lib.hpp\"\n";
        for (auto& pair : types) {
            cpp += "struct " + pair.first + " {\n" + pair.second.body_cpp + "\n";
            for (auto& decl : pair.second.methods_decls) cpp += "    " + decl + "\n";
            cpp += "};\n\n";
        }
        
        // Method implementations
        for (auto& pair : types) {
            for (auto& impl : pair.second.methods_impls) cpp += impl + "\n\n";
        }
        
                // Function forward declarations (legacy)
        for (auto& fn : functions) {
            if (fn.signature.find("main(") == string::npos) {
                cpp += fn.signature + ";\n";
            }
        }
        // Function forward declarations from AST
        auto buildFunction = [&](const seed::FunctionDecl& fd, bool isDefinition) -> string {
            string result;
            // Template parameters
            if (!fd.generics.empty()) {
                result += "template<";
                for (size_t i = 0; i < fd.generics.size(); ++i) {
                    result += "typename " + fd.generics[i].name;
                    if (i + 1 < fd.generics.size()) result += ", ";
                }
                result += ">\n";
            }
            // Signature
            string sig;
            if (fd.isInline) sig += "inline ";
            sig += fd.returnType.empty() ? "void" : fd.returnType;
            sig += " " + fd.name + "(";
            for (size_t i = 0; i < fd.params.size(); ++i) {
                const auto& p = fd.params[i];
                sig += p.type + " " + p.name;
                if (!p.defaultValue.empty()) sig += " = " + p.defaultValue;
                if (i + 1 < fd.params.size()) sig += ", ";
            }
            sig += ")";
            result += sig;
            if (isDefinition) {
                result += " {}";
            } else {
                result += ";";
            }
            return result;
        };

        for (auto& fd_ptr : function_ast) {
            const auto& fd = *fd_ptr;
            cpp += buildFunction(fd, false) + "\n";
        }

        cpp += "\n";

        // Function implementations (legacy)
        for (auto& fn : functions) {
            if (fn.signature.find("main(") != string::npos) {
                cpp += "int main(int argc, char** argv) " + fn.body_cpp + "\n\n";
            } else {
                cpp += fn.signature + " " + fn.body_cpp + "\n\n";
            }
        }
        // Function implementations from AST (placeholder bodies)
        for (auto& fd_ptr : function_ast) {
            const auto& fd = *fd_ptr;
            cpp += buildFunction(fd, true) + "\n\n";
        }

        return cpp;
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: seed.exe <file.seed>\n";
        return 1;
    }
    
    string path = argv[1];
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Could not open file " << path << "\n";
        return 1;
    }
    
    string source((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    
    Lexer lexer(source);
    vector<Token> tokens = lexer.tokenize();
    
    Transpiler trans(tokens);
    trans.parse_top_level();
    
    string cpp_code = trans.generate_cpp();
    
    // Save to temp_build.cpp and compile
    string cpp_path = path + ".cpp";
    ofstream out(cpp_path);
    out << cpp_code;
    out.close();
    
    char exe_path_buf[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path_buf, MAX_PATH);
    string seed_exe_path = exe_path_buf;
    string seed_home = seed_exe_path.substr(0, seed_exe_path.find_last_of("\\/")) + "\\runtime";

    string cmd = "clang++ -std=c++20 -O2 -I\"" + seed_home + "\" \"" + cpp_path + "\" -o \"" + path + ".exe\"";
    system(cmd.c_str());
    
    return 0;
}
