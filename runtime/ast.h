#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

namespace seed {

// Base class for all AST nodes
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Expressions
struct Expr : ASTNode {};

struct LiteralExpr : Expr {
    std::string value;
    explicit LiteralExpr(const std::string &v) : value(v) {}
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(const std::string &n) : name(n) {}
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    explicit CallExpr(std::unique_ptr<Expr> c) : callee(std::move(c)) {}
};

// Parameter declaration
struct ParamDecl : ASTNode {
    std::string name;
    std::string type;
    std::string defaultValue; // empty if none
    bool isVariadic = false;
    bool isMutable = false;
};

// Generic parameter
struct GenericParam : ASTNode {
    std::string name;
    std::string constraint; // optional concept/trait
};

// Function declaration
struct FunctionDecl : ASTNode {
    std::string name;
    bool isInline = false;
    bool isPublic = false;
    std::vector<GenericParam> generics;
    std::vector<ParamDecl> params;
    std::string returnType; // "void" if omitted
    std::unique_ptr<Expr> body; // placeholder for future
};

// Statements
struct Stmt : ASTNode {};

struct LetStmt : Stmt {
    ParamDecl decl;
    std::unique_ptr<Expr> init;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch; // nullptr if none
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace seed

#endif // AST_H
