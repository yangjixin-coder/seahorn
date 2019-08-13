#pragma once

#include "seahorn/Analysis/CanFail.hh"
#include "seahorn/OperationalSemantics.hh"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Pass.h"

#include <boost/container/flat_set.hpp>

namespace llvm {
class GetElementPtrInst;
}

namespace seahorn {
namespace details {
class Bv3OpSemContext;
Bv3OpSemContext &ctx3(OpSemContext &_ctx);
} // namespace details
/**
   Bit-precise operational semantics for LLVM (take 3)

   Fairly accurate representation of LLVM semantics without
   considering undefined behaviour. Most operators are mapped
   directly to their logical equivalent SMT-LIB representation.

   Memory is modelled by arrays.
 */
class Bv3OpSem : public OperationalSemantics {
  Pass &m_pass;
  TrackLevel m_trackLvl;

  const DataLayout *m_td;
  const TargetLibraryInfo *m_tli;
  const CanFail *m_canFail;

public:
  Bv3OpSem(ExprFactory &efac, Pass &pass, const DataLayout &dl,
           TrackLevel trackLvl = MEM);

  Bv3OpSem(const Bv3OpSem &o);

  const DataLayout &getTD() {
    assert(m_td);
    return *m_td;
  }
  const DataLayout &getDataLayout() { return getTD(); }

  /// \brief Returns a concrete value to which a constant evaluates
  /// Adapted from llvm::ExecutionEngine
  Optional<GenericValue> getConstantValue(const Constant *C);

  /// \brief Initializes memory pointed by \p Addr with the value of the contant
  /// \p Init. Assumes that \p Addr is sufficiently large
  void initMemory(const Constant *Init, void *Addr);

  /// \brief Stores a value in \p Val to memory pointed by \p Ptr. The store is
  /// of type \p Ty
  void storeValueToMemory(const GenericValue &Val, GenericValue *Ptr, Type *Ty);

  /// \brief Creates a new context
  OpSemContextPtr mkContext(SymStore &values, ExprVector &side) override;

  /// \brief Executes one intra-procedural instructions in the
  /// current context. Assumes that current instruction is not a
  /// branch. Returns true if instruction was executed and false if
  /// there is no suitable instruction found
  bool intraStep(seahorn::details::Bv3OpSemContext &C);
  /// \brief Executes one intra-procedural branch instruction in the
  /// current context. Assumes that current instruction is a branch
  void intraBr(seahorn::details::Bv3OpSemContext &C, const BasicBlock &dst);

  /// \brief Execute all PHINode instructions of the current basic block
  /// \brief assuming that control flows from previous basic block
  void intraPhi(seahorn::details::Bv3OpSemContext &C);

  /// \brief Returns symbolic representation of the global errorFlag variable
  Expr errorFlag(const BasicBlock &BB) override;

  void exec(const BasicBlock &bb, OpSemContext &_ctx) override {
    exec(bb, details::ctx3(_ctx));
  }
  void execPhi(const BasicBlock &bb, const BasicBlock &from,
               OpSemContext &_ctx) override {
    execPhi(bb, from, details::ctx3(_ctx));
  }
  void execEdg(const BasicBlock &src, const BasicBlock &dst,
               OpSemContext &_ctx) override {
    execEdg(src, dst, details::ctx3(_ctx));
  }
  void execBr(const BasicBlock &src, const BasicBlock &dst,
              OpSemContext &_ctx) override {
    execBr(src, dst, details::ctx3(_ctx));
  }

protected:
  void exec(const BasicBlock &bb, seahorn::details::Bv3OpSemContext &ctx);
  void execPhi(const BasicBlock &bb, const BasicBlock &from,
               seahorn::details::Bv3OpSemContext &ctx);
  void execEdg(const BasicBlock &src, const BasicBlock &dst,
               seahorn::details::Bv3OpSemContext &ctx);
  void execBr(const BasicBlock &src, const BasicBlock &dst,
              seahorn::details::Bv3OpSemContext &ctx);

public:
  /// \brief Returns a concrete representation of a given symbolic expression.
  ///        Assumes that the input expression \p v has concrete representation.
  const Value &conc(Expr v) const override;

  /// \brief Indicates whether an instruction/value is skipped by
  /// the semantics. An instruction is skipped means that, from the
  /// perspective of the semantics, the instruction does not
  /// exist. It is not executed, has no effect on the execution
  /// context, and no instruction that is not skipped depends on it
  bool isSkipped(const Value &v) const;

  bool isTracked(const Value &v) const override { return !isSkipped(v); }

  /// \brief Deprecated. Returns true if \p v is a symbolic register
  bool isSymReg(Expr v) override { llvm_unreachable(nullptr); }
  /// \brief Returns true if \p v is a symbolic register
  bool isSymReg(Expr v, seahorn::details::Bv3OpSemContext &ctx);

  /// \brief Creates a symbolic register for an llvm::Value
  Expr mkSymbReg(const Value &v, OpSemContext &_ctx) override;
  /// \brief Finds a symbolic register for \p v, if it exists
  Expr getSymbReg(const Value &v, const OpSemContext &_ctx) const override;

  /// \brief Returns the current symbolic value of \p v in the context \p ctx
  Expr getOperandValue(const Value &v, seahorn::details::Bv3OpSemContext &ctx);
  /// \brief Deprecated
  Expr lookup(SymStore &s, const Value &v) { llvm_unreachable(nullptr); }

  using gep_type_iterator = generic_gep_type_iterator<>;
  /// \brief Returns symbolic representation of the gep offset
  Expr symbolicIndexedOffset(gep_type_iterator it, gep_type_iterator end,
                             seahorn::details::Bv3OpSemContext &ctx);

  /// \brief Returns memory size of type \p t
  unsigned storageSize(const llvm::Type *t) const;
  /// \breif Returns offset of a filed in a structure
  unsigned fieldOff(const StructType *t, unsigned field) const;

  /// \brief Size of the register (in bits) required to store \p v
  uint64_t sizeInBits(const llvm::Value &v) const;
  /// \brief Size of the register (in bits) required to store values of type \p
  /// t
  uint64_t sizeInBits(const llvm::Type &t) const;
  /// \brief Number of bits required to store a pointer
  unsigned pointerSizeInBits() const;

  /// Reports (and records) an instruction as skipped by the semantics
  void skipInst(const Instruction &inst,
                seahorn::details::Bv3OpSemContext &ctx);
  /// Reports (and records) an instruction as not being handled by
  /// the semantics
  void unhandledInst(const Instruction &inst,
                     seahorn::details::Bv3OpSemContext &ctx);
  void unhandledValue(const Value &v, seahorn::details::Bv3OpSemContext &ctx);
};
} // namespace seahorn