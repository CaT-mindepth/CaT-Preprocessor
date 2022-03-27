#include "array_replacer.h"
#include "clang_utility_functions.h"
#include "context.h"

#include <functional>

using namespace clang;

std::string
array_replacer_transform(const clang::TranslationUnitDecl *tu_decl) {
  // body (mostly) copied over from pkt_func_transform.cc
  // Accumulate all declarations

  ReplacementVisitor replacement_visitor;

  std::vector<const Decl *> all_decls;
  for (const auto *decl : dyn_cast<DeclContext>(tu_decl)->decls())
    all_decls.emplace_back(decl);

  // Sort all_decls
  std::stable_sort(all_decls.begin(), all_decls.end(),
                   [](const auto *decl1, const auto *decl2) {
                     return get_order(decl1) < get_order(decl2);
                   });

  // Loop through sorted vector of declarations
  std::string state_var_str = "";
  std::string scalar_func_str = "";
  std::string pkt_func_str = "";
  std::string record_decl_str = "";
  std::vector<std::string> new_decls;
  for (const auto *child_decl : all_decls) {
    assert_exception(child_decl);
    if (isa<VarDecl>(child_decl)) {
      // differentiate between an ArrayDecl and a regular VarDecl.
      const auto *vdecl = dyn_cast<VarDecl>(child_decl);
      const auto *underlying_type = vdecl->getType().getTypePtrOrNull();
      if (isa<ConstantArrayType>(underlying_type)) {
        // transform it into an ordinary integer type.
        const std::string decl_name = vdecl->getNameAsString();
        state_var_str += "int " + decl_name + ";";
      } else
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

      // Transform function body
      const auto transform_pair = array_replacer(
          dyn_cast<CompoundStmt>(function_decl->getBody()), pkt_name,
          replacement_visitor, &(tu_decl->getASTContext()));
      const auto transformed_body = transform_pair.first;
      new_decls = transform_pair.second;

      // Rewrite function with new body
      pkt_func_str += function_decl->getReturnType().getAsString() + " " +
                      function_decl->getNameInfo().getName().getAsString() +
                      "( " + pkt_type + " " + pkt_name + ") " +
                      transformed_body;
    } else if (isa<RecordDecl>(child_decl)) {
      // Open struct definition
      assert_exception(dyn_cast<RecordDecl>(child_decl)->isStruct());
      record_decl_str += "struct " +
                         dyn_cast<RecordDecl>(child_decl)->getNameAsString() +
                         "{\n";

      // acummulate current fields in struct
      for (const auto *field_decl : dyn_cast<DeclContext>(child_decl)->decls())
        record_decl_str +=
            dyn_cast<ValueDecl>(field_decl)->getType().getAsString() + " " +
            clang_value_decl_printer(dyn_cast<ValueDecl>(field_decl)) + ";";

      // Add newly created fields
      for (const auto &new_decl : new_decls)
        record_decl_str += new_decl;

      // Close struct definition
      record_decl_str += "};";
    }
  }
  return state_var_str + scalar_func_str + record_decl_str + pkt_func_str;
}

std::pair<std::string, std::vector<std::string>>
array_replacer(const clang::CompoundStmt *function_body,
               const std::string &pkt_name, ReplacementVisitor &visitor,
               ASTContext *_ctx) {
  std::string out = "";
  std::vector<std::string> new_decls;

  for (const auto *child : function_body->children()) {
    if (isa<BinaryOperator>(child)) {
      const auto *lhs =
          dyn_cast<BinaryOperator>(child)->getLHS()->IgnoreParenImpCasts();
      const auto *rhs =
          dyn_cast<BinaryOperator>(child)->getRHS()->IgnoreParenImpCasts();
      // use visitor pass on subtrees of lhs/rhs to do recursive replacement.
      const std::string lhs_visited = visitor.expr_visit_transform(_ctx, lhs);
      const std::string rhs_visited = visitor.expr_visit_transform(_ctx, rhs);
      out += lhs_visited + " = " + rhs_visited + ";\n";
    } else
      out += clang_stmt_printer(child) + ";";
  }

  return make_pair("{" + out + "}", new_decls);
}