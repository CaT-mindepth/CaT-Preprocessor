#include "elim_identical_lhs_rhs.h"
#include "pkt_func_transform.h"
#include "clang_utility_functions.h"

#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include <utility>

using namespace clang;

std::string
elim_identical_lhs_rhs_transform(const clang::TranslationUnitDecl *tu_decl) {
  return pkt_func_transform(tu_decl, elim_identical_lhs_rhs);
}
// must be scheduled after if-conversion but before SSA.
std::pair<std::string, std::vector<std::string>>
elim_identical_lhs_rhs(const clang::CompoundStmt *function_body,
                       const std::string &pkt_name __attribute__((unused))) {
  std::string out = "";

  for (const auto *child : function_body->children()) {
    assert(isa<BinaryOperator>(child));
    const auto *assgn_op = dyn_cast<BinaryOperator>(child);
    const auto * lhs = assgn_op->getLHS()->IgnoreParenImpCasts();
    const auto * rhs = assgn_op->getRHS()->IgnoreParenImpCasts();
    // escape the case where lhs == rhs.
    if (clang_stmt_printer(lhs) == clang_stmt_printer(rhs)) continue;
    out += clang_stmt_printer(assgn_op) + ";\n";
  }
  return std::make_pair("{" + out + "}", std::vector<std::string>());
}