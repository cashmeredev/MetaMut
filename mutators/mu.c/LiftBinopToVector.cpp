#include <clang/AST/Expr.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class LiftBinopToVector : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;

private:
  bool isCandidate(Expr *E);
  QualType getElementType(BinaryOperator *BO);
};

static RegisterMutator<LiftBinopToVector> M("c.lift-binop-to-vector",
    "Compute a scalar binary operation on vectors of a random, often "
    "unusual, length and take one element of the result.");

// The type both operands are converted to before the operation.
QualType LiftBinopToVector::getElementType(BinaryOperator *BO) {
  if (BO->isShiftOp() || BO->isComparisonOp())
    return BO->getLHS()->getType();
  return BO->getType();
}

bool LiftBinopToVector::isCandidate(Expr *E) {
  auto *BO = dyn_cast<BinaryOperator>(E);
  if (!BO || !isMutationSite(BO))
    return false;
  QualType left = BO->getLHS()->getType();
  QualType right = BO->getRHS()->getType();
  bool integers = isPlainInteger(left) && isPlainInteger(right);
  bool numbers = (isPlainInteger(left) || isPlainFloating(left)) &&
                 (isPlainInteger(right) || isPlainFloating(right));

  QualType element = getElementType(BO).getCanonicalType();
  const auto *BT = dyn_cast<BuiltinType>(element);
  if (!BT || BT->getKind() == BuiltinType::LongDouble ||
      BT->getKind() == BuiltinType::Int128 ||
      BT->getKind() == BuiltinType::UInt128)
    return false;
  if (!isPlainInteger(BO->getType()) && !isPlainFloating(BO->getType()))
    return false;

  switch (BO->getOpcode()) {
  case BO_Add:
  case BO_Sub:
  case BO_Mul:
  case BO_Div:
  case BO_LT:
  case BO_GT:
  case BO_LE:
  case BO_GE:
  case BO_EQ:
  case BO_NE:
    return numbers;
  case BO_Rem:
  case BO_And:
  case BO_Or:
  case BO_Xor:
  case BO_Shl:
  case BO_Shr:
    return integers;
  default:
    return false;
  }
}

bool LiftBinopToVector::mutate() {
  RuntimeExprCollector collector(
      [this](Expr *E) { return isCandidate(E); });
  std::vector<Expr *> sites = collector.collect(getASTContext());
  if (sites.empty())
    return false;

  auto *binop = cast<BinaryOperator>(randElement(sites));
  std::string element = plainTypeName(getElementType(binop), getASTContext());
  std::string type = plainTypeName(binop->getType(), getASTContext());

  std::vector<unsigned> lengths = {2, 3, 4, 5, 7, 8, 9, 16, 17, 32, 64};
  unsigned length = randElement(lengths);
  std::string index = std::to_string(randIndex(length));

  std::string vector = generateUniqueName("vec_t");
  std::string left = generateUniqueName("lhs");
  std::string right = generateUniqueName("rhs");
  std::string operation =
      "(" + left + " " + binop->getOpcodeStr().str() + " " + right + ")";

  std::string replacement =
      "({ typedef " + element + " " + vector +
      " __attribute__((ext_vector_type(" + std::to_string(length) + "))); " +
      vector + " " + left + " = (" + vector + ")(" + element + ")(" +
      getSourceText(binop->getLHS()) + "); " + vector + " " + right + " = (" +
      vector + ")(" + element + ")(" + getSourceText(binop->getRHS()) + "); ";
  if (binop->isComparisonOp())
    // a vector comparison yields -1 for true, keep the scalar 0 or 1
    replacement += "(" + type + ")(" + operation + "[" + index + "] != 0); })";
  else
    replacement += "(" + type + ")" + operation + "[" + index + "]; })";

  getRewriter().ReplaceText(binop->getSourceRange(), replacement);
  return true;
}
