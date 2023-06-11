#include "AccessPath.h"

AccessPath::AccessPath(const AccessPath &ap) {
  rootVariable = ap.rootVariable;
  if (ap.parent == nullptr) {
    parent = nullptr;
  } else {
    parent = new AccessPath(*(ap.parent));
  }
}

std::vector<Value *> AccessPath::getLinks() {

  std::vector<Value *> v;
  auto it = this;
  while (it != nullptr) {
    v.push_back(it->rootVariable);
    it = it->parent;
  }
  return v;
}

AccessPath *AccessPath::CreateDerefPath(Value *V) {
  auto ap = new AccessPath();
  ap->parent = new AccessPath(V);
  return ap;
}

AccessPath::AccessPath(GetElementPtrInst *GI) {
  AccessPath *baseParent = new AccessPath(GI->getPointerOperand());

  int i = 0;
  for (auto &idx : GI->indices()) {
    if (i == (GI->getNumIndices() - 1)) {
      // We have the last field name, make this the rootvariable
      rootVariable = idx.get();

      parent = baseParent;
    }
    if (i != 0) {
      AccessPath *child = new AccessPath(idx.get(), baseParent);
      baseParent = child;
    }
    ++i;
  }
}

bool AccessPath::isDerefPath() {

  if (rootVariable == nullptr) {
    return true;
  }
  return false;
}

AccessPath *AccessPath::getFieldPath() {

  if (parent == nullptr)
    return nullptr;
  AccessPath *new_ap = new AccessPath(rootVariable);
  auto it = new_ap;

  auto firstParent = parent;

  while (firstParent != nullptr) {
    if (firstParent->parent != nullptr) {
      it->parent = new AccessPath(firstParent->rootVariable);
      it = it->parent;
    }
    firstParent = firstParent->parent;
  }

  return new_ap;
}

AccessPath *AccessPath::getRoot() {

  if (parent == nullptr) {
    return this;
  } else {
    return parent->getRoot();
  }
}

void AccessPath::print() {
  if (parent != nullptr) {
    parent->print();
    outs() << "->";
  }

  if (isDerefPath())
    outs() << "*";
  else
    outs() << getShortValueName(rootVariable);
}
std::string AccessPath::str() {
  std::string s = "";
  if (parent != nullptr) {
    s += parent->str();
    s += "->";
  }

  if (isDerefPath())
    s += "*";
  else
    s += getShortValueName(rootVariable);
  return s;
}

AccessPath *AccessPath::getBase() {
  // Base is one link before curent, thus it is the parent of the access path
  // This would be nullptr for a root variable
  return parent;
}

std::vector<AccessPath *> getPrefixes(AccessPath *ap) {
  std::vector<AccessPath *> ret{};
  if (ap == nullptr)
    return ret;

  ret.push_back(new AccessPath(*ap));
  if (ap->parent == nullptr) {

    return ret;
  }

  auto it = ap->parent;

  while (it) {
    ret.push_back(new AccessPath(*it));
    it = it->parent;
  }
  return ret;
}

bool operator==(const AccessPath &lhs, const AccessPath &rhs) {
  if ((lhs.rootVariable == rhs.rootVariable) &&
      (lhs.parent == nullptr && rhs.parent == nullptr)) {
    return true;
  }

  if (lhs.rootVariable != rhs.rootVariable) {
    return false;
  }

  if (lhs.parent != nullptr && rhs.parent != nullptr) {
    return (*(lhs.parent) == (*rhs.parent));
  } else {
    return false;
  }
}

AccessPath *joinAccessPaths(AccessPath &lhs, AccessPath &rhs) {
  // We create a copy of the rhs,
  AccessPath *ap = new AccessPath(rhs);

  auto it = ap;
  // Get to the base of access path
  while (it->parent) {
    it = it->parent;
  }
  it->parent = new AccessPath(lhs);

  return ap;
}

bool operator<(const AccessPath &lhs, const AccessPath &rhs) {
  // a->b->c has prefix a, a->b thus, if lhs < rhs, then rhs contains lhs

  if (lhs.parent == nullptr && rhs.parent == nullptr)
    return false;
  auto parent = rhs.parent;

  while (parent) {
    if (lhs == *(parent))
      return true;
    parent = parent->parent;
  }

  return false;
}

bool operator<=(const AccessPath &lhs, const AccessPath &rhs) {
  return (lhs < rhs) || (lhs == rhs);
}
bool operator!=(const AccessPath &lhs, const AccessPath &rhs) {
  return !(lhs == rhs);
}

bool is_prefix(AccessPath &prefix, AccessPath &target) {

  return prefix <= target;
}

bool is_proper_prefix(AccessPath &prefix, AccessPath &target) {

  return prefix < target;
}
