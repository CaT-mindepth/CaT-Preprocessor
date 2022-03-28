#ifndef NEW_SSA_H_
#define NEW_SSA_H_

#include "ast_visitor.h"
#include "clang/AST/Stmt.h"
#include <set>
#include <vector>

using namespace clang;

struct Block {

  std::string block_prefix;

  std::vector<clang::Stmt *> stmts;

  Block *parent = nullptr;
  // pointer to then block
  Block *left = nullptr;
  // pointer to else block
  Block *right = nullptr;
  // pointer to next block in line.
  Block *next = nullptr;

  bool isSplitNode;
};

class CreateBlockVisitor {
  Block *Convert(const Stmt *stmt, Block *b) {

    Block * next = this->createBlock(parent = b);

    bool currentBlockEnds = false;
    std::vector<Stmt *> nextStmts;

    if (isa<CompoundStmt>(stmt)) {
      for (const auto *child : stmt->children()) {
        if (currentBlockEnds) {
          nextStmts.emplace_back(child);
          continue;
        }

        if (isa<IfStmt>(child)) {
          // perform recursion
          const auto *if_child = dyn_cast<IfStmt>(child);
          Block *left = Convert(if_child->getThen(), this->createBlock(parent = b, next = next));
          Block *right = if_child->getElse() == nullptr
                             ? nullptr
                             : Convert(if_child->getElse(), this->createBlock(parent = b, next = next));
          b->left = left;
          b->right = right;
          currentBlockEnds = true;
        } else {
          b->stmts.emplace_back(child);
        }
      }
    } else if (isa<IfStmt>(stmt)) {
      // perform recursion
      b->isSplitNode = true;
      const auto *if_child = dyn_cast<IfStmt>(stmt);
      Block *left = Convert(if_child->getThen(), this->createBlock(parent = b, next = next));
      Block *right = if_child->getElse() == nullptr
                                 ? nullptr
                                 : Convert(if_child->getElse(), this->createBlock(parent = b, next = next));
    } else {
        b->stmts.emplace_back(stmt);
    }

    if (currentBlockEnds)
      ConvertNext(nextStmts, next, b);
    b->next = next;

    return b;
  }

  Block *ConvertNext(std::vector<Stmt *> stmts, Block *b, Block *parent) {
    bool currentBlockEnds = false;

    for (const Stmt *stmt : stmts) {
        // TODO here
    }
  }

private:
  std::vector<Block> blocks;
  std::unique_ptr<Block> root;
  size_t max_id = 0;

  std::string generateBlockPrefix() {
    auto p = max_id;
    max_id++;
    return "_blk_" + std::to_string(p) + "_";
  }

  Block *createBlock(Block *parent = nullptr, Block *left = nullptr,
                     Block *right = nullptr, Block *next = nullptr) {
    this->blocks.emplace_back(Block{.block_prefix = this->generateBlockPrefix(),
                                    .parent = parent,
                                    .left = left,
                                    .right = right,
                                    .next = nullptr,
                                    .isSplitNode = false});
    return &(this->blocks[blocks.size() - 1]);
  }
};

#endif