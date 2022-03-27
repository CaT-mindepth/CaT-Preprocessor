#include "ite_ssa.h"

#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <utility>

#include "third_party/assert_exception.h"

#include "clang/AST/Expr.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

struct SSALine {
  std::string lhs;
  std::string rhs;
  SSALine(const std::string &lhs, const std::string &rhs) : lhs(lhs), rhs(rhs) {}
};

typedef std::pair<std::vector<SSALine>, std::vector<std::string>> BBSSAResult;

std::string ite_ssa_transform(const TranslationUnitDecl *tu_decl) {
  const auto &id_set = identifier_census(tu_decl);
  return pkt_func_transform(tu_decl,
                            std::bind(ite_ssa_rewrite_fn_body, _1, _2, id_set));
}

// Ruijie: The problem here is that clang AST isn't mutable, so we can't recursively
// re-name the RHSes of the given LHS when if-converting two "level" basic blocks. The
// solution seems to be to maintain some sort of a map from each line loc to a value number
// used for SSA.
// SSA a block of straight line code given previous definitions.

// TODO: Delete this once we're done.
BBSSAResult kaput_bb_ssa(const std::string & pkt_name, UniqueIdentifiers & unique_identifiers, std::vector<const BinaryOperator *> bb) {
  std::vector<std::string> new_decls = {};
  std::vector<SSALine> lines = {};

  // ruijie: we have to re-write this logic, but only store the renamed
  // varaiables in a map. This way, when we recursively merge two "level" BBs
  // we can just do operations on the map. The way we set up the map is
  // to map from a loc to a new name for the lhs corresponding to that location.
  size_t loc = 0; // 0-indexed from the starting line in the BB.
  std::vector<std::string> changeTo;
  // Map from packet variable name to replacement used for renaming
  std::map<std::string, std::string> current_packet_var_replacements;

  // Map from field name to replacement used for renaming
  // (mirrors current_packet_var_replacements, but instead of storing p.x,
  // we store x).
  std::map<std::string, std::string> current_packet_field_replacements;

  for (const auto * bin_op : bb) {
    // First rewrite RHS using whatever replacements we currently have
    const std::string rhs_str =
        replace_vars(bin_op->getRHS(), current_packet_var_replacements,
                     {{VariableType::STATE_SCALAR, false},
                      {VariableType::STATE_ARRAY, false},
                      {VariableType::PACKET, true}});

    // Now look at LHS
    const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const std::string lhs_var = clang_stmt_printer(lhs);

    if (isa<MemberExpr>(lhs)) {
      // Modify replacements
      const auto var_type =
          dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getType().getAsString();
      const auto new_tmp_var = unique_identifiers.get_unique_identifier(
          dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getNameAsString());
      const auto var_decl = var_type + " " + new_tmp_var + ";";
      new_decls.emplace_back(var_decl);
      current_packet_var_replacements[lhs_var] = pkt_name + "." + new_tmp_var;
      current_packet_field_replacements
          [dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getNameAsString()] =
              new_tmp_var;
    }

    // Now rewrite LHS
    const std::string lhs_str =
        replace_vars(bin_op->getLHS(), current_packet_var_replacements,
                     {{VariableType::STATE_SCALAR, false},
                      {VariableType::STATE_ARRAY, false},
                      {VariableType::PACKET, true}});

    SSALine currLine(lhs_str, rhs_str);
    lines.emplace_back(currLine);
  }
  return std::make_pair(lines, new_decls);
}

// bb_ssa: Return the SSA'd var names in a single basic block for merging later.
std::vector<std::string> bb_ssa(const std::string & pkt_name, UniqueIdentifiers & unique_identifiers, std::vector<const BinaryOperator *> bb) {
  // only stores the LHSes for the MemberExprs.
  std::vector<std::string> new_decls = {};
  for (const auto * bin_op : bb) {
    const auto *lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const std::string lhs_var = clang_stmt_printer(lhs);

    if (isa<MemberExpr>(lhs)) {
      const auto new_tmp_var = unique_identifiers.get_unique_identifier(
          dyn_cast<MemberExpr>(lhs)->getMemberDecl()->getNameAsString());
      new_decls.emplace_back(new_tmp_var);
    }
  }
  return new_decls;
}

void recursively_merge_ite_ssa(const std::string & pkt_name, UniqueIdentifiers & unique_identifiers, )

std::pair<std::string, std::vector<std::string>>
ite_ssa_rewrite_fn_body(const CompoundStmt *function_body,
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
      // Assert that this definition and current value of index show up in
      // def_locs
      const std::string lhs_var = clang_stmt_printer(lhs);
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
