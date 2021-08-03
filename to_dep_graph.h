#ifndef TO_DEP_GRAPH_H_
#define TO_DEP_GRAPH_H_

#include "graph.h"
#include "dep_graph.h"
#include <clang/AST/AST.h>

#include <utility>
#include <vector>
#include <set>
#include <string>

// a pair type storing the return value of the
// function_to_dep_graph pass, which turns a function into a 
// dependency graph. The first argument stores the actual
// dependency graph, and the second argument stores the list of stateful
// variables inside that dependency graph.
typedef std::pair< Graph< const clang::BinaryOperator * >, std::set<const std::string> > DepGraphPassResult;

// Turns a function body into a dependency graph.
DepGraphPassResult function_to_dep_graph(const CompoundStmt *function_body);

// Converts a regular dependency graph coupled with a set of stateful
// variables into a SCC graph of components.
SCCGraph dep_graph_to_scc_graph(const DependencyGraph &dep_graph);

#endif // TO_DEP_GRAPH_H_