#include "elim_ternary.h"

#include <map>

#include <iostream>
#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"

using namespace clang;
// Let's see if you ever find this comment
std::string ternary_transform(const TranslationUnitDecl * tu_decl) {
  return pkt_func_transform(tu_decl, ternary_body);
}

std::pair<std::string, std::vector<std::string>> ternary_body(const clang::CompoundStmt * function_body, const std::string & pkt_name __attribute__((unused))) {
  std::string transformed_body = "";
  const BinaryOperator * prev_bin_op; 

  for (const auto * child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    assert_exception(dyn_cast<BinaryOperator>(child)->isAssignmentOp());
    const auto * bin_op = dyn_cast<BinaryOperator>(child);
    // Get the left/right-hand-side of the assignment operation.
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const auto * rhs = bin_op->getRHS()->IgnoreParenImpCasts();
    if (isa<ConditionalOperator>(rhs)) {
      const auto * cond = dyn_cast<ConditionalOperator>(rhs);
      if (clang_stmt_printer(cond->getCond()->IgnoreParenImpCasts()) == "1") {
        // Eliminate expressions of form lhs := (1) ? a : b; 
        transformed_body += "  " + clang_stmt_printer(lhs) + " = " + clang_stmt_printer(cond->getTrueExpr()->IgnoreParenImpCasts());
        transformed_body += ";\n";
      } else {
        if (isa<ConditionalOperator>(prev_bin_op->getRHS())) {
          const auto * prev_cond = dyn_cast<ConditionalOperator>(prev_bin_op->getRHS());
          if (clang_stmt_printer(prev_cond->getCond()->IgnoreParenImpCasts()) 
            == clang_stmt_printer(cond->getCond()->IgnoreParenImpCasts())) {
              // they have the same condition.
             // if (clang_stmt_printer(prev_cond->getTrueExpr()) == clang_stmt_printer(prev_bin_op->getLHS())) {
             //   if (clang_stmt_printer(cond->))
             // }
              if (clang_stmt_printer(prev_cond->getFalseExpr()) == clang_stmt_printer(prev_bin_op->getLHS())) {

              }
            }
        }
        transformed_body += "  " + clang_stmt_printer(bin_op) + ";\n";
      }
    } else {
      transformed_body += "  " + clang_stmt_printer(bin_op) + ";\n";
    }
    prev_bin_op = bin_op;
  }
  return std::make_pair("{" + transformed_body + "}", std::vector<std::string>());
}
