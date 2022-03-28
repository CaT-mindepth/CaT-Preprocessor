#ifndef _ARRAY_REPLACER_H_
#define _ARRAY_REPLACER_H_

#include "ast_visitor.h"
#include "compiler_pass.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

#include "clang/AST/Expr.h"

#include <string>
#include <utility>
#include <vector>

std::string array_replacer_transform(const clang::TranslationUnitDecl *tu_decl);

class ReplacementVisitor : public AstVisitor {

protected:
  // replace array subscript expression A[x] with base A.
  std::string ast_visit_array_subscript_expr(
      const clang::ArraySubscriptExpr *array_subscript_expr) {
    const auto *base = array_subscript_expr->getBase();
    return clang_stmt_printer(base);
  }
};

std::pair<std::string, std::vector<std::string>>
array_replacer(const clang::Stmt *function_body,
               const std::string &pkt_name, ReplacementVisitor &visitor,
               clang::ASTContext *_ctx);

#endif