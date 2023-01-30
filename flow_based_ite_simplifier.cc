#include "flow_based_ite_simplifier.h"

#include "third_party/assert_exception.h"

#include "clang_utility_functions.h"
#include "pkt_func_transform.h"
#include "context.h"

#include <iostream>

using namespace clang;


std::string flow_based_ite_simplify_transform(const clang::TranslationUnitDecl *tu_decl) {
  return pkt_func_transform(
      tu_decl, std::bind(&flow_based_ite_simplifier_transform_body, std::placeholders::_1,
                         std::placeholders::_2, &(tu_decl->getASTContext())));
}

bool isPktVar(const std::string & a, const std::string& pkt_name) {
  if (a.size() < pkt_name.size()) return false;
  size_t i;
  for(i = 0; i < pkt_name.size(); i++)
    if (a[i] != pkt_name[i]) return false;
  if (i < a.size() && a[i] == '.') return true;
  else return false;
}

std::string removePktPrefix(const std::string& a, const std::string& pkt_name) {
  if (!isPktVar(a, pkt_name)) return a;
  else {
    return a.substr(pkt_name.size() + 1);
  }
}

// replace all occurances of `from` in `str` into `to`.
std::string strReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string processFocusedSubexpression(const clang::Expr * lhs, const clang::Expr * rhs, clang::BinaryOperatorKind bopKind, const std::string& pkt_name) {

  std::string l = "", r = "";

  std::set<std::string> lhsVars = gen_var_list(lhs);
  std::set<std::string> rhsVars = gen_var_list(rhs);

  std::set<std::string> notInRHS, notInLHS;
  for (const auto & v : lhsVars) 
    if (rhsVars.find(v) == rhsVars.end())
      notInRHS.insert(v);
  for (const auto & v : rhsVars)
    if (lhsVars.find(v) == lhsVars.end())
      notInLHS.insert(v);
    

  
  // critical condition
  if (notInLHS.size() == 1 && notInRHS.size() == 1) {
    const std::string lhs_var = *(notInRHS.begin());
    const std::string rhs_var = *(notInLHS.begin());
    // std::cout << " left base: " << Context::GetContext().GetBase(removePktPrefix(lhs_var, pkt_name));
    // std::cout << " right base: " << Context::GetContext().GetBase(removePktPrefix(rhs_var, pkt_name));
    if (isPktVar(lhs_var, pkt_name) && isPktVar(rhs_var, pkt_name)) {
      if (Context::GetContext().GetBase(removePktPrefix(lhs_var, pkt_name)) == Context::GetContext().GetBase(removePktPrefix(rhs_var, pkt_name))) {
        // can skip right
        // std::cout << " \nleft==right\n";

        // need to check if (lhs, rhs) is simplifiable.
        // simple hack: replace rhs_var occurances in rhs with lhs_var and do string equality check.
        l = clang_stmt_printer(lhs);
        r = clang_stmt_printer(rhs);

        std::string rReplaced = strReplaceAll(r, rhs_var, lhs_var);
        //std::cout << "l = " << l << "  ; r' = " << rReplaced << std::endl;

        if (l == rReplaced) {
          l = clang_stmt_printer(lhs);
          r = "";
        } // else, do not do replacement since l, r are not doing the same thing.

      } // derived from the same SSA equiv class
      else {
        // std::cout << "left!=right!!\n";
        l = clang_stmt_printer(lhs);
        r = clang_stmt_printer(rhs);
      }
    } else {
      l = clang_stmt_printer(lhs);
      r = clang_stmt_printer(rhs);
    }
  } else {
    // abort
    l = clang_stmt_printer(lhs);
    r = clang_stmt_printer(rhs);
  }
  
  // std::cout << " flow-based ite simplification: detected simplifiable statement --------" << std::endl;
  // std::cout << " before: " << clang_stmt_printer(lhs) << " <OP> " << clang_stmt_printer(rhs) << std::endl;
  // std::cout << " after: " << l << " , " << r << std::endl; 

  if (bopKind == clang::BinaryOperatorKind::BO_LAnd) {
      if (r == "" ) return l;
      else return l + " && " + r;
  } else if (bopKind == clang::BinaryOperatorKind::BO_LOr) {
      if (r == "") return l;
      else return l + " || " + r; 
  } else {
    assert_exception(0);
  }
  // unreachable
  return "";
}

std::string processFocus(const clang::BinaryOperator* focus, const clang::Expr* lhs, const clang::Stmt* child, const std::string & pkt_name) {

  // std::cout << "  simplify candidate found: " << clang_stmt_printer(child) << std::endl;

  if (focus->getOpcode() == clang::BinaryOperatorKind::BO_LAnd) {
    std::string processed = processFocusedSubexpression(focus->getLHS()->IgnoreParenImpCasts(), focus->getRHS()->IgnoreParenImpCasts(), focus->getOpcode(), pkt_name);
    return clang_stmt_printer(lhs) + " = " + processed + ";\n";
  } else if (focus->getOpcode() == clang::BinaryOperatorKind::BO_LOr) {
    std::string processed =processFocusedSubexpression(focus->getLHS()->IgnoreParenImpCasts(), focus->getRHS()->IgnoreParenImpCasts(), focus->getOpcode(), pkt_name);
    return clang_stmt_printer(lhs) + " = " + processed + ";\n";
  } else return clang_stmt_printer(child) + ";\n";
}

std::pair<std::string, std::vector<std::string>>
flow_based_ite_simplifier_transform_body(const CompoundStmt *function_body,
                  const std::string &pkt_name,
                  clang::ASTContext * _ctx) {
  // Do not run if this is not in SSA
  if (not is_in_ssa(function_body)) {
    throw std::logic_error("Flow-based ITE simplification can be run only after SSA. "
                           "It relies on variables not being redefined\n");
  }

  std::string transformed_body = "";

  // iterate over program statements and focus on branch points.

  for (const auto & child : function_body->children()) {
    assert_exception(isa<BinaryOperator>(child));
    const auto * bin_op = dyn_cast<BinaryOperator>(child);
    assert_exception(bin_op->isAssignmentOp());
    const auto * rhs = bin_op->getRHS()->IgnoreParenImpCasts();
    const auto * lhs = bin_op->getLHS()->IgnoreParenImpCasts();
    const std::string lhsStr = clang_expr_printer(lhs);
    // if lhs is branch temporary

    // std::cout << "  testing if lhs is branch temp: " << lhsStr << std::endl;


    if (Context::GetContext().GetType(removePktPrefix(lhsStr, pkt_name)) == D_BIT) {
      // only conservatively parse (a <OP> b) && (a <OP> b'), (a' <OP> b) && (a <OP> b) 
      // or neg ((a <OP> b) && (a <OP> b')), neg ((a' <OP> b) && (a <OP> b))    
      if (isa<BinaryOperator>(rhs)) {
        const auto * focus = dyn_cast<BinaryOperator>(rhs);
        transformed_body += processFocus(focus, lhs, child, pkt_name);
       } else if (isa<UnaryOperator>(rhs)) {
        const auto * un_op = dyn_cast<UnaryOperator>(rhs);
        if (isa<BinaryOperator>(un_op->getSubExpr())) {
          const auto * focus = dyn_cast<BinaryOperator>(un_op->getSubExpr());
          transformed_body += processFocus(focus, lhs, child, pkt_name);
        } else transformed_body += clang_stmt_printer(child) + ";";
      } 
    } else transformed_body += clang_stmt_printer(child) + ";";
  } 
 

  return std::make_pair("{" + transformed_body + "}",
                        std::vector<std::string>());
}
