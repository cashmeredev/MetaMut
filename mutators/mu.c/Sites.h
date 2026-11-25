#ifndef MUC_SITES_H
#define MUC_SITES_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>

#include <functional>
#include <set>
#include <string>
#include <vector>

// Collects expressions inside function bodies that are evaluated at run time,
// so they may be replaced by something that is not a constant expression (a
// statement expression, a builtin call, ...). Everything that has to stay a
// constant expression is skipped: case labels, array bounds, bit-field widths,
// initializers of static variables, attribute arguments and so on.
class RuntimeExprCollector
    : public clang::RecursiveASTVisitor<RuntimeExprCollector> {
  using VisitorTy = clang::RecursiveASTVisitor<RuntimeExprCollector>;

public:
  using Filter = std::function<bool(clang::Expr *)>;

  explicit RuntimeExprCollector(Filter filter) : filter(std::move(filter)) {}

  std::vector<clang::Expr *> collect(clang::ASTContext &Context) {
    for (clang::Decl *D : Context.getTranslationUnitDecl()->decls()) {
      auto *FD = llvm::dyn_cast<clang::FunctionDecl>(D);
      if (FD && FD->doesThisDeclarationHaveABody())
        TraverseStmt(FD->getBody());
    }
    return sites;
  }

  bool VisitExpr(clang::Expr *E) {
    if (!llvm::isa<clang::OpaqueValueExpr>(E) && filter(E))
      sites.push_back(E);
    return true;
  }

  // declarations whose expressions are constant or not evaluated
  bool TraverseVarDecl(clang::VarDecl *VD) {
    if (!VD->hasLocalStorage() || VD->isConstexpr())
      return true;
    return VisitorTy::TraverseVarDecl(VD);
  }
  bool TraverseFunctionDecl(clang::FunctionDecl *) { return true; }
  bool TraverseRecordDecl(clang::RecordDecl *) { return true; }
  bool TraverseEnumDecl(clang::EnumDecl *) { return true; }
  bool TraverseTypedefDecl(clang::TypedefDecl *) { return true; }
  bool TraverseStaticAssertDecl(clang::StaticAssertDecl *) { return true; }
  bool TraverseTypeLoc(clang::TypeLoc) { return true; }
  bool TraverseType(clang::QualType) { return true; }
  bool TraverseAttr(clang::Attr *) { return true; }

  // statements and expressions with constant or unevaluated operands
  bool TraverseCaseStmt(clang::CaseStmt *CS) {
    return TraverseStmt(CS->getSubStmt());
  }
  bool TraverseGCCAsmStmt(clang::GCCAsmStmt *) { return true; }
  bool TraverseMSAsmStmt(clang::MSAsmStmt *) { return true; }
  bool TraverseConstantExpr(clang::ConstantExpr *) { return true; }
  bool TraverseUnaryExprOrTypeTraitExpr(clang::UnaryExprOrTypeTraitExpr *) {
    return true;
  }
  bool TraverseOffsetOfExpr(clang::OffsetOfExpr *) { return true; }
  bool TraverseAtomicExpr(clang::AtomicExpr *) { return true; }
  bool TraverseDesignatedInitExpr(clang::DesignatedInitExpr *E) {
    return TraverseStmt(E->getInit());
  }
  bool TraverseChooseExpr(clang::ChooseExpr *E) {
    return WalkUpFromChooseExpr(E) && TraverseStmt(E->getLHS()) &&
           TraverseStmt(E->getRHS());
  }
  bool TraverseCallExpr(clang::CallExpr *E) {
    // many builtins require constant arguments, keep their operands as is
    if (E->getBuiltinCallee() != 0)
      return WalkUpFromCallExpr(E);
    return VisitorTy::TraverseCallExpr(E);
  }
  bool TraverseImplicitCastExpr(clang::ImplicitCastExpr *E) {
    // the operand has to stay a null pointer constant
    if (E->getCastKind() == clang::CK_NullToPointer)
      return WalkUpFromImplicitCastExpr(E);
    return VisitorTy::TraverseImplicitCastExpr(E);
  }

private:
  Filter filter;
  std::vector<clang::Expr *> sites;
};

// Collects local variables that are declared alone in their declaration
// statement and whose address is never taken, so changing their type cannot
// break a pointer that refers to them.
class LocalVarCollector : public clang::RecursiveASTVisitor<LocalVarCollector> {
  using VisitorTy = clang::RecursiveASTVisitor<LocalVarCollector>;

public:
  std::vector<clang::VarDecl *> collect(clang::ASTContext &Context) {
    TraverseAST(Context);
    std::vector<clang::VarDecl *> result;
    for (clang::VarDecl *VD : candidates)
      if (!excluded.count(VD))
        result.push_back(VD);
    return result;
  }

  bool VisitDeclStmt(clang::DeclStmt *DS) {
    if (!DS->isSingleDecl())
      return true;
    auto *VD = llvm::dyn_cast<clang::VarDecl>(DS->getSingleDecl());
    if (VD && VD->isLocalVarDecl())
      candidates.push_back(VD);
    return true;
  }

  bool VisitUnaryOperator(clang::UnaryOperator *UO) {
    if (UO->getOpcode() == clang::UO_AddrOf)
      exclude(UO->getSubExpr());
    return true;
  }

  bool VisitGCCAsmStmt(clang::GCCAsmStmt *AS) {
    for (unsigned index = 0; index < AS->getNumOutputs(); index++)
      exclude(AS->getOutputExpr(index));
    for (unsigned index = 0; index < AS->getNumInputs(); index++)
      exclude(AS->getInputExpr(index));
    return true;
  }

private:
  void exclude(clang::Expr *E) {
    E = E->IgnoreParenCasts();
    if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(E))
      if (auto *VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl()))
        excluded.insert(VD);
  }

  std::vector<clang::VarDecl *> candidates;
  std::set<clang::VarDecl *> excluded;
};

// A builtin integer type other than _Bool, e.g. `unsigned long`.
inline bool isPlainInteger(clang::QualType T) {
  const auto *BT = llvm::dyn_cast<clang::BuiltinType>(T.getCanonicalType());
  return BT && BT->isInteger() && BT->getKind() != clang::BuiltinType::Bool;
}

// `float`, `double` or `long double`.
inline bool isPlainFloating(clang::QualType T) {
  const auto *BT = llvm::dyn_cast<clang::BuiltinType>(T.getCanonicalType());
  if (!BT)
    return false;
  return BT->getKind() == clang::BuiltinType::Float ||
         BT->getKind() == clang::BuiltinType::Double ||
         BT->getKind() == clang::BuiltinType::LongDouble;
}

// The spelling of a builtin type without qualifiers or typedef names.
inline std::string plainTypeName(clang::QualType T,
                                 const clang::ASTContext &Context) {
  return T.getCanonicalType().getUnqualifiedType().getAsString(
      Context.getPrintingPolicy());
}

#endif
