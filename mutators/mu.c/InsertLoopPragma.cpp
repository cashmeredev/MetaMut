#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <set>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"

using namespace clang;

class InsertLoopPragma : public Mutator,
                         public clang::RecursiveASTVisitor<InsertLoopPragma> {
public:
  using Mutator::Mutator;
  bool mutate() override;
  bool VisitAttributedStmt(clang::AttributedStmt *AS);
  bool VisitForStmt(clang::ForStmt *FS);
  bool VisitWhileStmt(clang::WhileStmt *WS);
  bool VisitDoStmt(clang::DoStmt *DS);

private:
  void addLoop(clang::Stmt *loop);
  std::string count(int low, int high);
  std::string makePragma();

  std::vector<clang::Stmt *> TheLoops;
  std::set<clang::Stmt *> TheAnnotatedLoops;
};

static RegisterMutator<InsertLoopPragma> M("c.insert-loop-pragma",
    "Put a loop optimization pragma (vectorize, interleave, unroll, "
    "distribute) with random arguments in front of a loop.");

bool InsertLoopPragma::VisitAttributedStmt(AttributedStmt *AS) {
  // the loop already carries a pragma, a second one may contradict it
  TheAnnotatedLoops.insert(AS->getSubStmt());
  return true;
}

void InsertLoopPragma::addLoop(Stmt *loop) {
  if (isMutationSite(loop))
    TheLoops.push_back(loop);
}

bool InsertLoopPragma::VisitForStmt(ForStmt *FS) {
  addLoop(FS);
  return true;
}

bool InsertLoopPragma::VisitWhileStmt(WhileStmt *WS) {
  addLoop(WS);
  return true;
}

bool InsertLoopPragma::VisitDoStmt(DoStmt *DS) {
  addLoop(DS);
  return true;
}

std::string InsertLoopPragma::count(int low, int high) {
  return std::to_string(getManager().randint(low, high));
}

std::string InsertLoopPragma::makePragma() {
  if (randIndex(4) == 0) {
    std::vector<std::string> standalone = {
        "#pragma unroll", "#pragma unroll(" + count(2, 16) + ")",
        "#pragma nounroll", "#pragma unroll_and_jam",
        "#pragma unroll_and_jam(" + count(2, 8) + ")",
        "#pragma nounroll_and_jam", "#pragma GCC unroll " + count(0, 16)};
    return randElement(standalone);
  }

  // one option per group at most, options of one group contradict each other
  std::vector<std::string> vectorize = {
      "vectorize(enable)", "vectorize(disable)", "vectorize(assume_safety)",
      "vectorize_width(" + count(1, 64) + ")",
      "vectorize_width(" + count(1, 16) + ", fixed)",
      "vectorize_width(" + count(1, 16) + ", scalable)",
      "vectorize(enable) vectorize_predicate(enable)",
      "vectorize(enable) vectorize_width(" + count(2, 32) + ")"};
  std::vector<std::string> interleave = {
      "interleave(enable)", "interleave(disable)",
      "interleave_count(" + count(1, 16) + ")"};
  std::vector<std::string> unroll = {
      "unroll(enable)", "unroll(disable)", "unroll(full)",
      "unroll_count(" + count(1, 32) + ")"};
  std::vector<std::string> distribute = {
      "distribute(enable)", "distribute(disable)"};

  std::string options;
  while (options.empty()) {
    if (randBool())
      options += " " + randElement(vectorize);
    if (randBool())
      options += " " + randElement(interleave);
    if (randBool())
      options += " " + randElement(unroll);
    if (randBool())
      options += " " + randElement(distribute);
  }
  return "#pragma clang loop" + options;
}

bool InsertLoopPragma::mutate() {
  TraverseAST(getASTContext());
  std::vector<Stmt *> sites;
  for (Stmt *loop : TheLoops)
    if (!TheAnnotatedLoops.count(loop))
      sites.push_back(loop);
  if (sites.empty())
    return false;

  Stmt *loop = randElement(sites);
  getRewriter().InsertTextBefore(loop->getBeginLoc(),
                                 "\n" + makePragma() + "\n");
  return true;
}
