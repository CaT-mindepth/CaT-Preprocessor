#ifndef _BRANCH_VAR_CREATOR
#define _BRANCH_VAR_CREATOR

#include "compiler_pass.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"
#include "clang/AST/Expr.h"

#include <string>
#include <utility>
#include <vector>

std::string
branch_var_creator_transform(const clang::TranslationUnitDecl *tu_decl);

std::pair<std::string, std::vector<std::string>>
create_branch_var(const clang::CompoundStmt *function_body,
                  const std::string &pkt_name, UniqueIdentifiers &uid);

#endif