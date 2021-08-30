#ifndef REDUCE_VAR_TYPES_HELPER_H_
#define REDUCE_VAR_TYPES_HELPER_H_

#include <set>
#include <string>
#include <utility>
#include <map>
#include "clang/AST/Expr.h"


/// Help final processing pass (rename_pkt_fields) reduce variables of type 'bit' correctly by batch-renaming them.
std::string reduce_var_types_helper_transform(const clang::TranslationUnitDecl * tu_decl);

/// Main function for 
std::pair<std::string, std::vector<std::string>> reduce_var_types_helper_body(const clang::CompoundStmt * function_body, const std::string & pkt_name __attribute__((unused)));


#endif // CONST_PROP_H_
