#include "ssa.h"

#include <functional>
#include <iostream>
#include <set>
#include <string>

#include "third_party/assert_exception.h"

#include "clang/AST/Expr.h"

#include "clang_utility_functions.h"
#include "context.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

std::string ssa_transform(const TranslationUnitDecl *tu_decl) {
  const auto &id_set = identifier_census(tu_decl);
  return pkt_func_transform(tu_decl,
                            std::bind(ssa_rewrite_fn_body, _1, _2, id_set));
}

std::pair<std::string, std::vector<std::string>>
ssa_rewrite_fn_body(const CompoundStmt *function_body,
                    const std::string &pkt_name,
                    const std::set<std::string> &id_set) {
  // Vector of newly created packet temporaries
  std::vector<std::string> new_decls = {};

  // Create unique identifier set
  UniqueIdentifiers unique_identifiers(id_set);

  // All indices where every packet variable is defined/written/is on the LHS.
  // We choose to rename ALL definitions/writes of a packet variable
  // rather than just the redefinitions because there might
  // be reads preceding the first definition/write
  // and it's correct to rename all definitions.
  std::map<std::string, std::vector<int>> def_locs;
  int index = 0;
  for (const auto *child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());

    const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();

    if (isa<MemberExpr>(lhs)) {
      if (def_locs.find(clang_stmt_printer(lhs)) == def_locs.end()) {
        def_locs[clang_stmt_printer(lhs)] = {index};
      } else {
        def_locs.at(clang_stmt_printer(lhs)).emplace_back(index);
      }
    }
    index++;
  }

  // Now, do the renaming, storing output in function_body
  std::string function_body_str;
  index = 0;

  // Map from packet variable name to replacement used for renaming
  std::map<std::string, std::string> current_packet_var_replacements;

  // Map from field name to replacement used for renaming
  // (mirrors current_packet_var_replacements, but instead of storing p.x,
  // we store x).
  std::map<std::string, std::string> current_packet_field_replacements;

  for (const auto *child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto *bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());

    // First rewrite RHS using whatever replacements we currently have
    const std::string rhs_str =
        replace_vars(bin_op->getRHS(), current_packet_var_replacements,
                     {{VariableType::STATE_SCALAR, false},
                      {VariableType::STATE_ARRAY, false},
                      {VariableType::PACKET, true}});

    // Now look at LHS
    const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    if (isa<MemberExpr>(lhs)) {
      const auto *lhsPkt = dyn_cast<MemberExpr>(lhs)->getMemberDecl();
      const std::string lhs_var = clang_stmt_printer(lhs);
      const std::string lhs_field = lhsPkt->getNameAsString();
   //   if (!(Context::GetContext().GetVarKind(lhs_var) == D_STATEFUL)) {
        // Assert that this definition and current value of index show up in
        // def_locs lhs_var is the base of SSA string.
        assert_exception(def_locs.find(lhs_var) != def_locs.end());
        assert_exception(std::find(def_locs.at(lhs_var).begin(),
                                   def_locs.at(lhs_var).end(),
                                   index) != def_locs.at(lhs_var).end());

        // Modify replacements
        const auto var_type =
            dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getType().getAsString();
        const auto new_tmp_var = unique_identifiers.get_unique_identifier(
            dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getNameAsString());
        const auto var_decl = var_type + " " + new_tmp_var + ";";

        // add to context
        // var kind: if lhs is a D_TMP, we inherit the kind. Otherwise
        // we mark newly created var as a PKT_FIELD, because we don't want to destroy
        // read/write flanks.
         std::cout << "getting type of var " << lhs_var << "\n";
        const auto var_domino_type = Context::GetContext().GetType(lhs_field);
        const auto var_domino_kind = Context::GetContext().GetVarKind(lhs_field);

        Context::GetContext().SetType(new_tmp_var, var_domino_type);
        Context::GetContext().SetVarKind(new_tmp_var, var_domino_kind == D_TMP ? D_TMP : D_PKT_FIELD);
        Context::GetContext().Derive(lhs_field, new_tmp_var);
        // if a variable is a tmp, mark it as OPT.
        if (var_domino_kind == D_TMP)
          Context::GetContext().SetOptLevel(new_tmp_var, D_OPT);
        
        new_decls.emplace_back(var_decl);
        current_packet_var_replacements[lhs_var] = pkt_name + "." + new_tmp_var;
        current_packet_field_replacements
            [dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getNameAsString()] =
                new_tmp_var;
    }

    // Now rewrite LHS
    assert_exception(bin_op->getLHS());
    const std::string lhs_str =
        replace_vars(bin_op->getLHS(), current_packet_var_replacements,
                     {{VariableType::STATE_SCALAR, false},
                      {VariableType::STATE_ARRAY, false},
                      {VariableType::PACKET, true}});
    function_body_str += lhs_str + " = " + rhs_str + ";";
    index++;
  }

  // Print out the final replacements, i.e. the value of
  // current_packet_var_replacements at this point
  for (const auto &repl_pair : current_packet_field_replacements)
    std::cerr << "// " + repl_pair.first << " " << repl_pair.second
              << std::endl;

  return std::make_pair("{" + function_body_str + "}", new_decls);
}
