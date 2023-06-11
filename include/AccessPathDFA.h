#pragma once

#include <map>
#include <ostream>
#include <utility>
#include <vector>

#include "AccessPath.h"
#include "Common.h"
using namespace std;
class AccessPathDFA {
public:
  map<Instruction *, BitVector> InS;
  map<Instruction *, BitVector> OutS;

  vector<AccessPath> domain;

  AccessPathDFA(Function &F);

  void printBV(BitVector bv);

  void printAPFromBV(BitVector bv);

  BitVector meetOp(Instruction &I);

  BitVector transferFunc(Instruction &I);

  void performDFA(Function &F);

  tuple<BitVector, BitVector, BitVector> getSets(Instruction &I);
};