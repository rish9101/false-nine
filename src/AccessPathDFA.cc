#include "AccessPathDFA.h"

AccessPathDFA::AccessPathDFA(Function &F) {

  for (auto &BB : F) {
    for (auto &I : BB) {
      InS[&I] = BitVector(1024, false);
      OutS[&I] = BitVector(1024, false);
    }
  }

  domain.reserve(512);
}

void AccessPathDFA::printBV(BitVector bv) {

  outs() << "{ ";
  for (auto i : bv.set_bits()) {
    outs() << i << " ,";
  }
  outs() << "}\n";
}
void AccessPathDFA::printAPFromBV(BitVector bv) {

  outs() << "{ ";
  for (auto i : bv.set_bits()) {
    domain[i].print();
    outs() << " , ";
  }
  outs() << "}\n";
}

BitVector AccessPathDFA::meetOp(Instruction &I) {

  BitVector newOut(256, true);

  if (isa<BranchInst>(&I)) {

    auto BB = I.getParent();
    for (auto successorBB : successors(BB)) {
      auto &successorInst = successorBB->front();
      newOut &= InS[&successorInst];
    }
    return newOut;
  }
  auto successorInst = I.getNextNode();
  newOut &= InS[successorInst];
  return newOut;
}

BitVector AccessPathDFA::transferFunc(Instruction &I) {

  BitVector newIn(256, true);

  BitVector LKill(256, false), LDirect(256, false), LTransfer(256, false);
  std::tie<BitVector, BitVector, BitVector>(LKill, LDirect, LTransfer) =
      getSets(I);
  newIn &= OutS[&I];
  LKill.flip();
  newIn &= LKill;
  newIn |= LDirect;
  newIn |= LTransfer;
  return newIn;
}

void AccessPathDFA::performDFA(Function &F) {
  bool changed = true;

  auto &bbList = F.getBasicBlockList();

  std::vector<Instruction *> insts;

  for (auto bb = bbList.rbegin(); bb != bbList.rend(); ++bb) {
    for (auto inst = (*bb).rbegin(); inst != (*bb).rend(); ++inst) {
      insts.push_back(&(*inst));
    }
  }

  // BACKWARDS PASS

  while (changed == true) {
    changed = false;
    for (auto inst : insts) {
      auto oldOut = OutS[inst];
      auto oldIn = InS[inst];

      auto newOut = meetOp(*inst);
      OutS[inst].reset();
      OutS[inst] |= newOut;

      auto newIn = transferFunc(*inst);
      InS[inst].reset();
      InS[inst] |= newIn;

      if ((oldOut != OutS[inst]) || (oldIn != InS[inst]))
        changed = true;
    }
  }
}

tuple<BitVector, BitVector, BitVector> AccessPathDFA::getSets(Instruction &I) {
  BitVector killSet(256, false);
  BitVector directSet(256, false);
  BitVector transferSet(256, false);
  // We have an instruction, based on what kind of inst it is, get direct,,
  // kill and transfer sets

  if (auto callInst = dyn_cast<CallInst>(&I)) {
    if (callInst->getCalledFunction()->getReturnType()->isPointerTy()) {
      auto generatedAp = new AccessPath(&I);

      // we have a malloc call
      // Since in LLVM IR, malloc calls aren't directly associated to
      // pointers, but bitcast and stored. Thus malloc calls has direct set
      // empty as base(px) = phi
      for (auto set_bit : OutS[&I].set_bits()) {
        auto accessPath = domain[set_bit];
        if (is_prefix(*generatedAp, accessPath)) {
          killSet.set(std::find(domain.begin(), domain.end(), accessPath) -
                      domain.begin());
        }
      }
    }
  } else if (auto storeInst = dyn_cast<StoreInst>(&I)) {
    // Store Instruction is also of two types
    // *a = b (in case of 'a' being a pointer to pointer type) or *a = value
    // (in case of 'a' being a pointer to data)
    if (isPointerToPointer(storeInst->getPointerOperandType()) == true) {

      // The store may have global variable as an operand, which is represented
      // by global_ptr->* = ptr in the IR, However, this is not really a
      // dereference style store since the mem location of global_ptr->* is
      // actually in the .data
      AccessPath *storeAP = nullptr;
      if (isa<GlobalVariable>(storeInst->getPointerOperand())) {
        storeAP = new AccessPath(storeInst->getPointerOperand());
      } else {
        storeAP = AccessPath::CreateDerefPath(storeInst->getPointerOperand());
      }
      // Calculating the killset
      for (auto set_bit : OutS[&I].set_bits()) {
        auto accessPath = domain[set_bit];
        if (is_prefix(*storeAP, accessPath)) {
          killSet.set(set_bit);
        }
      }

      // Calculating the Direct set
      auto baseAP = storeAP->getBase();
      if (baseAP) {
        if (std::find(domain.begin(), domain.end(), *baseAP) == domain.end()) {
          domain.push_back(*baseAP);
        }
        directSet.set(std::find(domain.begin(), domain.end(), *baseAP) -
                      domain.begin());
      }
      auto rhsAP = new AccessPath(storeInst->getValueOperand());
      if (std::find(domain.begin(), domain.end(), *rhsAP) == domain.end()) {
        domain.push_back(*rhsAP);
      }
      directSet.set(std::find(domain.begin(), domain.end(), *rhsAP) -
                    domain.begin());

      // Calculating the Transfer set
      for (auto set_bit : OutS[&I].set_bits()) {
        auto accessPath = domain[set_bit];
        if (is_proper_prefix(*storeAP, accessPath)) {
          auto suffix = accessPath.getFieldPath();
          auto transferAP = joinAccessPaths(*rhsAP, *suffix);
          if (std::find(domain.begin(), domain.end(), *transferAP) ==
              domain.end())
            domain.push_back(*transferAP);

          transferSet.set(std::find(domain.begin(), domain.end(), *transferAP) -
                          domain.begin());
        }
      }
    } else {

      // It is a simple Use case *a = i where i in not a pointer variable
      // thus a->*  Access Path is used
      auto derefAP =
          AccessPath::CreateDerefPath(storeInst->getPointerOperand());

      auto prefixes = getPrefixes(derefAP->getBase());
      for (auto prefix : prefixes) {
        if (std::find(domain.begin(), domain.end(), *prefix) == domain.end()) {
          domain.push_back(*prefix);
        }
        directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                      domain.begin());
      }
    }
  } else if (auto loadInst = dyn_cast<LoadInst>(&I)) {
    // If the type being loaded is ptr, then it is an a = *(b) type of
    // assignment, which is useful
    if (isPointerToPointer(loadInst->getPointerOperandType()) != false) {
      auto transferBaseAP = new AccessPath(loadInst->getPointerOperand());
      auto generatedAp = new AccessPath(&I);

      for (auto set_bit : OutS[&I].set_bits()) {
        auto accessPath = domain[set_bit];
        if (is_prefix(*generatedAp, accessPath)) {
          killSet.set(std::find(domain.begin(), domain.end(), accessPath) -
                      domain.begin());
        }
      }

      // LDirect is not phi - This generates b->* as a LDirect class
      // auto derefAP =
      // AccessPath::CreateDerefPath(loadInst->getPointerOperand());
      AccessPath *loadAP = nullptr;
      if (isa<GlobalVariable>(loadInst->getPointerOperand())) {
        loadAP = new AccessPath(loadInst->getPointerOperand());
      } else {
        loadAP = AccessPath::CreateDerefPath(loadInst->getPointerOperand());
      }
      auto prefixes = getPrefixes(loadAP);
      for (auto prefix : prefixes) {
        if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
          domain.push_back(*prefix);
        directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                      domain.begin());
      }
      // We calculate the LTransfer set
      for (auto set_bit : OutS[&I].set_bits()) {
        auto accessPath = domain[set_bit];
        if (is_proper_prefix(*generatedAp, accessPath)) {
          // we have a->c in Out, this is transferred to (*b)->c in In
          auto suffix = accessPath.getFieldPath();
          // auto derefAP = new AccessPath();
          // derefAP->parent = transferBaseAP;
          auto derefAP = new AccessPath(*loadAP);
          auto transferAp = *(joinAccessPaths(*derefAP, *suffix));
          if (std::find(domain.begin(), domain.end(), transferAp) ==
              domain.end())
            domain.push_back(transferAp);

          transferSet.set(std::find(domain.begin(), domain.end(), transferAp) -
                          domain.begin());
        }
      }
    }
    // Otherwise, this is a Use of the reference, thus we calculate the sets
    // differently for this case
    else {
      auto derefAP = AccessPath::CreateDerefPath(loadInst->getPointerOperand());
      auto prefixes = getPrefixes(derefAP->getBase());
      for (auto prefix : prefixes) {
        if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
          domain.push_back(*prefix);
        directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                      domain.begin());
      }
      // Add this to the LDirect set, Other two sets are empty for this case
      // if (std::find(domain.begin(), domain.end(), *derefAP) ==
      // domain.end())
      //     domain.push_back(*derefAP);
      // directSet.set(std::find(domain.begin(), domain.end(), *derefAP) -
      // domain.begin());
    }
  } else if (auto gepInst = dyn_cast<GetElementPtrInst>(&I)) {
    auto gepAccessPath = new AccessPath(gepInst);
    auto generatedAp = new AccessPath(&I);

    for (auto set_bit : OutS[&I].set_bits()) {
      auto accessPath = domain[set_bit];
      if (is_prefix(*generatedAp, accessPath)) {
        killSet.set(std::find(domain.begin(), domain.end(), accessPath) -
                    domain.begin());
      }
    }

    auto xPref = getPrefixes(generatedAp->getBase());

    for (auto prefix : xPref) {
      if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
        domain.push_back(*prefix);
      directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                    domain.begin());
    }

    auto yPref = getPrefixes(gepAccessPath);

    for (auto prefix : yPref) {
      if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
        domain.push_back(*prefix);
      directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                    domain.begin());
    }

    // We calculate the LTransfer set
    for (auto set_bit : OutS[&I].set_bits()) {
      auto accessPath = domain[set_bit];
      if (is_proper_prefix(*generatedAp, accessPath)) {
        // we have a->c in Out, this is transferred to (*b)->c in In
        auto suffix = accessPath.getFieldPath();
        auto transferAp = joinAccessPaths(*gepAccessPath, *suffix);
        if (std::find(domain.begin(), domain.end(), *transferAp) ==
            domain.end())
          domain.push_back(*transferAp);

        transferSet.set(std::find(domain.begin(), domain.end(), *transferAp) -
                        domain.begin());
      }
    }
  } else if (auto bitcastInst = dyn_cast<BitCastInst>(&I)) {
    auto bitcastAP = new AccessPath(bitcastInst->getOperand(0));
    auto generatedAp = new AccessPath(&I);

    for (auto set_bit : OutS[&I].set_bits()) {
      auto accessPath = domain[set_bit];
      if (is_prefix(*generatedAp, accessPath)) {
        killSet.set(std::find(domain.begin(), domain.end(), accessPath) -
                    domain.begin());
      }
    }

    auto xPref = getPrefixes(generatedAp->getBase());

    for (auto prefix : xPref) {
      if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
        domain.push_back(*prefix);
      directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                    domain.begin());
    }

    auto yPref = getPrefixes(bitcastAP);

    for (auto prefix : yPref) {
      if (std::find(domain.begin(), domain.end(), *prefix) == domain.end())
        domain.push_back(*prefix);
      directSet.set(std::find(domain.begin(), domain.end(), *prefix) -
                    domain.begin());
    }

    // We calculate the LTransfer set
    for (auto set_bit : OutS[&I].set_bits()) {
      auto accessPath = domain[set_bit];
      if (is_proper_prefix(*generatedAp, accessPath)) {
        // we have a->c in Out, this is transferred to (*b)->c in In
        auto suffix = accessPath.getFieldPath();
        auto transferAp = joinAccessPaths(*bitcastAP, *suffix);
        if (std::find(domain.begin(), domain.end(), *transferAp) ==
            domain.end())
          domain.push_back(*transferAp);

        transferSet.set(std::find(domain.begin(), domain.end(), *transferAp) -
                        domain.begin());
      }
    }
  } else if (auto retInst = dyn_cast<ReturnInst>(&I)) {

    if (retInst->getReturnValue()->getType()->isPointerTy()) {
      // if we return a pointer, then we have a Access Path generation
      auto accessPath = new AccessPath(retInst->getReturnValue());
      if (std::find(domain.begin(), domain.end(), *accessPath) == domain.end())
        domain.push_back(*accessPath);

      directSet.set(std::find(domain.begin(), domain.end(), *accessPath) -
                    domain.begin());
    }
  }

  return tuple<BitVector, BitVector, BitVector>(killSet, directSet,
                                                transferSet);
}