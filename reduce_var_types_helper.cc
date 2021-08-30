#include "reduce_var_types_helper.h"

#include <functional>
#include <iostream>

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

using namespace clang;

std::string
reduce_var_types_helper_transform(const TranslationUnitDecl *tu_decl) {
  return pkt_func_transform(tu_decl, reduce_var_types_helper_body);
}

std::pair<std::string, std::vector<std::string>>
reduce_var_types_helper_body(const clang::CompoundStmt *function_body,
                             const std::string &pkt_name
                             __attribute__((unused))) {
  std::string transformed_body = "";

  // Vector of newly created packet temporaries
  std::vector<std::string> new_decls = {};

  // Check that it's in ssa.
  assert_exception(is_in_ssa(function_body));

  // Map to store the variables we'd like to batch-rename.
  std::map<std::string, std::string> changeTo;
  for (const auto *child : function_body->children()) {
    // Extract packet variable (i.e. only look at assignment statements.)
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());
    const auto *lhs = bin_op->getLHS();
    const auto *rhs = bin_op->getRHS();
    const std::string pkt_var = clang_stmt_printer(lhs);
    const std::string expr_str = clang_stmt_printer(rhs);
// The caveat here is we can't correctly deduce types
// of boolean types mixed with integer arithmetic.
// For instance, `int a = (3 > 2) + 1` is valid C code
// but here we'll end up renaming a to a_br, thus giving
// it an inferred type of `bit` in the final stage. But
// this is fine, since this type of code does not make
// a lot of sense anyways.
// TODO: this is a bit of a kludge since we pattern match multiple
// times on the same string, maybe rewrite it into a tree-search pass in the
// future.
#define Contains(str, op) (str.find((op)) != std::string::npos)
    if (Contains(expr_str, ">") || Contains(expr_str, "<") ||
        Contains(expr_str, "&&") || Contains(expr_str, "||") ||
        Contains(expr_str, "==")) {
      if (!Contains(pkt_var, "br_")) {
        changeTo[pkt_var] =
            pkt_var + "_br_"; // TODO: modify variable prefix instead of suffix.
                              // But this involves parsing "p.xxx" part.
      }
    }
#undef Contains
  }

  // Now carry out replacements.
  for (const auto *child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    assert_exception(dyn_cast<BinaryOperator>(child)->isAssignmentOp());
    // Check whether LHS is one of the eliminated constant variable assignments

    const auto *lhs =
        dyn_cast<BinaryOperator>(child)->getLHS()->IgnoreParenImpCasts();
    if (changeTo.find(clang_stmt_printer(lhs)) == changeTo.end()) {
      transformed_body +=
          replace_vars(dyn_cast<BinaryOperator>(child)->getLHS(), changeTo,
                       {{VariableType::STATE_SCALAR, false},
                        {VariableType::STATE_ARRAY, false},
                        {VariableType::PACKET, true}}) +
          " = " +
          replace_vars(dyn_cast<BinaryOperator>(child)->getRHS(), changeTo,
                       {{VariableType::STATE_SCALAR, false},
                        {VariableType::STATE_ARRAY, false},
                        {VariableType::PACKET, true}}) +
          ";";
    }
  }

  return std::make_pair("{" + transformed_body + "}",
                        std::vector<std::string>());
}
