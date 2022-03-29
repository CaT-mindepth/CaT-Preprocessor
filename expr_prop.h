#ifndef EXPR_PROP_H_
#define EXPR_PROP_H_

#include <map>
#include <string>
#include <utility>
#include <iostream>

#include "ast_visitor.h"
#include "clang/AST/AST.h"
#include "clang_utility_functions.h"

/// Entry point from SinglePass to expression
/// propagater, calls expr_prop_fn_body immediately
std::string expr_prop_transform(const clang::TranslationUnitDecl *tu_decl);

/// Expr propagation: replace y = b + c; a = y;
/// with y=b+c; a=b+c; In some sense we are inverting
/// common subexpression elimination
std::pair<std::string, std::vector<std::string>>
expr_prop_fn_body(const clang::CompoundStmt *function_body,
                  const std::string &pkt_name __attribute__((unused)),
                  clang::ASTContext * _ctx);

class ExprSubstVisitor : public AstVisitor {
public:
  ExprSubstVisitor(std::map<std::string, std::string> &rewrite) {
    this->_rewrite = &(rewrite);
  }

protected:
  std::string ast_visit_member_expr(const clang::MemberExpr *member_expr) {
    const auto *expr = member_expr->IgnoreParenImpCasts();
    const auto expr_str = clang_stmt_printer(expr);
    // std::cout << "size of rewrite : " << this->_rewrite->size() << std::endl;
    if (this->_rewrite->find(expr_str) != this->_rewrite->end()) {
      return this->_rewrite->at(expr_str);
    } else
      return expr_str;
  }

private:
  const std::map<std::string, std::string> * _rewrite;
};

#endif // EXPR_PROP_H_
