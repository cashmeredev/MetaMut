#include <clang/AST/Expr.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class ArithToOverflowBuiltin : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;

private:
  bool isCandidate(Expr *E);
};

static RegisterMutator<ArithToOverflowBuiltin> M("c.arith-to-overflow-builtin",
    "Replace an integer +, - or * by the matching __builtin_*_overflow call "
    "with a result of a random integer type.");

bool ArithToOverflowBuiltin::isCandidate(Expr *E) {
  auto *BO = dyn_cast<BinaryOperator>(E);
  if (!BO || !isMutationSite(BO))
    return false;
  if (BO->getOpcode() != BO_Add && BO->getOpcode() != BO_Sub &&
      BO->getOpcode() != BO_Mul)
    return false;
  return isPlainInteger(BO->getType()) &&
         isPlainInteger(BO->getLHS()->getType()) &&
         isPlainInteger(BO->getRHS()->getType());
}

bool ArithToOverflowBuiltin::mutate() {
  RuntimeExprCollector collector(
      [this](Expr *E) { return isCandidate(E); });
  std::vector<Expr *> sites = collector.collect(getASTContext());
  if (sites.empty())
    return false;

  auto *binop = cast<BinaryOperator>(randElement(sites));
  std::string builtin = "__builtin_add_overflow";
  if (binop->getOpcode() == BO_Sub)
    builtin = "__builtin_sub_overflow";
  else if (binop->getOpcode() == BO_Mul)
    builtin = "__builtin_mul_overflow";

  std::string type = plainTypeName(binop->getType(), getASTContext());
  std::vector<std::string> resultTypes = {
      type, type, type, "signed char", "short", "int", "long long",
      "__int128", "unsigned char", "unsigned short", "unsigned int",
      "unsigned long", "unsigned __int128", "_BitInt(17)",
      "unsigned _BitInt(63)", "_BitInt(128)"};
  std::string resultType = randElement(resultTypes);

  std::string result = generateUniqueName("res");
  std::string call = builtin + "((" + getSourceText(binop->getLHS()) + "), (" +
                     getSourceText(binop->getRHS()) + "), &" + result + ")";

  std::string replacement = "({ " + resultType + " " + result + "; ";
  switch (randIndex(3)) {
  case 0:
    replacement += call + "; (" + type + ")" + result + "; })";
    break;
  case 1:
    replacement += "if (" + call + ") " + result + " = 0; (" + type + ")" +
                   result + "; })";
    break;
  default: {
    std::string flag = generateUniqueName("ovf");
    replacement += "_Bool " + flag + " = " + call + "; " + flag + " ? (" +
                   type + ")0 : (" + type + ")" + result + "; })";
    break;
  }
  }

  getRewriter().ReplaceText(binop->getSourceRange(), replacement);
  return true;
}
