#include <clang/AST/Decl.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class ToAtomic : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;
};

static RegisterMutator<ToAtomic> M("c.to-atomic",
    "Qualify a local arithmetic variable with _Atomic, so every access to it "
    "becomes an atomic operation.");

bool ToAtomic::mutate() {
  LocalVarCollector collector;
  std::vector<VarDecl *> sites;
  for (VarDecl *VD : collector.collect(getASTContext())) {
    QualType type = VD->getType();
    if (!isMutationSite(VD) || type->isAtomicType())
      continue;
    if (isPlainInteger(type) || isPlainFloating(type) ||
        type->isEnumeralType() || type->isBooleanType())
      sites.push_back(VD);
  }
  if (sites.empty())
    return false;

  VarDecl *var = randElement(sites);
  getRewriter().InsertTextBefore(var->getBeginLoc(), "_Atomic ");
  return true;
}
