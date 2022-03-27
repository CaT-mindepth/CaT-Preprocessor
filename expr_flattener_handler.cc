#include "expr_flattener_handler.h"

#include <iostream>

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

std::string
ExprFlattenerHandler::transform(const TranslationUnitDecl *tu_decl) {
  unique_identifiers_ = UniqueIdentifiers(identifier_census(tu_decl));
  return pkt_func_transform(
      tu_decl, std::bind(&ExprFlattenerHandler::flatten_body, this, _1, _2));
}

std::pair<std::string, std::vector<std::string>>
ExprFlattenerHandler::flatten_body(const Stmt *function_body,
                                   const std::string &pkt_name) const {
  assert_exception(function_body);

  std::string output = "";
  std::vector<std::string> new_decls = {};

  // std::cout << " expr_flattener: iterating through function " << pkt_name <<
  // std::endl; std::cout << "  contents: " << clang_stmt_printer(function_body)
  // << std::endl; iterate through function body
  assert_exception(isa<CompoundStmt>(function_body));
  for (const auto &child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());

    // query for type of LHS in context.
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    DominoType lhs_type;
    if (isa<MemberExpr>(lhs)) {
      const auto * lhs_decl = dyn_cast<MemberExpr>(lhs)->getMemberDecl();
      lhs_type = Context::GetContext().GetType(lhs_decl->getNameAsString());
    } else if (isa<DeclRefExpr>(lhs)) {
      lhs_type = Context::GetContext().GetType(clang_stmt_printer(lhs));
    } else {
      std::cout << "Error: encountered lhs that is neither MemberExpr nor DeclRefExpr. It is " << clang_expr_printer(lhs) << std::endl;
      assert_exception(false);
    }
    const auto ret = flatten(bin_op->getRHS(), pkt_name, lhs_type);

    // First add new definitions
    output += ret.new_defs;

    // Then add flattened expression
    output +=
        clang_stmt_printer(bin_op->getLHS()) + " = " + ret.flat_expr + ";";

    // Then append new pkt var declarations
    new_decls.insert(new_decls.end(), ret.new_decls.begin(),
                     ret.new_decls.end());
  }
  // std::cout << "  ...done\n";
  return make_pair("{" + output + "}", new_decls);
}

bool ExprFlattenerHandler::is_atomic_expr(const clang::Expr *expr) const {
  expr = expr->IgnoreParenImpCasts();
  return isa<DeclRefExpr>(expr) or isa<IntegerLiteral>(expr) or
         isa<MemberExpr>(expr) or isa<CallExpr>(expr) or
         isa<ArraySubscriptExpr>(expr);
}

bool ExprFlattenerHandler::is_flat(const clang::Expr *expr) const {
  expr = expr->IgnoreParenImpCasts();
  assert_exception(expr);
  if (isa<UnaryOperator>(expr)) {
    return is_atomic_expr(dyn_cast<UnaryOperator>(expr)->getSubExpr());
  } else if (isa<ConditionalOperator>(expr)) {
    const auto *cond_op = dyn_cast<ConditionalOperator>(expr);
    return is_atomic_expr(cond_op->getCond()) and
           is_atomic_expr(cond_op->getTrueExpr()) and
           is_atomic_expr(cond_op->getFalseExpr());
  } else if (isa<BinaryOperator>(expr)) {
    return is_atomic_expr(dyn_cast<BinaryOperator>(expr)->getLHS()) and
           is_atomic_expr(dyn_cast<BinaryOperator>(expr)->getRHS());
  } else {
    assert_exception(is_atomic_expr(expr));
    return true;
  }
}

FlattenResult ExprFlattenerHandler::flatten(const clang::Expr *expr,
                                            const std::string &pkt_name,
                                            DominoType expr_type) const {
  expr = expr->IgnoreParenImpCasts();
  if (is_flat(expr)) {
    return {clang_stmt_printer(expr), "", {}};
  } else {
    if (isa<ConditionalOperator>(expr)) {
      return flatten_cond_op(dyn_cast<ConditionalOperator>(expr), pkt_name,
                             expr_type);
    } else if (isa<BinaryOperator>(expr)) {
      return flatten_bin_op(dyn_cast<BinaryOperator>(expr), pkt_name,
                            expr_type);
    } else if (isa<UnaryOperator>(expr)) {
      return flatten_unary_op(dyn_cast<UnaryOperator>(expr), pkt_name,
                              expr_type);
    } else {
      std::cout << "ExprFlattenerHandler::flatten error: Expr is neither "
                   "conditional, binary, or unary. It is : "
                << clang_stmt_printer(expr) << std::endl;
      assert_exception(false);
      return {"", "", {}};
    }
  }
}

FlattenResult
ExprFlattenerHandler::flatten_unary_op(const clang::UnaryOperator *un_op,
                                       const std::string &pkt_name,
                                       DominoType expr_type) const {
  // Handle statements of the form `b = !a`. Check if `a` is flat, if `a` is
  // flat then we treat `b = !a` as flat. Otherwise, introduce temporary
  // variable to store the flattened result of a, and let `b` be the negated
  // value of that temporary.
  const auto *subExpr = un_op->getSubExpr();
  DominoType un_op_type =
      (un_op->getOpcode() == clang::UnaryOperatorKind::UO_LNot) ||
             (un_op->getOpcode() == clang::UnaryOperatorKind::UO_Not)
          ? D_BIT
          : D_INT;
  if (not is_flat(subExpr)) {
    const auto ret_subExpr =
        flatten_to_atomic_expr(subExpr, pkt_name, un_op_type);
    return {std::string(UnaryOperator::getOpcodeStr(un_op->getOpcode())) + "(" +
                ret_subExpr.flat_expr + ")",
            ret_subExpr.new_defs, ret_subExpr.new_decls};
  } else
    return {clang_stmt_printer(un_op), "", {}};
}

FlattenResult ExprFlattenerHandler::flatten_bin_op(const BinaryOperator *bin_op,
                                                   const std::string &pkt_name,
                                                   DominoType expr_type) const {
  assert_exception(not is_flat(bin_op));

  auto IsLogicalOperation = [](const BinaryOperatorKind b) {
    return b == BinaryOperatorKind::BO_LAnd || b == BinaryOperatorKind::BO_LOr;
  };

  DominoType bin_op_type = IsLogicalOperation(bin_op->getOpcode()) ? D_BIT : D_INT; 

  const auto ret_lhs = flatten_to_atomic_expr(bin_op->getLHS(), pkt_name, bin_op_type);
  const auto ret_rhs = flatten_to_atomic_expr(bin_op->getRHS(), pkt_name, bin_op_type);

  // Join all declarations
  std::vector<std::string> all_decls;
  all_decls.insert(all_decls.begin(), ret_lhs.new_decls.begin(),
                   ret_lhs.new_decls.end());
  all_decls.insert(all_decls.begin(), ret_rhs.new_decls.begin(),
                   ret_rhs.new_decls.end());

  return {ret_lhs.flat_expr +
              std::string(BinaryOperator::getOpcodeStr(bin_op->getOpcode())) +
              ret_rhs.flat_expr,
          ret_lhs.new_defs + ret_rhs.new_defs, all_decls};
}

FlattenResult
ExprFlattenerHandler::flatten_cond_op(const ConditionalOperator *cond_op,
                                      const std::string &pkt_name,
                                      DominoType expr_type) const {
  assert_exception(not is_flat(cond_op));
  const auto ret_cond = flatten_to_atomic_expr(cond_op->getCond(), pkt_name, D_BIT);
  const auto ret_true =
      flatten_to_atomic_expr(cond_op->getTrueExpr(), pkt_name, expr_type);
  const auto ret_false =
      flatten_to_atomic_expr(cond_op->getFalseExpr(), pkt_name, expr_type);

  // Join all declarations
  std::vector<std::string> all_decls;
  all_decls.insert(all_decls.begin(), ret_cond.new_decls.begin(),
                   ret_cond.new_decls.end());
  all_decls.insert(all_decls.begin(), ret_true.new_decls.begin(),
                   ret_true.new_decls.end());
  all_decls.insert(all_decls.begin(), ret_false.new_decls.begin(),
                   ret_false.new_decls.end());

  return {ret_cond.flat_expr + " ? " + ret_true.flat_expr + " : " +
              ret_false.flat_expr,
          ret_cond.new_defs + ret_true.new_defs + ret_false.new_defs,
          all_decls};
}

FlattenResult ExprFlattenerHandler::flatten_to_atomic_expr(
    const Expr *expr, const std::string &pkt_name, DominoType expr_type) const {
  if (is_atomic_expr(expr)) {
    return {clang_stmt_printer(expr), "", {}};
  } else {
    const auto flat_var_member = unique_identifiers_.get_unique_identifier();

    // Add flat_var_member to context.
    Context::GetContext().SetType(flat_var_member, expr_type);
    Context::GetContext().SetVarKind(flat_var_member, D_TMP);
    Context::GetContext().Derive(flat_var_member, flat_var_member);
    Context::GetContext().SetOptLevel(flat_var_member, D_OPT);

    // Add flat_var_member to running list of newly created decls.
    const auto flat_var_decl =
        expr->getType().getAsString() + " " + flat_var_member + ";";
    const auto pkt_flat_variable = pkt_name + "." + flat_var_member;
    const auto pkt_flat_var_def =
        pkt_flat_variable + " = " + clang_stmt_printer(expr) + ";";
    return {pkt_flat_variable, pkt_flat_var_def, {flat_var_decl}};
  }
}
