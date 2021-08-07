#include "rename_pkt_fields.h"

#include <functional>
#include <iostream>

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"
#include "unique_identifiers.h"

using namespace clang;
using std::placeholders::_1;
using std::placeholders::_2;

std::string rename_pkt_fields_transform(const TranslationUnitDecl *tu_decl) {

  const auto &id_set = identifier_census(tu_decl);

  // Storage for returned string
  std::string ret;

  // Create unique identifier generator
  UniqueIdentifiers unique_identifiers(id_set);

  // Identifiers census
  std::cout
      << "rename_pkt_fields_transform: processing packet and stateful vars: "
      << std::endl;
  const VariableTypeSelector packetVarsSel = {
      {VariableType::STATE_SCALAR, false},
      {VariableType::STATE_ARRAY, false},
      {VariableType::PACKET, true},
      {VariableType::FUNCTION_PARAMETER, false}};

  const VariableTypeSelector statefulVarsSel = {
      {VariableType::STATE_SCALAR, true},
      {VariableType::STATE_ARRAY, true},
      {VariableType::PACKET, false},
      {VariableType::FUNCTION_PARAMETER, false}};

  std::set<std::string> stateVars = identifier_census(tu_decl, statefulVarsSel);
  std::set<std::string> branchVars;
  std::set<std::string> statelessVars =
      identifier_census(tu_decl, packetVarsSel);
  std::cout << "rename_pkt_fields_transform: outputting vars..." << std::endl;
  for (const auto &statelessVar : statelessVars) {
    if (statelessVar.rfind("_br", 0) != std::string::npos) {
      branchVars.insert(statelessVar);
    } else {
      std::cout << "int p_" << statelessVar << ";" << std::endl;
    }
  }
  std::cout << "# state variables start" << std::endl;
  for (const auto &stateVar : stateVars) {
    std::cout << "int " << stateVar << std::endl;
  }
  std::cout << "# state variables end" << std::endl;
  for (const auto & branchVar : branchVars) {
    std::cout << "bit p_" << branchVar << ";" << std::endl;
  }
  std::cout << "# declarations end" << std::endl;
  for (const auto *child_decl : dyn_cast<DeclContext>(tu_decl)->decls()) {
    assert_exception(child_decl);
    if (isa<VarDecl>(child_decl) or isa<RecordDecl>(child_decl)) {
      // Pass through these declarations as is
      // ret += clang_decl_printer(child_decl) + ";";
      continue;
    } else if (isa<FunctionDecl>(child_decl) and
               (not is_packet_func(dyn_cast<FunctionDecl>(child_decl)))) {
      // ret += generate_scalar_func_def(dyn_cast<FunctionDecl>(child_decl));
      continue;
    } else if (isa<FunctionDecl>(child_decl) and
               (is_packet_func(dyn_cast<FunctionDecl>(child_decl)))) {
      const auto *function_decl = dyn_cast<FunctionDecl>(child_decl);

      // Extract function signature
      assert_exception(function_decl->getNumParams() >= 1);
      const auto *pkt_param = function_decl->getParamDecl(0);
      const auto pkt_type =
          function_decl->getParamDecl(0)->getType().getAsString();
      const auto pkt_name = clang_value_decl_printer(pkt_param);

      ret += rename_pkt_fields_body(
          dyn_cast<clang::CompoundStmt>(function_decl->getBody()), pkt_name);
    }
  }
  std::cout << ret << std::endl;
  return ret;
}

std::string rename_pkt_fields_body(const clang::CompoundStmt *function_body,
                                   const std::string &pkt_name
                                   __attribute__((unused))) {
  std::string transformed_body = "";

  // Vector of newly created packet temporaries
  std::vector<std::string> new_decls = {};

  // Check that it's in ssa.
  assert_exception(is_in_ssa(function_body));

  // all the stuff (packet fields) we care.
  const VariableTypeSelector selector = {{VariableType::STATE_SCALAR, false},
                                         {VariableType::STATE_ARRAY, false},
                                         {VariableType::PACKET, true}};

  // Get a list of packet fields for mass-replacement.
  const auto packetFields = gen_var_list(function_body, selector);

  // Insert replacement variable names into the replacement map.
  std::map<std::string, std::string> repls;
  for (const std::string &pField : packetFields) {
    std::string replStr(pField);
    std::replace(replStr.begin(), replStr.end(), '.', '_');
    repls[pField] = replStr;
  }

  // Now carry out replacements.
  for (const auto *child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    assert_exception(dyn_cast<BinaryOperator>(child)->isAssignmentOp());
    transformed_body += replace_vars(dyn_cast<BinaryOperator>(child)->getLHS(),
                                     repls, selector) +
                        " = " +
                        replace_vars(dyn_cast<BinaryOperator>(child)->getRHS(),
                                     repls, selector) +
                        ";\n";
  }

  return transformed_body;
}
