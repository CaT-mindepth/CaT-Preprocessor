#ifndef ELIM_IDENTICAL_LHS_RHS_
#define ELIM_IDENTICAL_LHS_RHS_

#include "clang/AST/Decl.h"

std::string elim_identical_lhs_rhs_transform(const clang::TranslationUnitDecl * tu_decl);

/// Main function for DCE.
std::pair<std::string, std::vector<std::string>> 
    elim_identical_lhs_rhs(const clang::CompoundStmt * function_body,
                             const std::string & pkt_name __attribute__((unused)));

#endif