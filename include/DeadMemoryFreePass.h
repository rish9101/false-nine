#pragma once

#include <map>
#include <ostream>
#include <utility>
#include <vector>

#include "AccessPath.h"
#include "AccessPathDFA.h"
#include "Common.h"
#include "MemoryGraphPass.h"
using namespace std;

class DeadMemoryFreePass : public FunctionPass {

private:
  std::vector<Instruction *> mallocInstructions;

  typedef enum CAN_REMOVE_NODE { YES, NO, MAYBE } CAN_REMOVE_NODE;

public:
  static char ID;
  vector<Node *> getAccessedNodes(AccessPath *ap, MemoryGraph *memGraph);

  CAN_REMOVE_NODE canRemoveNode(Node *N, MemoryGraph *memGraph,
                                vector<Node *> accessedNodes,
                                vector<Value *> mallocValues,
                                vector<Node *> removedNodes);

  DeadMemoryFreePass() : FunctionPass(ID){};

  virtual bool runOnFunction(Function &F);
  virtual bool runOnModule(Module &M);

  void getAnalysisUsage(AnalysisUsage &AU) const;
};
