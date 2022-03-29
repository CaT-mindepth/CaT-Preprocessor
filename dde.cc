#include "dde.h"

#include <functional>
#include <iostream>
#include <map>
#include <set>

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "context.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

std::string dde_transform(const TranslationUnitDecl *tu_decl) {
  // Accumulate all decls
  std::vector<const Decl *> all_decls;
  for (const auto *decl : dyn_cast<DeclContext>(tu_decl)->decls())
    all_decls.emplace_back(decl);

  // Sort all_decls
  std::stable_sort(all_decls.begin(), all_decls.end(),
                   [](const auto *decl1, const auto *decl2) {
                     return get_order(decl1) < get_order(decl2);
                   });

  // For each function decl inside the TU, find the packet
  // variables / state variables used by it and store the name somewhere.
  std::set<std::string> usedPktVars; // set of all used packet var names within
                                     // all functions in a TU.
  std::set<std::string> usedStateVars; // set of all used state var names within
                                       // all functions in a TU.
  const VariableTypeSelector pktVarSelector = {
      {VariableType::FUNCTION_PARAMETER, false},
      {VariableType::PACKET, true},
      {VariableType::STATE_ARRAY, false},
      {VariableType::STATE_SCALAR, false}};
  const VariableTypeSelector stateVarSelector = {
      {VariableType::FUNCTION_PARAMETER, false},
      {VariableType::PACKET, false},
      {VariableType::STATE_SCALAR, true},
      {VariableType::STATE_ARRAY, false}};

  std::string global_pkt_name = "";

  for (const auto *cdecl : all_decls) {
    if (isa<FunctionDecl>(cdecl)) {
      const auto *fdecl = dyn_cast<FunctionDecl>(cdecl);
      const auto *fbody = fdecl->getBody();

      auto pkt_name = clang_value_decl_printer(fdecl->getParamDecl(0));
      if (global_pkt_name == "")
        global_pkt_name = pkt_name;
      else
        assert_exception(global_pkt_name == pkt_name);

      std::set<std::string> pktVars = gen_var_list(fbody, pktVarSelector);
      std::set<std::string> stateVars = gen_var_list(fbody, stateVarSelector);
      usedPktVars.insert(pktVars.begin(), pktVars.end());
      usedStateVars.insert(stateVars.begin(), stateVars.end());
    }
  }

  //for (const auto &var : usedPktVars)
  //  std::cout << "used var: " << var << std::endl;

  // Loop through sorted vector of declarations

  std::string state_var_str = "";
  std::string scalar_func_str = "";
  std::string pkt_func_str = "";
  std::string record_decl_str = "";

  for (const auto *child_decl : all_decls) {
    assert_exception(child_decl);
    if (isa<VarDecl>(child_decl)) {
      // All state variables need to be preserved.
      state_var_str += clang_decl_printer(child_decl) + ";";
    } else if ((isa<FunctionDecl>(child_decl) and
                (not is_packet_func(dyn_cast<FunctionDecl>(child_decl))))) {
      scalar_func_str +=
          generate_scalar_func_def(dyn_cast<FunctionDecl>(child_decl));
    } else if (isa<FunctionDecl>(child_decl) and
               (is_packet_func(dyn_cast<FunctionDecl>(child_decl)))) {
      const auto *function_decl = dyn_cast<FunctionDecl>(child_decl);

      // Extract function signature
      assert_exception(function_decl->getNumParams() >= 1);
      const auto *pkt_param = function_decl->getParamDecl(0);
      const auto pkt_type =
          function_decl->getParamDecl(0)->getType().getAsString();
      const auto pkt_name = clang_value_decl_printer(pkt_param);

      // Rewrite function with new body
      pkt_func_str += function_decl->getReturnType().getAsString() + " " +
                      function_decl->getNameInfo().getName().getAsString() +
                      "( " + pkt_type + " " + pkt_name + ") " +
                      clang_stmt_printer(function_decl->getBody());
    } else if (isa<RecordDecl>(child_decl)) {
      // Open struct definition
      assert_exception(dyn_cast<RecordDecl>(child_decl)->isStruct());
      record_decl_str += "struct " +
                         dyn_cast<RecordDecl>(child_decl)->getNameAsString() +
                         "{\n";

      // acummulate current fields in struct
      for (const auto *field_decl :
           dyn_cast<DeclContext>(child_decl)->decls()) {
        std::string field_str =
            clang_value_decl_printer(dyn_cast<ValueDecl>(field_decl));
        // If this field is marked as indeed used in some function body, then
        // include it in the final printout. Otherwise, it is dead, so we
        // discard it.
        if (usedPktVars.find(global_pkt_name + "." + field_str) != usedPktVars.end() ||
            Context::GetContext().GetOptLevel(field_str) == D_NO_OPT) {
          record_decl_str +=
              dyn_cast<ValueDecl>(field_decl)->getType().getAsString() + " " +
              clang_value_decl_printer(dyn_cast<ValueDecl>(field_decl)) + ";";
        } //else
         // std::cout << "dde: eliminating unused variable " << field_str << "\n";
      }

      // Close struct definition
      record_decl_str += "};";
    }
  }
  return state_var_str + scalar_func_str + record_decl_str + pkt_func_str;
}
