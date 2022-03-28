#include "if_conversion_handler.h"

#include <iostream>

#include "third_party/assert_exception.h"

#include "algebraic_simplifier.h"
#include "clang_utility_functions.h"
#include "context.h"
#include "pkt_func_transform.h"

#include <functional>
#include <iostream>
using namespace std;

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

static ASTContext *_ctx;

std::string IfConversionHandler::transform(const TranslationUnitDecl *tu_decl) {
  _ctx = &(tu_decl->getASTContext());
  unique_identifiers_ = UniqueIdentifiers(identifier_census(tu_decl));
  return pkt_func_transform(
      tu_decl, std::bind(&IfConversionHandler::if_convert_body, this, _1, _2));
}

std::pair<std::string, std::vector<std::string>>
IfConversionHandler::if_convert_body(const Stmt *function_body,
                                     const std::string &pkt_name) const {
  assert_exception(function_body);

  std::string output_ = "";
  std::vector<std::string> new_decls_ = {};

  // 1 is the C representation for true
  if_convert(output_, new_decls_, "1", function_body, pkt_name);
  return make_pair("{" + output_ + "}", new_decls_);
}

void IfConversionHandler::if_convert(std::string &current_stream,
                                     std::vector<std::string> &current_decls,
                                     const std::string &predicate,
                                     const Stmt *stmt,
                                     const std::string &pkt_name) const {
  if (isa<CompoundStmt>(stmt)) {
    for (const auto &child : stmt->children()) {
      if_convert(current_stream, current_decls, predicate, child, pkt_name);
    }
  } else if (isa<IfStmt>(stmt)) {
    const auto *if_stmt = dyn_cast<IfStmt>(stmt);

    if (if_stmt->getConditionVariableDeclStmt()) {
      throw std::logic_error("We don't yet handle declarations within the test "
                             "portion of an if\n");
    }

    auto br_tmp_var = unique_identifiers_.get_unique_identifier("_br_tmp");
    auto pkt_br_tmp_var = pkt_name + "." + br_tmp_var;
    current_decls.push_back("int " + br_tmp_var + ";");
    Context::GetContext().SetType(br_tmp_var, D_BIT);
    Context::GetContext().SetVarKind(br_tmp_var, D_TMP);
    Context::GetContext().Derive(br_tmp_var, br_tmp_var);
    Context::GetContext().SetOptLevel(br_tmp_var, D_OPT);

    current_stream += pkt_br_tmp_var + " = " + clang_stmt_printer(if_stmt->getCond()) + ";";

    // Create temporary variable to hold the if condition
    // Create predicates for if and else block
    auto pred_within_if_block =
        predicate + " && (" + pkt_br_tmp_var + ")";
    auto pred_within_else_block = predicate + " && !(" + pkt_br_tmp_var + ")";

    // If convert statements within getThen block to ternary operators.
    if_convert(current_stream, current_decls, pred_within_if_block,
               if_stmt->getThen(), pkt_name);

    // If there is a getElse block, handle it recursively again
    if (if_stmt->getElse() != nullptr) {
      if_convert(current_stream, current_decls, pred_within_else_block,
                 if_stmt->getElse(), pkt_name);
    }
  } else if (isa<BinaryOperator>(stmt)) {
    std::cout << "if_convert: visiting statemenent " << clang_stmt_printer(stmt)
              << std::endl;
    assert_exception(!isa<DeclStmt>(stmt));
    current_stream +=
        if_convert_atomic_stmt(dyn_cast<BinaryOperator>(stmt), predicate);
  } else if (isa<DeclStmt>(stmt)) {
    // Just append statement as is, but check that this only happens at the
    // top level i.e. when predicate = "1" or true
    assert_exception(predicate == "1");
    current_stream += clang_stmt_printer(stmt);
    return;
  } else if (isa<NullStmt>(stmt)) {
    // Do nothing
    return;
  } else {
    throw std::logic_error("Cannot handle stmt " + clang_stmt_printer(stmt) +
                           " of type " + std::string(stmt->getStmtClassName()));
    assert_exception(false);
  }
}

std::string IfConversionHandler::if_convert_atomic_stmt(
    const BinaryOperator *stmt, const std::string &predicate) const {
  assert_exception(stmt);
  assert_exception(stmt->isAssignmentOp());
  assert_exception(not stmt->isCompoundAssignmentOp());

  // Create predicated version of BinaryOperator

  const std::string lhs =
      clang_stmt_printer(dyn_cast<BinaryOperator>(stmt)->getLHS());
  const std::string rhs = "(" + predicate + " ? (" +
                          clang_stmt_printer(stmt->getRHS()) + ") :  " + lhs +
                          ")";
  return (lhs + " = " + rhs + ";");
}
