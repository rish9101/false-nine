#pragma once

#include <map>
#include <ostream>
#include <utility>
#include <vector>

#include "Common.h"

struct AccessPath {
  Value *rootVariable;
  AccessPath *parent;

  AccessPath() : rootVariable(nullptr), parent(nullptr){};
  AccessPath(Value *V) : rootVariable(V), parent(nullptr){};
  AccessPath(Value *V, AccessPath *parent) : rootVariable(V), parent(parent){};
  AccessPath(const AccessPath &ap);

  std::vector<Value *> getLinks();

  static AccessPath *CreateDerefPath(Value *V);

  AccessPath(GetElementPtrInst *GI);
  bool isDerefPath();
  AccessPath *getFieldPath();

  AccessPath *getRoot();
  void print();
  std::string str();
  AccessPath *getBase();
};

std::vector<AccessPath *> getPrefixes(AccessPath *ap);

bool operator==(const AccessPath &lhs, const AccessPath &rhs);

AccessPath *joinAccessPaths(AccessPath &lhs, AccessPath &rhs);

bool operator<(const AccessPath &lhs, const AccessPath &rhs);

bool operator<=(const AccessPath &lhs, const AccessPath &rhs);
bool operator!=(const AccessPath &lhs, const AccessPath &rhs);

bool is_prefix(AccessPath &prefix, AccessPath &target);

bool is_proper_prefix(AccessPath &prefix, AccessPath &target);