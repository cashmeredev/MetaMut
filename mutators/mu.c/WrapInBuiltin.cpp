#include <clang/AST/Expr.h>
#include <string>

#include "Mutator.h"
#include "MutatorManager.h"
#include "Sites.h"

using namespace clang;

class WrapInBuiltin : public Mutator {
public:
  using Mutator::Mutator;
  bool mutate() override;

private:
  bool isCandidate(Expr *E);
  std::string wrapInteger(const std::string &text, const std::string &type);
  std::string wrapFloating(const std::string &text, QualType type);
};

static RegisterMutator<WrapInBuiltin> M("c.wrap-in-builtin",
    "Pass an expression through a compiler builtin (branch hints, _Generic, "
    "bit manipulation, math) that fits its type.");

bool WrapInBuiltin::isCandidate(Expr *E) {
  if (!isMutationSite(E) || !E->isPRValue())
    return false;
  QualType type = E->getType();
  if (isPlainFloating(type))
    return true;
  return isPlainInteger(type) && getASTContext().getTypeSize(type) <= 64;
}

std::string WrapInBuiltin::wrapInteger(const std::string &text,
                                       const std::string &type) {
  std::string cast = "(" + type + ")";
  std::string value = "(" + text + ")";
  // forms that repeat the operand are kept away from long operands, otherwise
  // repeated mutation doubles the program again and again
  unsigned forms = text.size() < 120 ? 14 : 11;
  switch (randIndex(forms)) {
  case 0:
    return cast + "__builtin_expect((long)" + value + ", " +
           std::to_string(getManager().randint(0, 1)) + ")";
  case 1: {
    std::vector<std::string> probabilities = {"0.0", "0.1", "0.5", "0.9",
                                              "1.0"};
    return cast + "__builtin_expect_with_probability((long)" + value + ", " +
           std::to_string(getManager().randint(0, 1)) + ", " +
           randElement(probabilities) + ")";
  }
  case 2: {
    std::vector<std::string> widths = {"16", "32", "64"};
    std::string width = randElement(widths);
    return cast + "__builtin_bswap" + width + "((__UINT" + width + "_TYPE__)" +
           value + ")";
  }
  case 3: {
    std::vector<std::string> widths = {"8", "16", "32", "64"};
    std::string width = randElement(widths);
    return cast + "__builtin_bitreverse" + width + "((__UINT" + width +
           "_TYPE__)" + value + ")";
  }
  case 4: {
    std::vector<std::string> widths = {"8", "16", "32", "64"};
    std::vector<std::string> directions = {"left", "right"};
    std::string width = randElement(widths);
    return cast + "__builtin_rotate" + randElement(directions) + width +
           "((__UINT" + width + "_TYPE__)" + value + ", " +
           std::to_string(getManager().randint(0, 70)) + ")";
  }
  case 5:
    return cast + "__builtin_popcountll((unsigned long long)" + value + ")";
  case 6:
    return cast + "__builtin_parityll((unsigned long long)" + value + ")";
  case 7:
    return cast + "__builtin_ffsll((long long)" + value + ")";
  case 8:
    return cast + "__builtin_clrsbll((long long)" + value + ")";
  case 9:
    return cast + "__builtin_llabs((long long)" + value + ")";
  case 10: {
    std::vector<std::string> builtins = {"__builtin_clzll", "__builtin_ctzll"};
    return cast + randElement(builtins) + "((unsigned long long)" + value +
           " | 1)";
  }
  case 11:
    return "_Generic(" + value + ", int: " + value + ", unsigned int: " +
           value + ", long: " + value + ", default: " + value + ")";
  case 12:
    return "__builtin_choose_expr(sizeof" + value + " == " +
           std::to_string(1 << randIndex(4)) + ", " + value + ", " + value +
           ")";
  default:
    return "(__builtin_constant_p" + value + " ? " + value + " : " + value +
           ")";
  }
}

std::string WrapInBuiltin::wrapFloating(const std::string &text,
                                        QualType type) {
  std::string suffix;
  const auto *BT = cast<BuiltinType>(type.getCanonicalType());
  if (BT->getKind() == BuiltinType::Float)
    suffix = "f";
  else if (BT->getKind() == BuiltinType::LongDouble)
    suffix = "l";

  std::string value = "(" + text + ")";
  unsigned forms = text.size() < 120 ? 8 : 5;
  switch (randIndex(forms)) {
  case 0: {
    std::vector<std::string> builtins = {
        "fabs", "sqrt", "rint", "nearbyint", "floor", "ceil", "trunc",
        "round", "roundeven", "exp", "log", "sin", "cos"};
    return "__builtin_" + randElement(builtins) + suffix + value;
  }
  case 1: {
    std::vector<std::string> builtins = {"copysign", "fmax", "fmin", "fmod",
                                         "pow", "fdim"};
    std::vector<std::string> operands = {"0.0", "-0.0", "1.0", "-1.0", "2.0",
                                         "0.5"};
    return "__builtin_" + randElement(builtins) + suffix + "(" + value + ", " +
           randElement(operands) + ")";
  }
  case 2:
    return "__builtin_ldexp" + suffix + "(" + value + ", " +
           std::to_string(getManager().randint(-3, 3)) + ")";
  case 3:
    return "__builtin_fma" + suffix + "(" + value + ", 1.0, 0.0)";
  case 4: {
    std::vector<std::string> builtins = {"lrint", "llrint", "lround",
                                         "llround", "ilogb"};
    return "__builtin_" + randElement(builtins) + suffix + value;
  }
  case 5:
    return "__builtin_fma" + suffix + "(" + value + ", " + value + ", " +
           value + ")";
  case 6: {
    std::vector<std::string> builtins = {
        "__builtin_isnan", "__builtin_isinf", "__builtin_isfinite",
        "__builtin_isnormal", "__builtin_signbit"};
    return "(" + randElement(builtins) + value + " ? " + value + " : " +
           value + ")";
  }
  default: {
    std::vector<std::string> builtins = {
        "__builtin_isgreater", "__builtin_isless", "__builtin_isunordered",
        "__builtin_islessgreater"};
    return "(" + randElement(builtins) + "(" + value + ", " + value + ") ? " +
           value + " : " + value + ")";
  }
  }
}

bool WrapInBuiltin::mutate() {
  RuntimeExprCollector collector(
      [this](Expr *E) { return isCandidate(E); });
  std::vector<Expr *> sites = collector.collect(getASTContext());
  if (sites.empty())
    return false;

  Expr *expr = randElement(sites);
  std::string text = getSourceText(expr);
  std::string replacement;
  if (isPlainFloating(expr->getType()))
    replacement = wrapFloating(text, expr->getType());
  else
    replacement =
        wrapInteger(text, plainTypeName(expr->getType(), getASTContext()));

  getRewriter().ReplaceText(expr->getSourceRange(), "(" + replacement + ")");
  return true;
}
