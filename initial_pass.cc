#include "initial_pass.h"
#include "clang_utility_functions.h"

#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "context.h"

using namespace clang; 
using namespace std;

#include <iostream>

string initial_pass_transform(const clang::TranslationUnitDecl * tu_decl) {
  for (const auto *child_decl : dyn_cast<DeclContext>(tu_decl)->decls()) {
      if (isa<RecordDecl>(child_decl)) {
          for (const auto *field_decl : dyn_cast<DeclContext>(child_decl)->decls()) {
            const auto name = dyn_cast<FieldDecl>(field_decl)->getNameAsString();
            // cout << "encountered field decl : " << name  << "\n";
            Context::GetContext().SetType(name, D_INT);
            Context::GetContext().SetVarKind(name, D_PKT_FIELD);
            Context::GetContext().Derive(name, name);
          }

      } else if (isa<VarDecl>(child_decl)) {
        const auto * var = dyn_cast<VarDecl>(child_decl);
        const auto varName = var->getNameAsString();
        // cout << "encountered state var " << varName << "\n";
        Context::GetContext().SetType(varName, D_INT);
        Context::GetContext().SetVarKind(varName, D_STATEFUL);
        Context::GetContext().Derive(varName, varName);
      }
  }
  return clang_decl_printer(tu_decl);
}
