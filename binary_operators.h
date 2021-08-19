#ifndef BINARY_OPERATORS_H_
#define BINARY_OPERATORS_H_

#include <clang/AST/AST.h>
#include <clang/AST/Stmt.h>

#include <set>

/// A cascading binary binary operator acts on
/// a group of statements
struct CommutativeOperator {
  CommutativeOperator(clang::BinaryOperatorKind opcode,
                      std::set<clang::Stmt> &terms, const clang::Stmt *next)
      : opcode_(opcode), terms_(terms), next_(next) {}
  clang::BinaryOperatorKind opcode_;
  std::set<clang::Stmt> terms_;
  const clang::Stmt *next_;
};

#endif BINARY_OPERATORS_H_ // BINARY_OPERATORS_H_