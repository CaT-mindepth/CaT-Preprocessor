#ifndef RENAME_PKT_FIELDS_H_
#define RENAME_PKT_FIELDS_H_

#include <set>
#include <string>
#include <utility>
#include <map>
#include "clang/AST/Expr.h"


/// Entry point for pass that rewrites packet fields from p.xxx to p_xxx
/// This renaming pass comes last in the frontend pass pipeline, and 
/// will prepare the SSA'd Domino code for being lowered into the mid-end
/// dependency and SCC graphs, and finally for Sketch synthesis.
std::string rename_pkt_fields_transform(const clang::TranslationUnitDecl * tu_decl);

/// Main function for packet
std::string rename_pkt_fields_body(const clang::CompoundStmt * function_body, const std::string & pkt_name __attribute__((unused)));


#endif // RENAME_PKT_FIELDS_H_
