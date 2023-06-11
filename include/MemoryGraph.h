#pragma once

#include <string>
#include <vector>

#include "Common.h"
// Memory Graph is implemented using adjacency nodes

struct Node {
  // This value is NULL for non rootVariable
  Value *nodeValue;
  bool isMemoryNode;
  bool isFieldNode;
  int idx;

  Node(bool isMemoryNode, int idx)
      : nodeValue(nullptr), isMemoryNode(isMemoryNode), idx(idx){};
  Node(Value *V, int idx) : nodeValue(V), isMemoryNode(false), idx(idx){};
  Node(bool isMemoryNode, bool isFieldNode, Value *v, int idx)
      : nodeValue(v), idx(idx), isMemoryNode(isMemoryNode),
        isFieldNode(isFieldNode){};
  Node(const Node &N, int idx)
      : nodeValue(N.nodeValue), isMemoryNode(N.isMemoryNode), idx(idx),
        isFieldNode(N.isFieldNode){};

  std::string str();
};

struct Edge {
  struct Node *src;
  struct Node *dest;
  Value *edgeLabel;
  Edge(Node *src, Node *dest, Value *edgeLabel)
      : src(src), dest(dest), edgeLabel(edgeLabel){};

  bool isDerefEdge();

  std::string str();
};

class MemoryGraph {
public:
  std::vector<std::vector<struct Edge *>> graph;

  std::vector<struct Node *> nodes;
  std::vector<struct Node *> mallocNodes;

  bool hasNode(Node *node);

  // Always use with hasNode
  Node *getNodeFromValue(Value *V);

  int getNodeIdx(Node *N);

  Node *getTargetMemoryNode(Node *N);

  Node *getTargetFieldNode(Node *N, Value *V);

  Node *getTargetDerefNode(Node *N);

  Value *getRootValueForMemNode(Node *N);

  Node *getFieldMemoryNode(Node *N);

  Node *getOrAddNode(bool isMemoryNode, bool isFieldNode, Value *v = nullptr);

  bool hasGlobalAccessor(Node *N);

  void addEdge(Node *src, Node *dest, struct Edge *edge);

  void print();
};
