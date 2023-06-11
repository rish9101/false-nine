#include "MemoryGraphPass.h"

void MemoryGraphPass::analyzeInst(Instruction &I) {

  if (auto callInst = dyn_cast<CallInst>(&I)) {

    if (callInst->getCalledFunction()->getReturnType()->isPointerTy()) {
      // We have a function call which returns a pointer, add a memory and root
      // Node in the graph, also add an edge between them

      auto valNode = memGraph->getOrAddNode(false, false, callInst);
      auto memNode = memGraph->getOrAddNode(true, false);
      auto newEdge = new Edge(valNode, memNode, nullptr);
      memGraph->addEdge(valNode, memNode, newEdge);
    }
  } else if (auto bitcastInst = dyn_cast<BitCastInst>(&I)) {
    if (auto bitcastNode =
            memGraph->getNodeFromValue(bitcastInst->getOperand(0))) {
      auto srcNode = memGraph->getOrAddNode(false, false, bitcastInst);
      auto destNode = memGraph->getTargetMemoryNode(bitcastNode);
      memGraph->addEdge(srcNode, destNode,
                        new Edge(srcNode, destNode, nullptr));
    }
  } else if (auto gepInst = dyn_cast<GetElementPtrInst>(&I)) {

    auto node = memGraph->getNodeFromValue(gepInst->getPointerOperand());
    auto baseMemNode = memGraph->getTargetMemoryNode(node);

    int i = 0;
    for (auto &idx : gepInst->indices()) {
      if (i == (gepInst->getNumIndices() - 1)) {
        auto memIdx = memGraph->getNodeIdx(baseMemNode);
        Edge *targetEdge = nullptr;
        for (auto edge : memGraph->graph[memIdx]) {
          if (edge != nullptr && edge->edgeLabel != nullptr &&
              dyn_cast<ConstantInt>(edge->edgeLabel)->getValue() ==
                  dyn_cast<ConstantInt>(idx.get())->getValue()) {
            targetEdge = edge;
          }
        }
        if (targetEdge == nullptr) {
          // We did not find the required edge, add it to the graph
          auto targetFieldNode = memGraph->getOrAddNode(true, true, nullptr);
          memGraph->addEdge(baseMemNode, targetFieldNode,
                            new Edge(baseMemNode, targetFieldNode, idx.get()));

          // Add an edge from getelementptr destination to the field Node
          auto gepNode = memGraph->getOrAddNode(false, false, gepInst);
          memGraph->addEdge(gepNode, targetFieldNode,
                            new Edge(gepNode, targetFieldNode, nullptr));
        } else {
          // We found the target edge
          auto targetFieldNode = targetEdge->dest;
          auto gepNode = memGraph->getOrAddNode(false, false, gepInst);
          memGraph->addEdge(gepNode, targetFieldNode,
                            new Edge(gepNode, targetFieldNode, nullptr));
        }
      }
      ++i;
    }
  } else if (auto storeInst = dyn_cast<StoreInst>(&I)) {
    // Only deal with stores for pointer-to-pointer stores
    // Two kinds of stores are encountered, one for local variable and other for
    // global variables, which translate to global_ptr->* = ptr in the IR
    // The deref Node of global_ptr will not exist in the Memory Map (Since it
    // is global), But, it has to be dealt with too! So we simply assume that
    // global_ptr itself points to ptr and not its deref
    if (isPointerToPointer(storeInst->getPointerOperandType()) == true) {
      auto node = memGraph->getNodeFromValue(storeInst->getValueOperand());
      auto destMemNode = memGraph->getTargetMemoryNode(node);
      Node *srcNode = nullptr;
      if (isa<GlobalVariable>(storeInst->getPointerOperand())) {
        srcNode = memGraph->getOrAddNode(false, false,
                                         storeInst->getPointerOperand());
      } else {
        node = memGraph->getNodeFromValue(storeInst->getPointerOperand());
        srcNode = memGraph->getTargetMemoryNode(node);
      }

      // node = memGraph->getNodeFromValue(storeInst->getPointerOperand());
      memGraph->addEdge(srcNode, destMemNode,
                        new Edge(srcNode, destMemNode, nullptr));
    }
  } else if (auto loadInst = dyn_cast<LoadInst>(&I)) {
    if (isPointerToPointer(loadInst->getPointerOperandType()) == true) {
      auto ptrVal = loadInst->getPointerOperand();
      auto srcNode = memGraph->getOrAddNode(false, false, loadInst);
      Node *destNode = nullptr;
      if (isa<GlobalVariable>(ptrVal)) {
        destNode =
            memGraph->getTargetMemoryNode(memGraph->getNodeFromValue(ptrVal));
      } else {
        auto baseMemNode =
            memGraph->getTargetMemoryNode(memGraph->getNodeFromValue(ptrVal));
        destNode = memGraph->getTargetDerefNode(baseMemNode);
      }
      memGraph->addEdge(srcNode, destNode,
                        new Edge(srcNode, destNode, nullptr));
    }
  }
}

char MemoryGraphPass::ID = 0;

bool MemoryGraphPass::runOnModule(Module &M) {
  outs() << formatv(" Running Memory Graph Passs for {0}\n", M.getName());
  return false;
}

bool MemoryGraphPass::runOnFunction(Function &F) {

  memGraph = new MemoryGraph();

  for (auto &BB : F) {
    for (auto &I : BB) {
      analyzeInst(I);
    }
  }

  return false;
}

MemoryGraph *MemoryGraphPass::getMemoryGraph(Function &F) { return memGraph; }

void addMemoryGraphPass(const PassManagerBuilder & /* unused */,
                        legacy::PassManagerBase &PM) {
  PM.add(new MemoryGraphPass());
}

static RegisterPass<MemoryGraphPass> A("memory-graph",
                                       "Calculate Memory Graph for a module");

static RegisterStandardPasses B(PassManagerBuilder::EP_VectorizerStart,
                                addMemoryGraphPass);
static RegisterStandardPasses C(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                addMemoryGraphPass);