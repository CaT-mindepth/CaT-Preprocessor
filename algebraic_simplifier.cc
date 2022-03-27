#include "algebraic_simplifier.h"

#include "clang_utility_functions.h"
#include "third_party/assert_exception.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric> // std::accumulate
#include <queue>
using namespace clang;

std::string
AlgebraicSimplifier::ast_visit_bin_op(const clang::BinaryOperator *bin_op) {
  if (can_be_simplified(bin_op)) {
    return simplify_simple_bin_op(dyn_cast<clang::BinaryOperator>(bin_op->IgnoreParenImpCasts()));
  } else {
    return ast_visit_stmt(bin_op->getLHS()) +
           std::string(bin_op->getOpcodeStr()) +
           ast_visit_stmt(bin_op->getRHS());
  }
}

std::string AlgebraicSimplifier::ast_visit_cond_op(
    const clang::ConditionalOperator *cond_op) {
  // std::cout << "****** algebraic simplifier: ast_visit_cond_op called on stmt
  // " << clang_stmt_printer(cond_op) << std::endl;
  if (isa<IntegerLiteral>(cond_op->getCond()) and
      dyn_cast<IntegerLiteral>(cond_op->getCond())->getValue().getSExtValue() ==
          1) {
    return ast_visit_stmt(cond_op->getTrueExpr());
  } else if (isa<IntegerLiteral>(cond_op->getCond()) and
             dyn_cast<IntegerLiteral>(cond_op->getCond())
                     ->getValue()
                     .getSExtValue() == 0) {
    return ast_visit_stmt(cond_op->getFalseExpr());
  } else {
    return ast_visit_stmt(cond_op->getCond()) + " ? " +
           ast_visit_stmt(cond_op->getTrueExpr()) + " : " +
           ast_visit_stmt(cond_op->getFalseExpr());
  }
}

bool AlgebraicSimplifier::can_be_simplified(
    const BinaryOperator *bin_op) const {
  assert_exception(bin_op);
  return true; // TODO: delete this function
}

bool AlgebraicSimplifier::safe_to_convert(const clang::BinaryOperator *bin_op) {
  // Cannot contain ternary or assignment statements
  // bool stmt_type_census<T>(const clang::Stmt *stmt) returns true iff bin_op
  // contains stmt of type T.
  using clang::BinaryOperatorKind;
  std::set<clang::BinaryOperatorKind> unsafe_operators = {
      BO_Assign,    BO_AddAssign, BO_MulAssign, BO_DivAssign, BO_RemAssign,
      BO_SubAssign, BO_AndAssign, BO_OrAssign,  BO_XorAssign, BO_Cmp,
      BO_Comma,     BO_EQ,        BO_PtrMemD,   BO_PtrMemI,   BO_ShlAssign,
      BO_ShrAssign, BO_LT,        BO_GT,        BO_LE,        BO_GE};
  return //(!stmt_type_census<ConditionalOperator>(bin_op)) && // TODO:
         // commented out for compilation.
      (!binop_contains(bin_op, unsafe_operators));
}

std::string
AlgebraicSimplifier::ast_visit_un_op(const clang::UnaryOperator *un_op) {
  // handle double negations or double minus signs.
  // note that, here whether to use a subExpr with IgnoreParenImpCases
  // is critical, since the parentheses will be printed in the return result (or
  // not) depending on the choice.
  const auto *subExpr = un_op->getSubExpr()->IgnoreParenImpCasts();
  const auto opcode_str =
      std::string(UnaryOperator::getOpcodeStr(un_op->getOpcode()));
  if (!(isa<UnaryOperator>(subExpr)))
    return opcode_str + ast_visit_stmt(un_op->getSubExpr());
  const auto *sub_un_op = dyn_cast<UnaryOperator>(subExpr);
  const auto opcode = un_op->getOpcode();
  const auto sub_opcode = sub_un_op->getOpcode();
  if (opcode == sub_opcode) {
    // Double negation or double minus signs. We can now simplify them out.
    // Note that now we need to visit the sub-expression of sub_un_op, not
    // un_op.
    return ast_visit_stmt(sub_un_op->getSubExpr());
  } else {
    return opcode_str + ast_visit_stmt(un_op->getSubExpr());
  }
}

std::string
AlgebraicSimplifier::simplify_simple_bin_op(const BinaryOperator *bin_op) {
  assert_exception(can_be_simplified(bin_op));
  const auto opcode = bin_op->getOpcode();
  const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();
  const auto *rhs = bin_op->getRHS()->IgnoreParenImpCasts();

  std::cout << "algebra simplifier: simplifying binary operation "
            << clang_stmt_printer(bin_op) << std::endl;

  // Here lhs, rhs stand for left/right operand of a binary arithmetic
  // operation, not an assignment.
  // if RHS and LHS are constants then also calculate.
  // We use clang's built-in Expr evaluator to do this.
  // (Recall that BinaryOperator subclasses Expr, so it is
  // possible to call Expr::EvaluateAsInt directly on BinaryOperator).
  clang::Expr::EvalResult er, erLHS, erRHS;
  bool lhsIsConst, rhsIsConst;
  if (bin_op->EvaluateAsInt(er, *(this->ctx))) {
    std::cout << " ----- is an int!\n";
    return std::to_string(er.Val.getInt().getSExtValue());
  }

  // Simplify multiplicative identity.
  if (opcode == clang::BinaryOperatorKind::BO_Mul or
      opcode == clang::BinaryOperatorKind::BO_LAnd) {
    if (isa<IntegerLiteral>(lhs) and
        dyn_cast<IntegerLiteral>(lhs)->getValue().getSExtValue() == 1) {
      // 1 * anything = anything
      // 1 && anything = anything
      std::cout << "here : 1 && anything = anything\n"; 
      return ast_visit_stmt(bin_op->getRHS());
    } else if (isa<IntegerLiteral>(rhs) and
               dyn_cast<IntegerLiteral>(rhs)->getValue().getSExtValue() == 1) {
      // anything * 1 = anything
      // anything && 1 = anything
      std::cout << "here : 1 && anything = anything\n"; 

      return ast_visit_stmt(bin_op->getLHS());
    }
  }

  // Simplify additive identity.
  if (opcode == clang::BinaryOperatorKind::BO_Add or
      opcode == clang::BinaryOperatorKind::BO_Or) {
    if (isa<IntegerLiteral>(lhs) and
        dyn_cast<IntegerLiteral>(lhs)->getValue().getSExtValue() == 0) {
      // 0 + anything = anything
      // 0 ||  anything = anything
      return ast_visit_stmt(bin_op->getRHS());
    } else if (isa<IntegerLiteral>(rhs) and
               dyn_cast<IntegerLiteral>(rhs)->getValue().getSExtValue() == 0) {
      // anything + 0 = anything
      // anything || 0 = anything
      return ast_visit_stmt(bin_op->getLHS());
    }
  }

  // Simplify cases like A && A where A, A are the same,
  // or A || A where A and A are the same.
  if (opcode == clang::BinaryOperatorKind::BO_LAnd or
      opcode == clang::BinaryOperatorKind::BO_LOr) {
    std::cout << " ------ A||A or A&&A triggered\n";
    if (clang_stmt_printer(bin_op->getLHS()->IgnoreParenImpCasts()) ==
        clang_stmt_printer(bin_op->getRHS()->IgnoreParenImpCasts())) {
      return ast_visit_stmt(bin_op->getLHS());
    }
  }

  // Otherwise, if LHS or RHS are constexps, we evaluate them too.
  lhsIsConst = bin_op->EvaluateAsInt(erLHS, *(this->ctx));
  rhsIsConst = bin_op->EvaluateAsInt(erRHS, *(this->ctx));
  if (lhsIsConst) {
    long long lhsVal = erLHS.Val.getInt().getSExtValue();
    if (rhsIsConst) {
      long long rhsVal = erRHS.Val.getInt().getSExtValue();
      return std::to_string(lhsVal) + std::string(bin_op->getOpcodeStr()) +
             std::to_string(rhsVal);
    } else {
      return std::to_string(lhsVal) + std::string(bin_op->getOpcodeStr()) +
             ast_visit_stmt(bin_op->getRHS());
    }
  } else {
    if (rhsIsConst) {
      long long rhsVal = erRHS.Val.getInt().getSExtValue();
      return ast_visit_stmt(bin_op->getLHS()) +
             std::string(bin_op->getOpcodeStr()) + std::to_string(rhsVal);
    }
  }

  // Finally, try some rewrite rules using commutativity and associativity.
  // If an expression consists of only a single operator, then "flatten"
  // them out, eliminating any associativity problems.
  // It suffices to only do this locally, as any subexpression of this
  // expression will also obey the same property, so we fixpoint iterate until
  // convergence.
  //
  // Since traversing the expression tree is rather expensive when the
  // expression is long, we only simplify four basic operators for now.
  // std::cout << "algebra simplifier: trying rewrite rules" << std::endl;
  if (binop_contains_only(bin_op, {clang::BinaryOperatorKind::BO_Add}) ||
      binop_contains_only(bin_op, {clang::BinaryOperatorKind::BO_Mul}) ||
      binop_contains_only(bin_op, {clang::BinaryOperatorKind::BO_LOr}) ||
      binop_contains_only(bin_op, {clang::BinaryOperatorKind::BO_LAnd})) {
    // allVars is a set, so already lexicographic.
    // std::cout << "binop_contains_only: " << clang_stmt_printer(bin_op) <<
    // std::endl;
    const auto allVars =
        gen_var_list(bin_op, {{VariableType::FUNCTION_PARAMETER, true},
                              {VariableType::PACKET, true},
                              {VariableType::STATE_ARRAY, true},
                              {VariableType::STATE_SCALAR, true}});
    const auto allConsts = get_constants_in(bin_op);
    for (const auto & e : allConsts) {

      std::cout << " -> constant: " << e << std::endl;
    }
    
    long constEval = [allConsts, opcode]() {
      switch (opcode) {
        long c;
      case clang::BinaryOperatorKind::BO_Add: {
        c = 0;
        for (const auto &e : allConsts) {
          c += strtol(e.c_str(), NULL, 10);
        }
        return c;
      }
      case clang::BinaryOperatorKind::BO_LAnd: {
        c = 1;
        for (const auto &e : allConsts) {
          c = c && strtol(e.c_str(), NULL, 10);
        }
        return c;
      }
      case clang::BinaryOperatorKind::BO_Mul: {
        c = 1;
        for (const auto &e : allConsts) {
          c *= strtol(e.c_str(), NULL, 10);
        }
        return c;
      }
      case clang::BinaryOperatorKind::BO_LOr: {
        c = 0;
        for (const auto &e : allConsts) {
          c = c || strtol(e.c_str(), NULL, 10);
        }
        return c;
      }
      default:
        assert_exception(false);
      }
    }();

    std::string ret = ""; 
    int noConsts = !(allConsts.size() > 0);
    if (!noConsts)
     ret = std::to_string(constEval);    
    for (const auto &vs : allVars) {
      if (noConsts) {
        ret += vs;
        noConsts = 0;
      } else 
        ret += bin_op->getOpcodeStr().str() + vs;
    }
    std::cout << "  result of rewriting: " << ret << std::endl;
    return ret;
  }

  // c*sum(x1,x2,...) = sum(c*x1,c*x2,..)
  // if (opcode == clang::BinaryOperatorKind::BO_Mul && isa<IntegerLiteral>(lhs)
  // && binop_contains_only(rhs, {clang::BinaryOperatorKind::BO_Add})) {
  //
  //}
  // Resort operands so that they follow lexicographic order
  // when the operator is commutative. This lexicographically
  // sorts an entire subexpression when called in fixpoint iteration style.

  // Three cases: a + b, a + C, C + a, where a, b are vars and C is const.
  // a + b: compare a, b lexicographically.

  const auto isValOrConst = [](const Expr *e) {
    return (isa<clang::IntegerLiteral>(e) || isa<clang::MemberExpr>(e) ||
            isa<clang::DeclRefExpr>(e));
  };
  if (isValOrConst(lhs) && isValOrConst(rhs) &&
      (opcode == clang::BinaryOperatorKind::BO_Add ||
       opcode == clang::BinaryOperatorKind::BO_Mul ||
       opcode == clang::BinaryOperatorKind::BO_LAnd ||
       opcode == clang::BinaryOperatorKind::BO_LOr)) {

    const auto lhsStr = ast_visit_stmt(lhs);
    const auto rhsStr = ast_visit_stmt(rhs);
    // lexicographic comparison.
    if (lhsStr <= rhsStr)
      return lhsStr + std::string(bin_op->getOpcodeStr()) + rhsStr;
    else
      return rhsStr + std::string(bin_op->getOpcodeStr()) + lhsStr;
  }

  return ast_visit_stmt(bin_op->getLHS()) +
         std::string(bin_op->getOpcodeStr()) + ast_visit_stmt(bin_op->getRHS());
}
