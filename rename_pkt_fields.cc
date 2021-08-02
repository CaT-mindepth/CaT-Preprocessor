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
  return pkt_func_transform(tu_decl, rename_pkt_fields_body);
}

std::pair<std::string, std::vector<std::string>>
rename_pkt_fields_body(const clang::CompoundStmt *function_body,
                       const std::string &pkt_name __attribute__((unused))) {
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
                        ";";
  }

  return std::make_pair("{" + transformed_body + "}",
                        std::vector<std::string>());
}
