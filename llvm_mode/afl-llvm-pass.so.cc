/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.
*/

#define AFL_LLVM_PASS

#include "../config.h"
#include "../debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"

#if LLVM_VERSION_MAJOR < 11
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#else
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/OptimizationLevel.h"
#endif

using namespace llvm;

namespace {

  /* Shared instrumentation implementation. Returns true if the module was
     modified. */

  static bool instrumentModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "afl-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>\n");

  } else be_quiet = 1;

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* Get globals for the SHM region and the previous location. Note that
     __afl_prev_loc is thread-local. */

    GlobalVariable *AFLMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
               GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

    GlobalVariable *AFLPrevLoc = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc",
      0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0;

  for (auto &F : M)
    for (auto &BB : F) {

      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      if (AFL_R(100) >= inst_ratio) continue;

      /* Make up cur_loc */

      unsigned int cur_loc = AFL_R(MAP_SIZE);

      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */

      LoadInst *PrevLoc = IRB.CreateLoad(Int32Ty, AFLPrevLoc);
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"),
               MDNode::get(C, ArrayRef<Metadata *>()));
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      /* Load SHM pointer */

        LoadInst *MapPtr =
          IRB.CreateLoad(PointerType::get(Int8Ty, 0), AFLMapPtr);
        MapPtr->setMetadata(M.getMDKindID("nosanitize"),
                  MDNode::get(C, ArrayRef<Metadata *>()));
      // Value *MapPtrIdx =
      //     IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));
      /* Shift coverage feedback to the left by SHIFT_SIZE many elements
        (PrevLocCasted XOR PrevLocCasted) -->
        (((PrevLocCasted XOR PrevLocCasted) % (MAP_SIZE - SHIFT_SIZE)) + SHIFT_SIZE)
      */
        Value *MapPtrIdx = IRB.CreateGEP(
          Int8Ty, MapPtr,
          IRB.CreateAdd(
            IRB.CreateURem(
              IRB.CreateXor(PrevLocCasted, CurLoc),
              ConstantInt::get(Int32Ty, MAP_SIZE - SHIFT_SIZE)),
            ConstantInt::get(Int32Ty, SHIFT_SIZE)));

      /* Update bitmap */

      LoadInst *Counter = IRB.CreateLoad(Int8Ty, MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"),
               MDNode::get(C, ArrayRef<Metadata *>()));
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
        IRB.CreateStore(Incr, MapPtrIdx)
            ->setMetadata(M.getMDKindID("nosanitize"),
                          MDNode::get(C, ArrayRef<Metadata *>()));

      /* Set prev_loc to cur_loc >> 1 */

        StoreInst *Store = IRB.CreateStore(
            ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
        Store->setMetadata(M.getMDKindID("nosanitize"),
                           MDNode::get(C, ArrayRef<Metadata *>()));

      inst_blocks++;

    }

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u locations (%s mode, ratio %u%%).",
             inst_blocks, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

    return true;

  }

#if LLVM_VERSION_MAJOR < 11

  class AFLCoverage : public ModulePass {

   public:

    static char ID;
    AFLCoverage() : ModulePass(ID) {}

    bool runOnModule(Module &M) override { return instrumentModule(M); }

  };

}


char AFLCoverage::ID = 0;


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_ModuleOptimizerEarly, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);

#else  /* LLVM_VERSION_MAJOR >= 11 */

  struct AFLCoverageNewPM : public PassInfoMixin<AFLCoverageNewPM> {

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {

      instrumentModule(M);
      return PreservedAnalyses::none();

    }

  };

}  // namespace


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {

  return {LLVM_PLUGIN_API_VERSION, "afl-coverage", VERSION,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                  MPM.addPass(AFLCoverageNewPM());
                });
          }};

}

#endif
