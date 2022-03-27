#ifndef PAREN_REMOVER_H_
#define PAREN_REMOVER_H_

#include "compiler_pass.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"
#include "clang/AST/Expr.h"

#include <string>
#include <utility>
#include <vector>

std::string paren_remover_transform(const clang::TranslationUnitDecl *tu_decl);

std::pair<std::string, std::vector<std::string>>
paren_remover(const clang::CompoundStmt *function_body,
              const std::string &pkt_name);

#endif