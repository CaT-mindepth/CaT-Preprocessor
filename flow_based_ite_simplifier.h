#ifndef FLOW_BASED_ITE_SIMPLIFIER_H_
#define FLOW_BASED_ITE_SIMPLIFIER_H_

#include <map>
#include <string>
#include <utility>
#include <iostream>

#include "ast_visitor.h"
#include "clang/AST/AST.h"
#include "clang_utility_functions.h"

/// Entry point from SinglePass to flow-based ITE simplifier, calls expr_prop_fn_body immediately
std::string flow_based_ite_simplify_transform(const clang::TranslationUnitDecl *tu_decl);

/// Flow-based branch condition simplification:
/// because we do SSA after if-conversion, we end up value-numbering sequentially
/// along a flattened control flow, which gives us a overapproximate value number for some program branch
/// points. Nested-ifs encountered in the Chipmunk program mutations exacerbate this effect.
///
/// A simple hack to overcome this issue stems from the the observation that there is
/// a Domino semantic restriction that forbids concurrent access of a stateful transaction's old and new
/// values at once. This means any branch temporary of form (p <OP1> read_flank) <BOP> (p <OP1> write_flank)
/// where <OP1> equals <OP2> and <BOP> is a boolean logic operation (AND, OR) can be simplified into
/// only (p <OP1> read_flank). Due to the Domino semantic restriction, this is a sound transformation.
/// 
/// To avoid touching additional corner-cases, this is implemented as a final fix-up pass of our preprocessor
/// (scheduled right before outputting the preprocessed file).
std::pair<std::string, std::vector<std::string>>
flow_based_ite_simplifier_transform_body(const clang::CompoundStmt *function_body,
                  const std::string &pkt_name __attribute__((unused)),
                  clang::ASTContext * _ctx);


#endif // FLOW_BASED_ITE_SIMPLIFIER_H_
