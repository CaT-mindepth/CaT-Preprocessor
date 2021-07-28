#include "cse.h"

#include <map>

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"

using namespace clang;

std::string cse_transform(const TranslationUnitDecl * tu_decl) {
  return pkt_func_transform(tu_decl, cse_body);
}
// CSE works in conjunction with CSI; 
// First CSI identifies statements whose RHS are entirely identical
// and then CSE eliminates them using the below procedure.
std::pair<std::string, std::vector<std::string>> cse_body(const clang::CompoundStmt * function_body, const std::string & pkt_name __attribute__((unused))) {
  std::string transformed_body = "";
  std::map<std::string, bool> assigned;

  for (const auto * child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    assert_exception(dyn_cast<BinaryOperator>(child)->isAssignmentOp());
    const auto * bin_op = dyn_cast<BinaryOperator>(child);
    // Get the left/right-hand-side of the assignment operation.
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const auto * rhs = bin_op->getRHS()->IgnoreParenImpCasts();
    // If the left hand side (expression) is not in the map, add it into the map, 
    // and promot the left operand into a variable.
    if (assigned.find(clang_stmt_printer(lhs)) == assigned.end()) {
      assigned[clang_stmt_printer(lhs)] = true;
      transformed_body += clang_stmt_printer(lhs) + "=" + clang_stmt_printer(rhs) + ";";
    }
    // Otherwise, the LHS is already assigned in the map. We can safely ignore it,
    // since it is already defined.
  }
  return std::make_pair("{" + transformed_body + "}", std::vector<std::string>());
}
