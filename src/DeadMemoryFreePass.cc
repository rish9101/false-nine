#include "DeadMemoryFreePass.h"
using namespace std;

vector<Node *> DeadMemoryFreePass::getAccessedNodes(AccessPath *ap,
                                                    MemoryGraph *memGraph) {
  auto links = ap->getLinks();
  vector<Node *> v;
  Node *currNode = nullptr;
  for (int i = links.size() - 1; i >= 0; i--) {
    auto val = links[i];
    if (currNode == nullptr) {
      // We first have to get the malloc node where the base of Access Path
      // points to
      auto baseNode = memGraph->getNodeFromValue(val);
      if (baseNode == nullptr)
        outs() << formatv("COULD NOT FIND BASENODE for {0}\n",
                          getShortValueName(val));

      currNode = memGraph->getTargetMemoryNode(baseNode);
      v.push_back(currNode);
    } else {
      if (val == nullptr) {
        currNode = memGraph->getTargetDerefNode(currNode);
        v.push_back(currNode);
      } else {
        currNode = memGraph->getTargetFieldNode(currNode, val);
        v.push_back(currNode);
      }
    }
  }
  return v;
}

enum DeadMemoryFreePass::CAN_REMOVE_NODE DeadMemoryFreePass::canRemoveNode(
    Node *N, MemoryGraph *memGraph, vector<Node *> accessedNodes,
    vector<Value *> mallocValues, vector<Node *> removedNodes) {

  // If already removed, don't double free it!
  if (std::find(removedNodes.begin(), removedNodes.end(), N) !=
      removedNodes.end()) {
    return CAN_REMOVE_NODE::NO;
  }

  for (int i = 0; i < accessedNodes.size(); ++i) {
    if (accessedNodes[i]->isMemoryNode && !accessedNodes[i]->isFieldNode) {
      if (N == accessedNodes[i])
        return CAN_REMOVE_NODE::NO;
    }
    if (accessedNodes[i]->isFieldNode) {
      auto memNode = memGraph->getFieldMemoryNode(accessedNodes[i]);
      if (memNode == N) {
        return CAN_REMOVE_NODE::NO;
      }
    }
  }
  // If this node hasn't been accessed, it still might not be safe to free it,
  // Since we construct The memory graph prior to this step, if its variable has
  // been defined and it hasn't been accessed, then it can be freed.

  auto idx = memGraph->getNodeIdx(N);
  if (idx == -1) {
    outs() << "Node doesn't exist!\n";
    return CAN_REMOVE_NODE::NO;
  }
  // Are there cases where we can say for sure that the memory can be freed??
  // Point to think about
  for (auto vec : memGraph->graph) {
    auto edge = vec[idx];
    if (edge == nullptr)
      continue;
    auto srcNode = edge->src;
    if (srcNode && srcNode->nodeValue != nullptr &&
        std::find(mallocValues.begin(), mallocValues.end(),
                  srcNode->nodeValue) != mallocValues.end()) {
      return CAN_REMOVE_NODE::MAYBE;
    }
  }
  return CAN_REMOVE_NODE::NO;
};

char DeadMemoryFreePass::ID = 0;

bool DeadMemoryFreePass::runOnFunction(Function &F) {

  AccessPathDFA *ptdfa = new AccessPathDFA(F);
  ptdfa->performDFA(F);

  auto memoryGraph = getAnalysis<MemoryGraphPass>().getMemoryGraph(F);
  int i = 0;

  vector<pair<Value *, Instruction *>> freeCalls;
  vector<Value *> mallocVals;
  vector<Node *> removedMemNodes;
  vector<Node *> freedNodes;

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isa<ReturnInst>(&I)) {
        continue;
      }
      if (isa<CallInst>(&I) && dyn_cast<CallInst>(&I)
                                   ->getCalledFunction()
                                   ->getReturnType()
                                   ->isPointerTy()) {
        mallocVals.push_back(&I);
      }
      if (isa<CallInst>(&I) &&
          dyn_cast<CallInst>(&I)->getCalledFunction()->getName().equals(
              "free")) {
        // We have a free call, remove this node from memory graph, it has been
        // freed.

        auto freeArg = I.getOperand(0);
        auto memNode = memoryGraph->getTargetMemoryNode(
            memoryGraph->getNodeFromValue(freeArg));
        freedNodes.push_back(memNode);
      }
      auto aps = ptdfa->OutS[&I];
      vector<Node *> accessedNodes;
      for (auto bit : aps.set_bits()) {
        auto ap = ptdfa->domain[bit];
        auto n = getAccessedNodes(&ap, memoryGraph);
        for (auto a : n) {
          accessedNodes.push_back(a);
        }
      }
      for (auto memNode : memoryGraph->mallocNodes) {
        CAN_REMOVE_NODE crn = canRemoveNode(memNode, memoryGraph, accessedNodes,
                                            mallocVals, removedMemNodes);

        switch (crn) {
        case CAN_REMOVE_NODE::YES: {
          auto val = memoryGraph->getRootValueForMemNode(memNode);

          freeCalls.push_back(pair<Value *, Instruction *>(val, &I));

          removedMemNodes.push_back(memNode);
          break;
        }
        case CAN_REMOVE_NODE::MAYBE: {
          // If mem node is accessed by a global variable AND this is not a main
          // function DO NOT free the node
          if (memoryGraph->hasGlobalAccessor(memNode) == false ||
              F.getName().equals("main")) {
            auto val = memoryGraph->getRootValueForMemNode(memNode);

            freeCalls.push_back(pair<Value *, Instruction *>(val, &I));

            removedMemNodes.push_back(memNode);
          }
          break;
        }

        case CAN_REMOVE_NODE::NO:
          break;
        }
      }
    }
  }

  int numFreesInserted = 0;
  for (auto freeCall : freeCalls) {
    auto memNode = memoryGraph->getTargetMemoryNode(
        memoryGraph->getNodeFromValue(freeCall.first));
    if (std::find(freedNodes.begin(), freedNodes.end(), memNode) !=
        freedNodes.end()) {
      continue;
    }

    CallInst::CreateFree(freeCall.first, freeCall.second->getNextNode());
    numFreesInserted++;
  }
  outs() << formatv(
      "Number of free calls inserted in the function {0} are {1}\n",
      F.getName(), numFreesInserted);

  return true;
}

bool DeadMemoryFreePass::runOnModule(Module &M) { return false; }

void addPointsToPass(const PassManagerBuilder & /* unused */,
                     legacy::PassManagerBase &PM) {
  PM.add(new DeadMemoryFreePass());
}

void DeadMemoryFreePass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MemoryGraphPass>();
}

// Register the pass so `opt -mempass` runs it.
static RegisterPass<DeadMemoryFreePass> A("heap-liveness",
                                          "Perform Points To analysis");
// Tell frontends to run the pass automatically.
static RegisterStandardPasses B(PassManagerBuilder::EP_VectorizerStart,
                                addPointsToPass);
static RegisterStandardPasses C(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                addPointsToPass);