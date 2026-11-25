#include <clang/AST/Decl.h>
#include <clang/AST/TypeLoc.h>
#include <sstream>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class IntToBitInt : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;

private:
  bool findTypeRange(VarDecl *VD, SourceRange &range);
};

static RegisterMutator<IntToBitInt> M("c.int-to-bitint",
    "Change the type of a local integer variable to a _BitInt of a random, "
    "often odd, width.");

// Finds the tokens that spell the builtin integer type of the variable. Fails
// when the type is not written as plain keywords, e.g. through a typedef.
bool IntToBitInt::findTypeRange(VarDecl *VD, SourceRange &range) {
  if (!isPlainInteger(VD->getType()) || !VD->getTypeSourceInfo())
    return false;
  TypeLoc TL = VD->getTypeSourceInfo()->getTypeLoc().getUnqualifiedLoc();
  BuiltinTypeLoc BTL = TL.getAs<BuiltinTypeLoc>();
  if (BTL.isNull())
    return false;
  range = BTL.getSourceRange();
  if (range.isInvalid() || range.getBegin().isMacroID() ||
      range.getEnd().isMacroID())
    return false;

  SourceManager &SM = getSourceManager();
  SourceLocation endLoc = getLocForEndOfToken(range.getEnd());
  const char *begin = SM.getCharacterData(range.getBegin());
  const char *end = SM.getCharacterData(endLoc);
  if (!begin || !end || begin >= end)
    return false;

  std::istringstream words(std::string(begin, end - begin));
  std::string word;
  unsigned count = 0;
  while (words >> word) {
    if (word != "int" && word != "unsigned" && word != "signed" &&
        word != "long" && word != "short" && word != "char")
      return false;
    count++;
  }
  return count > 0;
}

bool IntToBitInt::mutate() {
  LocalVarCollector collector;
  std::vector<VarDecl *> sites;
  std::vector<SourceRange> ranges;
  for (VarDecl *VD : collector.collect(getASTContext())) {
    SourceRange range;
    if (isMutationSite(VD) && findTypeRange(VD, range)) {
      sites.push_back(VD);
      ranges.push_back(range);
    }
  }
  if (sites.empty())
    return false;

  unsigned index = randIndex(sites.size());
  bool isUnsigned = randBool();
  unsigned width;
  switch (randIndex(4)) {
  case 0: // same width as the original type
    width = getASTContext().getTypeSize(sites[index]->getType());
    break;
  case 1: // tiny
    width = getManager().randint(2, 9);
    break;
  case 2: // wider than any builtin integer
    width = getManager().randint(129, 1024);
    break;
  default:
    width = getManager().randint(2, 128);
    break;
  }

  std::string replacement = "_BitInt(" + std::to_string(width) + ")";
  if (isUnsigned)
    replacement = "unsigned " + replacement;
  getRewriter().ReplaceText(ranges[index], replacement);
  return true;
}
