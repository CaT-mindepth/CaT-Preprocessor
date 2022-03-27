#include "paren_remover.h"
#include "clang_utility_functions.h"
#include "context.h"

#include <functional>

using namespace clang;

std::string
paren_remover_transform(const clang::TranslationUnitDecl *tu_decl) {
  UniqueIdentifiers uid(identifier_census(tu_decl));
  return pkt_func_transform(tu_decl, paren_remover);
}

std::pair<std::string, std::vector<std::string>>
paren_remover(const clang::CompoundStmt *function_body,
              const std::string &pkt_name) {
  std::string out = "";
  std::vector<std::string> new_decls;

  for (const auto *child : function_body->children()) {
    if (isa<BinaryOperator>(child)) {
      const auto *lhs =
          dyn_cast<BinaryOperator>(child)->getLHS()->IgnoreParenImpCasts();
      const auto *rhs =
          dyn_cast<BinaryOperator>(child)->getRHS()->IgnoreParenImpCasts();

      if (isa<ConditionalOperator>(rhs)) {
        const auto *ternary = dyn_cast<ConditionalOperator>(rhs);
        const auto *cond_expr = ternary->getCond()->IgnoreParenImpCasts();
        const auto *t_expr = ternary->getTrueExpr()->IgnoreParenImpCasts();
        const auto *f_expr = ternary->getFalseExpr()->IgnoreParenImpCasts();
        out += clang_stmt_printer(lhs) + " = (" +
               clang_stmt_printer(cond_expr) + ") ? (" +
               clang_stmt_printer(t_expr) + ") : (" +
               clang_stmt_printer(f_expr) + ");";
      } else
        out += clang_stmt_printer(lhs) + " = " + clang_stmt_printer(rhs) + ";";
    } else
      out += clang_stmt_printer(child) + ";";
  }

  return make_pair("{" + out + "}", new_decls);
}