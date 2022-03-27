#ifndef INITIAL_PASS_H_
#define INITIAL_PASS_H_

#include <string>
#include <utility>
#include <set>

#include "clang/AST/Decl.h"


std::string initial_pass_transform(const clang::TranslationUnitDecl * tu_decl);


#endif