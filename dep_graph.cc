#include <iostream>
#include "clang/AST/AST.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "graph.h"
#include "clang_utility_functions.h"
#include "pkt_func_transform.h"
#include "single_pass.h"

using namespace clang;
using namespace clang::tooling;

// Print out dependency graph
static std::pair<std::string, std::vector<std::string>> dep_graph_transform(const CompoundStmt * function_body, const std::string & pkt_name __attribute__ ((unused))) {
  // Newly created packet temporaries
  std::vector<std::string> new_decls = {};

  // Verify that it's in SSA
  std::set<std::string> assigned_vars;
  for (const auto * child : function_body->children()) {
    assert(isa<BinaryOperator>(child));
    const auto * bin_op = dyn_cast<BinaryOperator>(child);
    assert(bin_op->isAssignmentOp());
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const auto pair = assigned_vars.emplace(clang_stmt_printer(lhs));
    if (pair.second == false) {
      throw std::logic_error("Program not in SSA form\n");
    }
  }

  // Dependency graph creation
  Graph<const BinaryOperator *> dep_graph(clang_stmt_printer);
  for (const auto * child : function_body->children()) {
    assert(isa<BinaryOperator>(child));
    dep_graph.add_node(dyn_cast<BinaryOperator>(child));
  }

  // Identify statements that read from and write to state
  // And create edges between them
  std::map<std::string, const BinaryOperator *> state_reads;
  std::map<std::string, const BinaryOperator *> state_writes;
  for (const auto * child : function_body->children()) {
    assert(isa<BinaryOperator>(child));
    const auto * bin_op = dyn_cast<BinaryOperator>(child);
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const auto * rhs = bin_op->getRHS()->IgnoreParenImpCasts();
    if (isa<DeclRefExpr>(rhs)) {
      state_reads[clang_stmt_printer(rhs)] = bin_op;
    } else if (isa<DeclRefExpr>(lhs)) {
      state_writes[clang_stmt_printer(lhs)] = bin_op;
      const auto state_var = clang_stmt_printer(lhs);
      dep_graph.add_edge(state_reads.at(state_var), state_writes.at(state_var));
      dep_graph.add_edge(state_writes.at(state_var), state_reads.at(state_var));
    }
  }
  std::cerr << dep_graph.dot_output() << std::endl;

  return std::make_pair(clang_stmt_printer(function_body), new_decls);
}

static llvm::cl::OptionCategory dep_graph(""
"Print out dependency graph of the program as a dot file");

int main(int argc, const char ** argv) {
  // Parser options
  CommonOptionsParser op(argc, argv, dep_graph);

  // Parse file once and output dot file
  std::cout << SinglePass<std::string>(op, std::bind(pkt_func_transform, std::placeholders::_1, dep_graph_transform)).output();
}