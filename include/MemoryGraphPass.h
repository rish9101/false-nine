
#pragma once

#include "Common.h"
#include "MemoryGraph.h"

class MemoryGraphPass : public FunctionPass {
public:
  static char ID;
  MemoryGraph *memGraph;
  MemoryGraphPass() : FunctionPass(ID){};

  virtual bool runOnModule(Module &M);
  virtual bool runOnFunction(Function &F);

  void analyzeInst(Instruction &I);

  MemoryGraph *getMemoryGraph(Function &F);
};
