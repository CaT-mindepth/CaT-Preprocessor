#ifndef ELIM_TERNARY_H_
#define ELIM_TERNARY_H_

#include <string>
#include <utility>
#include "clang/AST/Expr.h"

// Entry point to ternary elimination
std::string ternary_transform(const clang::TranslationUnitDecl * tu_decl);

// Main function for CSE from a function body
std::pair<std::string, std::vector<std::string>> ternary_body(const clang::CompoundStmt * function_body, const std::string & pkt_name __attribute__ ((unused)));

#endif  // ELIM_TERNARY_H_
