#ifndef DEP_GRAPH_H_
#define DEP_GRAPH_H_

#include "graph.h"
#include "clang_utility_functions.h"
#include <clang/AST/AST.h>

using namespace clang;

// Helper types for defining a dependency graph and an SCC-graph.

// A DependencyGraph object is formed on individual statements, and
// takes into account RAW (read-after-write) dependencies.
typedef Graph<const clang::BinaryOperator *> DependencyGraph;

// A Component object is a node upon which the SCCGraph is formed.
struct Component {
  Component(std::vector<const clang::BinaryOperator *> stmts, unsigned id)
      : stmts_(stmts), id_(id) {
        std::set<std::string> stateVars;
        for (const auto * stmt : stmts) {
          const auto & rtnResult = gen_var_list(stmt, {{VariableType::PACKET, false},
                                    {VariableType::STATE_SCALAR, true},
                                    {VariableType::STATE_ARRAY, true}});
          stateVars.insert(rtnResult.begin(), rtnResult.end());
        }
        if (stateVars.size() > 0) {
          this->statefulVars_ = stateVars;
          this->isStateful_ = true;
        } else {
          this->isStateful_ = false;
        }
        // Compute inputs and outputs for the current component.
        this->getInputsOutputs();
      }
  
  // Retrieves the UID for the current component.
  unsigned ID() const {
    return this->id_;
  }

  // Resets the UID for the current component.
  void setID(unsigned id) {
    this->id_ = id;
  }

  // Queries whether the current component is a stateful one (involves operation
  // on a stateful variable) or a stateless one.
  bool IsStateful() const { return this->isStateful_; }

  // Does the current operation contain a ternary statement?
  // This is needed to e.g. decide what sort of grammar to use during symthesis.
  // TODO write code to make this work.
  bool ContainsBranch() {
    //  for (const auto * stmt : this->stmts_) {
    //
    //  }
    return false; // TODO
  }

  // Retrieves the variables that are inputs to the current component.
  std::set<std::string> GetInputs() const {
    return this->inputs_;
  }

  // Retrieves the variables that are outputs to the current component.
  std::set<std::string> GetOutputs() const {
    return this->outputs_;
  }

  // Retrieves the list of stmts in the current component.
  auto getStmts() const {
    return this->stmts_;
  }

  // Component merging: This happens as "step 2" in the HotNets paper for CaT, where 
  // we try to pack more stateless code into stateful components.
  // Some simple logic here to handle merging of components.
  // TODO: write a pass over the SCC Graph that actually merges mergeable components. 
  // TODO: Maybe what we can do here is code a method called
  // "Merge" that actually handles the merging process.
  // 
  // We can overload the "+"-operator to handle the merging process. 
  // source: https://en.cppreference.com/w/cpp/language/operators
  //
  // Note that, addition here is _asymmetric_: By default "that" is concatenated into
  // the bottom of "this". To reverse the behavior, you can just call operator+ on
  // another component. 
  //
  // Also note that the returned component will have `this`'s ID.
  Component operator+(const Component & that) const {
    // We cannot merge two stateless components, that just cannot happen.
    // Indeed; either this is stateful or that is stateful.
    assert_exception((this->IsStateful() || that.IsStateful()));
    std::vector<const clang::BinaryOperator *> allStmts;
    const auto & thisStmts = this->getStmts();
    const auto & thatStmts = that.getStmts();
    for (const auto & stmt : thisStmts) {
      allStmts.push_back(stmt);
    }
    for (const auto & stmt : thatStmts) {
      allStmts.push_back(stmt);
    }
    return Component(allStmts, this->id_);
  }
  
  // graph<T> requires T to be sortable. Hence we overload the < operator.
  bool operator<(const Component & that) const {
    const auto & thisStmts = this->getStmts();
    const auto & thatStmts = that.getStmts();
    if (thisStmts.size() != thatStmts.size()) {
      return thisStmts.size() < thatStmts.size();
    } else {
      // lexicographically compare statements.
      size_t it = 0;

      for (it = 0; it < thisStmts.size(); ++it) {
        const auto & thisStmt = thisStmts[it];
        const auto & thatStmt = thatStmts[it];
        if (thisStmt != thatStmt) return thisStmt < thatStmt;
      }
      return false; // they are equal
    }
  }

  // Equality between two Component objects solely determined by their contents.
  bool operator==(const Component & that) {
    if (this->getStmts().size() != that.getStmts().size()) 
      return false;
    size_t it = 0;
    for (it = 0; it < this->getStmts().size(); ++it) {
      if (this->getStmts()[it] != that.getStmts()[it])
        return false;
    }
    return true;
  }

private:
  std::vector<const clang::BinaryOperator *> stmts_;
  bool isStateful_;
  std::set<std::string> statefulVars_;
  std::set<std::string> inputs_;
  std::set<std::string> outputs_;
  unsigned id_;

  // Gets a list of all defines (i.e. variables in the LHSes)
  // in the statement block.
  std::set<std::string> getDefines() {
    std::set<std::string> defines;
    for (const auto * stmt : this->stmts_) {
      // after all the compiler passes, each statement must be a 
      // BinaryOperator that is an assignment operation.
      assert_exception(isa<BinaryOperator>(stmt));
      const auto * bin_op = dyn_cast<BinaryOperator>(stmt);
      assert_exception(bin_op->isAssignmentOp());
      defines.emplace(clang_stmt_printer(bin_op->getLHS()->IgnoreParenImpCasts()));
    }
    return defines;
  }

  // Get a list of all uses (i.e. variables in the RHSes)
  // in the statement block.
  std::set<std::string> getUses() {
    std::set<std::string> uses;
    for (const auto * stmt : this->stmts_) {
      // after all the compiler passes, each statement must be a 
      // BinaryOperator that is an assignment operation.
      assert_exception(isa<BinaryOperator>(stmt));
      const auto * bin_op = dyn_cast<BinaryOperator>(stmt);
      assert_exception(bin_op->isAssignmentOp());
      // Now, we use a utility function to get all variables on the RHS.
      // Note that, we indeed get _all_ variables (incl. stateful, stateless vars) on the RHS.
      const auto * rhs = bin_op->getRHS()->IgnoreParenImpCasts();
      std::set<std::string> rhsUses = gen_var_list(rhs, {{VariableType::PACKET, true},
                                    {VariableType::STATE_SCALAR, true},
                                    {VariableType::STATE_ARRAY, true}});
      // `uses` takes the union over all `rhsUses`.
      uses.insert(rhsUses.begin(), rhsUses.end());
    }
    return uses;
  }

  // Computes input and output variables for this component.
  // Called by the constructor.
  void getInputsOutputs() {
    // get defines and uses.
    const auto & defines = this->getDefines();
    const auto & uses = this->getUses();

    // Inputs are exactly the _uses without defines_.
    for (const auto & use_var : uses) {
      if (defines.find(use_var) == defines.end()) {
        this->inputs_.emplace(use_var);
      }
    }
    // Outputs are exactly the _defines without uses_.
    for (const auto & define_var : defines) {
      if (uses.find(define_var) == uses.end()) {
        this->outputs_.emplace(define_var);;
      }
    }
  }
};

// A SCCGraph object is formed by condensing (see the condense method below)
// a DependencyGraph object.
typedef Graph<Component> SCCGraph;

// a node_printer function for SCCGraph.
std::string component_printer(const Component & comp) {
  std::string s = "comp_" + std::to_string(comp.ID()) + " {";
  const auto & stmt_list = comp.getStmts();
  for (const auto * stmt : stmt_list) {
    s += clang_stmt_printer(stmt);
  }
  s += "}";
  return s;
}

#endif // DEP_GRAPH_H_