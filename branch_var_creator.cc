#include "branch_var_creator.h"
#include "clang_utility_functions.h"
#include "context.h"

#include <functional>
#include <map>

using namespace clang;

std::string
branch_var_creator_transform(const clang::TranslationUnitDecl *tu_decl) {
  UniqueIdentifiers uid(identifier_census(tu_decl));
  return pkt_func_transform(tu_decl,
                            std::bind(&create_branch_var, std::placeholders::_1,
                                      std::placeholders::_2, uid));
}

std::pair<std::string, std::vector<std::string>>
create_branch_var(const clang::CompoundStmt *function_body,
                  const std::string &pkt_name, UniqueIdentifiers &uid) {
  // Note: need to schedule this pass after SSA.

  std::vector<std::string> new_decls;
  std::string out;

  Context &ctx = Context::GetContext();

  std::map<std::string, std::string> expr_to_var;

  for (const auto *child : function_body->children()) {
    if (isa<BinaryOperator>(child) &&
        isa<ConditionalOperator>(
            dyn_cast<BinaryOperator>(child)->getRHS()->IgnoreParenImpCasts())) {
      // record the ternary conditional.
      const auto *ternary = dyn_cast<ConditionalOperator>(
          dyn_cast<BinaryOperator>(child)->getRHS()->IgnoreParenImpCasts());
      const auto *condexpr = ternary->getCond()->IgnoreParenImpCasts();
      const std::string conditional = clang_stmt_printer(condexpr);

      bool cond_is_unary = false;
      std::string negated_expr = "";
      if (isa<UnaryOperator>(condexpr)) {
        cond_is_unary = true;
        negated_expr = clang_stmt_printer(dyn_cast<UnaryOperator>(condexpr)
                                              ->getSubExpr()
                                              ->IgnoreParenImpCasts());
      }

      const auto lhs = clang_stmt_printer(
          dyn_cast<BinaryOperator>(child)->getLHS()->IgnoreParenImpCasts());
      const auto rhs_texpr =
          clang_stmt_printer(ternary->getTrueExpr()->IgnoreParenImpCasts());
      const auto rhs_fexpr =
          clang_stmt_printer(ternary->getFalseExpr()->IgnoreParenImpCasts());

      // check if this conditional has been created before.
      // this is safe, because we're already in SSA
      if (expr_to_var.find(conditional) != expr_to_var.end()) {
        // do not create new branch temporary; instead, just use existing one.
        const std::string br_var_name = expr_to_var[conditional];
        out += lhs + " = " + br_var_name + " ? (" + rhs_texpr + ") : (" +
               rhs_fexpr + ");";
      } else if (cond_is_unary &&
                 expr_to_var.find(negated_expr) != expr_to_var.end()) {
        // do not create new branch temporary; instead, create a negated version
        // of an existing branch var.
        const std::string br_var_name = "!(" + expr_to_var[negated_expr] + ")";
        out += lhs + " = " + br_var_name + " ? (" + rhs_texpr + ") : (" +
               rhs_fexpr + ");";

      } else {
        // create new branch temporary.
        std::string tmp_var_name = uid.get_unique_identifier("_br_tmp");
        new_decls.push_back("int " + tmp_var_name + ";");

        ctx.SetType(tmp_var_name, D_BIT);
        ctx.SetVarKind(tmp_var_name, D_TMP);
        ctx.Derive(tmp_var_name, tmp_var_name);
        ctx.SetOptLevel(tmp_var_name, D_OPT);

        // modify the output program stream.
        const auto p_cond = pkt_name + "." + tmp_var_name;
        // update expr_to_var map.
        expr_to_var[conditional] = p_cond;
        
        out += p_cond + " = " + conditional + ";";
        out += lhs + " = " + p_cond + " ? (" + rhs_texpr + ") : (" + rhs_fexpr +
               ");";
      }
    } else {
      out += clang_stmt_printer(child) + ";";
    }
  }
  return std::make_pair("{" + out + "}", new_decls);
}
