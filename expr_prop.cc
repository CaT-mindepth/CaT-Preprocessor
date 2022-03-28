#include "expr_prop.h"

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"

using namespace clang;

std::string expr_prop_transform(const clang::TranslationUnitDecl *tu_decl) {
  return pkt_func_transform(
      tu_decl, std::bind(&expr_prop_fn_body, std::placeholders::_1,
                         std::placeholders::_2, &(tu_decl->getASTContext())));
}

std::pair<std::string, std::vector<std::string>>
expr_prop_fn_body(const CompoundStmt *function_body,
                  const std::string &pkt_name __attribute__((unused)),
                  clang::ASTContext * _ctx) {
  // Do not run if this is not in SSA
  if (not is_in_ssa(function_body)) {
    throw std::logic_error("Expression propagation can be run only after SSA. "
                           "It relies on variables not being redefined\n");
  }

  // Maintain map from variable name (packet or state variable)
  // to a string representing its expression, for expression propagation.
  std::map<std::string, std::string> var_to_expr;

  // Instantiate a visitor pass for recursively doing replacements on the rhs.
  ExprSubstVisitor subst_visitor(var_to_expr);

  // Rewrite function body
  std::string transformed_body = "";
  assert_exception(function_body);
  for (const auto &child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());

    // Strip off parenthesis and casts for LHS and RHS
    const auto *rhs = bin_op->getRHS()->IgnoreParenImpCasts();
    const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();

    // Populate var_to_expr, so long as
    // 1. The lhs variable doesn't appear within the rhs expression.
    // 2. The rhs expression isn't a (or doesn't contain a) DeclRefExpr
    // We don't want to be propagating DeclRefExpr's or ArraySubscriptExpr
    // because these are the syntactic construct for state variables and
    // propagating state variables destroys the property that state variables
    // are only ever read at the top of the program The partitiioning pass
    // relies on this property. ruijief: Essentially we don't want to flatten
    // the prologues and epilogues for the state var read/write flanks. ruijief:
    // In addition, we don't want to propagate the ternary statements which we
    // really just flattened.
    const auto &var_list = gen_var_list(rhs);
    if (var_list.find(clang_stmt_printer(lhs)) == var_list.end() and
        (not isa<DeclRefExpr>(rhs)) and (not isa<ArraySubscriptExpr>(rhs)) and
        (not isa<ConditionalOperator>(rhs))) {
      var_to_expr[clang_stmt_printer(lhs)] = clang_stmt_printer(rhs);
    }

    // ruijief: there used to be a isa<DeclRefExpr>(rhs) or ... here but it made
    // no sense so I deleted it.
    if ((not(isa<DeclRefExpr>(lhs)) and (not isa<ConditionalOperator>(rhs)) and
         isa<MemberExpr>(rhs)) and
        (var_to_expr.find(clang_stmt_printer(rhs)) != var_to_expr.end())) {
      // If rhs is a packet/state variable, replace it with its current expr
      transformed_body += clang_stmt_printer(lhs) + "=" +
                          var_to_expr.at(clang_stmt_printer(rhs)) + ";";
    } else if ((not(isa<DeclRefExpr>(lhs))) and
               (isa<ConditionalOperator>(rhs))) {
      // check if the condition inside the ITE has appeared before.
      const auto *ternary = dyn_cast<ConditionalOperator>(rhs);
      const auto *cond_expr = ternary->getCond()->IgnoreParenImpCasts();

      const std::string replaced_cond =
          subst_visitor.expr_visit_transform(_ctx, cond_expr);

      transformed_body += clang_stmt_printer(lhs) +
          "= (" + replaced_cond + ") ? " +
          clang_stmt_printer(ternary->getTrueExpr()) + " : " +
          clang_stmt_printer(ternary->getFalseExpr()) + ";";

    } else {
      // Pass through
      transformed_body +=
          clang_stmt_printer(lhs) + "=" + clang_stmt_printer(rhs) + ";";
    }
  }
  return std::make_pair("{" + transformed_body + "}",
                        std::vector<std::string>());
}
