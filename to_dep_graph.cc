// TODO: create a to_dep_graph.h file and include it here.
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include "clang_utility_functions.h"
#include "graph.h"
#include "set_idioms.h"
#include "unique_identifiers.h"
#include "util.h"

#include "third_party/assert_exception.h"

#include <map>
#include <set>
#include <vector>

using namespace clang;

// TODO: delegate this typedef statement to a corresponding (most appropriate)
// header file.
typedef Graph<const BinaryOperator *> ProgramGraph;

// op_reads_var: helper function for `depends`. Decides whether a
// given BinaryOperator expression reads from variable `var`.
static inline bool op_reads_var(const BinaryOperator *op, const Expr *var) {
  assert_exception(op);
  assert_exception(var);

  // We only check packet variables here because handle_state_vars
  // takes care of state variables.
  auto read_vars =
      gen_var_list(op->getRHS(), {{VariableType::PACKET, true},
                                  {VariableType::STATE_SCALAR, false},
                                  {VariableType::STATE_ARRAY, false}});

  // If the LHS is an array subscript expression, we need to check inside the
  // subscript as well
  if (isa<ArraySubscriptExpr>(op->getLHS())) {
    const auto *array_op = dyn_cast<ArraySubscriptExpr>(op->getLHS());
    const auto read_vars_lhs =
        gen_var_list(array_op->getIdx(), {{VariableType::PACKET, true},
                                          {VariableType::STATE_SCALAR, false},
                                          {VariableType::STATE_ARRAY, false}});
    read_vars = read_vars + read_vars_lhs;
  }

  return (read_vars.find(clang_stmt_printer(var)) != read_vars.end());
}

// Process dependency graph to build RAW dependencies.
// Given as input a vector of straight-line program statements, and
// a partially-constructed dependency graph. Outputs the modified
// dependency graph with RAW edges added.
Graph<const BinaryOperator *>
handle_state_vars(const std::vector<const BinaryOperator *> &stmt_vector,
                  const Graph<const BinaryOperator *> &dep_graph) {
  Graph<const BinaryOperator *> ret = dep_graph;
  std::map<std::string, const BinaryOperator *> state_reads;
  std::map<std::string, const BinaryOperator *> state_writes;
  for (const auto *stmt : stmt_vector) {
    const auto *lhs = stmt->getLHS()->IgnoreParenImpCasts();
    const auto *rhs = stmt->getRHS()->IgnoreParenImpCasts();
    // At this stage, after stateful_flanks has been run, the only
    // way state variables (scalar or array-based) appear is either on the LHS
    // or on the RHS and they appear by themselves (not as part of another
    // expression) Which is why we don't need to recursively traverse an AST to
    // check for state vars
    if (isa<DeclRefExpr>(rhs) or isa<ArraySubscriptExpr>(rhs)) {
      // Should see exactly one read to a state variable
      assert_exception(state_reads.find(clang_stmt_printer(rhs)) ==
                       state_reads.end());
      state_reads[clang_stmt_printer(rhs)] = stmt;
    } else if (isa<DeclRefExpr>(lhs) or isa<ArraySubscriptExpr>(lhs)) {
      // Should see exactly one write to a state variable
      assert_exception(state_writes.find(clang_stmt_printer(lhs)) ==
                       state_writes.end());
      state_writes[clang_stmt_printer(lhs)] = stmt;
      const auto state_var = clang_stmt_printer(lhs);
      // Check state_var exists in both maps
      assert_exception(state_reads.find(state_var) != state_reads.end());
      assert_exception(state_writes.find(state_var) != state_writes.end());
      ret.add_edge(state_reads.at(state_var), state_writes.at(state_var));
      ret.add_edge(state_writes.at(state_var), state_reads.at(state_var));
    }
  }

  // Check that there are pairs of reads and writes for every state variable
  for (const auto &pair : state_reads) {
    if (state_writes.find(pair.first) == state_writes.end()) {
      throw std::logic_error(pair.first +
                             " has a read that isn't paired with a write ");
    }
  }
  return ret;
}

// Parses RAW dependencies between two assignments.
bool depends(const BinaryOperator *op1, const BinaryOperator *op2) {
  // If op1 succeeds op2 in program order,
  // return false right away
  if (not(op1->getBeginLoc() < op2->getBeginLoc())) {
    return false;
  }

  // op1 writes the same variable that op2 writes (Write After Write)
  if (clang_stmt_printer(op1->getLHS()) == clang_stmt_printer(op2->getLHS())) {
    throw std::logic_error(
        "Cannot have Write-After-Write dependencies in SSA form from " +
        clang_stmt_printer(op1) + " to " + clang_stmt_printer(op2) + "\n");
  }

  // op1 reads a variable that op2 writes (Write After Read)
  if (op_reads_var(op1, op2->getLHS())) {
    // Make an exception for state variables. There is no way around this.
    // There is no need to add this edge, because handle_state_vars() does
    // this already.
    if (isa<DeclRefExpr>(op2->getLHS()) or
        isa<ArraySubscriptExpr>(op2->getLHS())) {
      return false;
    } else {
      throw std::logic_error(
          "Cannot have Write-After-Read dependencies in SSA form from " +
          clang_stmt_printer(op1) + " to " + clang_stmt_printer(op2) + "\n");
    }
  }

  // op1 writes a variable (LHS) that op2 reads. (Read After Write)
  return (op_reads_var(op2, op1->getLHS()));
}

// Convert a given function into a dependency graph.
ProgramGraph function_to_dep_graph(const CompoundStmt *function_body) {
  // Verify that it's in SSA
  if (not is_in_ssa(function_body)) {
    throw std::logic_error("Partitioning will run only after program is in SSA "
                           "form. This program isn't.");
  }
  // Append to a vector of const BinaryOperator *
  // in order of statement occurence.
  std::vector<const BinaryOperator *> stmt_vector;
  for (const auto *child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());
    stmt_vector.emplace_back(bin_op);
  }

  // Dependency graph creation
  Graph<const BinaryOperator *> dep_graph(clang_stmt_printer);
  for (const auto *stmt : stmt_vector) {
    dep_graph.add_node(stmt);
  }

  // Handle state variables specially
  dep_graph = handle_state_vars(stmt_vector, dep_graph);

  // Now add all Read After Write Dependencies, comparing a statement only with
  // a successor statement
  for (uint32_t i = 0; i < stmt_vector.size(); i++) {
    for (uint32_t j = i + 1; j < stmt_vector.size(); j++) {
      if (depends(stmt_vector.at(i), stmt_vector.at(j))) {
        dep_graph.add_edge(stmt_vector.at(i), stmt_vector.at(j));
      }
    }
  }

  // Eliminate nodes with no outgoing or incoming edge
  std::set<const BinaryOperator *> nodes_to_remove;
  for (const auto &node : dep_graph.node_set()) {
    if (dep_graph.pred_map().at(node).empty() and
        dep_graph.succ_map().at(node).empty()) {
      nodes_to_remove.emplace(node);
    }
  }
  for (const auto &node : nodes_to_remove) {
    dep_graph.remove_singleton_node(node);
  }

  std::cerr << dep_graph << std::endl;
  return dep_graph;
}

ProgramGraph dep_graph_to_scc_graph(const ProgramGraph& dep_graph) {
  // Condense (https://en.wikipedia.org/wiki/Strongly_connected_component)
  // dep_graph after collapsing strongly connected components into one node
  // Pass a function to order statements within the sccs
  const auto & condensed_graph = dep_graph.condensation([] (const BinaryOperator * op1, const BinaryOperator * op2)
                                                        {return op1->getBeginLoc() < op2->getBeginLoc();});
  return condensed_graph;
}