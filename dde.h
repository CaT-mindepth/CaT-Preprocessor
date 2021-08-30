#ifndef DDE_H_
#define DDE_H_

#include <set>
#include <string>
#include <utility>
#include <map>
#include "clang/AST/Expr.h"


/// Dead Declarations Elimination: Eliminate unused packet variables
/// as well as state variables in a translation unit.
std::string dde_transform(const clang::TranslationUnitDecl * tu_decl);


#endif // DCE_H_
