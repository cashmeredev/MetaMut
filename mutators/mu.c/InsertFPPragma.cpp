#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class InsertFPPragma : public Mutator,
                       public clang::RecursiveASTVisitor<InsertFPPragma> {
  using VisitorTy = clang::RecursiveASTVisitor<InsertFPPragma>;

public:
  using Mutator::Mutator;
  bool mutate() override;
  bool TraverseFunctionDecl(clang::FunctionDecl *FD);
  bool VisitCompoundStmt(clang::CompoundStmt *CS);
  bool VisitExpr(clang::Expr *E);

private:
  std::string makePragma();

  std::vector<clang::CompoundStmt *> TheBlocks;
  std::vector<clang::CompoundStmt *> TheCurrentBlocks;
  bool usesFloatingPoint = false;
};

static RegisterMutator<InsertFPPragma> M("c.insert-fp-pragma",
    "Put a floating-point environment pragma (FENV_ACCESS, rounding, "
    "contraction, exception behavior) at the start of a block that computes "
    "with floating-point values.");

bool InsertFPPragma::TraverseFunctionDecl(FunctionDecl *FD) {
  TheCurrentBlocks.clear();
  usesFloatingPoint = false;
  if (!VisitorTy::TraverseFunctionDecl(FD))
    return false;
  // the pragmas only change code that computes with floating-point values
  if (usesFloatingPoint)
    TheBlocks.insert(TheBlocks.end(), TheCurrentBlocks.begin(),
                     TheCurrentBlocks.end());
  return true;
}

bool InsertFPPragma::VisitCompoundStmt(CompoundStmt *CS) {
  if (isMutationSite(CS) && CS->getLBracLoc().isValid() &&
      !CS->getLBracLoc().isMacroID())
    TheCurrentBlocks.push_back(CS);
  return true;
}

bool InsertFPPragma::VisitExpr(Expr *E) {
  if (isPlainFloating(E->getType()))
    usesFloatingPoint = true;
  return true;
}

std::string InsertFPPragma::makePragma() {
  std::vector<std::string> pragmas = {
      "#pragma STDC FENV_ACCESS ON",
      "#pragma STDC FENV_ACCESS OFF",
      "#pragma STDC FP_CONTRACT ON",
      "#pragma STDC FP_CONTRACT OFF",
      "#pragma STDC FENV_ROUND FE_TONEAREST",
      "#pragma STDC FENV_ROUND FE_UPWARD",
      "#pragma STDC FENV_ROUND FE_DOWNWARD",
      "#pragma STDC FENV_ROUND FE_TOWARDZERO",
      "#pragma STDC FENV_ROUND FE_DYNAMIC",
      "#pragma clang fp contract(fast)",
      "#pragma clang fp contract(on)",
      "#pragma clang fp contract(off)",
      "#pragma clang fp reassociate(on)",
      "#pragma clang fp reciprocal(on)",
      "#pragma clang fp exceptions(strict)",
      "#pragma clang fp exceptions(maytrap)",
      "#pragma clang fp exceptions(ignore)",
      "#pragma clang fp eval_method(source)",
      "#pragma clang fp eval_method(double)",
      "#pragma clang fp eval_method(extended)",
      "#pragma float_control(precise, on)",
      "#pragma float_control(precise, off)",
      "#pragma float_control(except, on)"};
  return randElement(pragmas);
}

bool InsertFPPragma::mutate() {
  TraverseAST(getASTContext());
  if (TheBlocks.empty())
    return false;

  CompoundStmt *block = randElement(TheBlocks);
  getRewriter().InsertTextAfterToken(block->getLBracLoc(),
                                     "\n" + makePragma() + "\n");
  return true;
}
