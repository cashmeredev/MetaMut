#include <clang/AST/Expr.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class WrapInStmtExpr : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;

private:
  bool isCandidate(Expr *E);
};

static RegisterMutator<WrapInStmtExpr> M("c.wrap-in-stmt-expr",
    "Wrap an expression into a GNU statement expression that computes it "
    "through a temporary, optionally with control flow inside.");

bool WrapInStmtExpr::isCandidate(Expr *E) {
  if (!isMutationSite(E) || !E->isPRValue())
    return false;
  QualType T = E->getType().getCanonicalType();
  if (isPlainInteger(T) || isPlainFloating(T))
    return true;
  if (const auto *PT = dyn_cast<PointerType>(T))
    return isa<BuiltinType>(PT->getPointeeType().getCanonicalType());
  return false;
}

bool WrapInStmtExpr::mutate() {
  RuntimeExprCollector collector(
      [this](Expr *E) { return isCandidate(E); });
  std::vector<Expr *> sites = collector.collect(getASTContext());
  if (sites.empty())
    return false;

  Expr *expr = randElement(sites);
  QualType type = expr->getType().getCanonicalType().getUnqualifiedType();
  std::string name = generateUniqueName("tmp");
  std::string decl = formatAsDecl(type, name);
  std::string text = getSourceText(expr);

  std::string replacement;
  switch (randIndex(5)) {
  case 0:
    replacement = "({ " + decl + " = (" + text + "); " + name + "; })";
    break;
  case 1:
    replacement =
        "({ " + decl + "; { " + name + " = (" + text + "); } " + name + "; })";
    break;
  case 2:
    replacement = "({ " + decl + "; do { " + name + " = (" + text +
                  "); } while (0); " + name + "; })";
    break;
  case 3:
    replacement = "({ " + decl + " = (" + text + "); if (" + name + ") { " +
                  name + " = " + name + "; } " + name + "; })";
    break;
  default:
    replacement = "({ " + decl + " = (" + text +
                  "); switch (0) { default: break; } " + name + "; })";
    break;
  }

  getRewriter().ReplaceText(expr->getSourceRange(), replacement);
  return true;
}
