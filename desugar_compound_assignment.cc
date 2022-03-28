#include "desugar_compound_assignment.h"

#include "third_party/assert_exception.h"
#include "clang_utility_functions.h"
#include <iostream>
using namespace clang;

BinaryOperator::Opcode DesugarCompAssignment::get_underlying_op(const BinaryOperator::Opcode & comp_asgn_op) const {
  // This is a clang quirk:
  // opcodes for all operators with compound assignment variants
  // aren't contiguous.

  if (comp_asgn_op == BinaryOperator::Opcode::BO_AddAssign) return BinaryOperator::Opcode::BO_Add;
  else if (comp_asgn_op == BinaryOperator::Opcode::BO_MulAssign) return BinaryOperator::Opcode::BO_Mul;
  else if (comp_asgn_op == BinaryOperator::Opcode::BO_SubAssign) return BinaryOperator::Opcode::BO_Sub;
  else if (comp_asgn_op == BinaryOperator::Opcode::BO_DivAssign) return BinaryOperator::Opcode::BO_Div;
  else if (comp_asgn_op == BinaryOperator::Opcode::BO_AndAssign) return BinaryOperator::Opcode::BO_And;
  else if (comp_asgn_op == BinaryOperator::Opcode::BO_OrAssign) return BinaryOperator::Opcode::BO_Or;
  else {
        throw std::logic_error("Got opcode that get_underlying_op can't handle: " + std::string(BinaryOperator::getOpcodeStr(comp_asgn_op)));
  }
}

std::string DesugarCompAssignment::ast_visit_bin_op(const BinaryOperator * bin_op) {
  assert_exception(bin_op);
  std::string ret;
  if (isa<CompoundAssignOperator>(bin_op)) {
   // std::cout << "visiting compound assignment operator " << clang_stmt_printer(bin_op) << std::endl;
    // Handle compound assignments alone
    const auto s = ast_visit_stmt(bin_op->getLHS()) + "=" + ast_visit_stmt(bin_op->getLHS())
           + std::string(BinaryOperator::getOpcodeStr(get_underlying_op(bin_op->getOpcode())))
           + ast_visit_stmt(bin_op->getRHS());

   // std::cout << " --> changed to: " << s << std::endl;
    return s;
  } else {
    // Delegate to default base class method
    return AstVisitor::ast_visit_bin_op(bin_op);
  }
}
