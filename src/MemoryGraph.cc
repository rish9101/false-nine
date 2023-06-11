#include "MemoryGraph.h"

int memLoc = 0;

std::string Node::str() {
  if (nodeValue != nullptr) {
    return formatv("[{0}]", getShortValueName(nodeValue));
  }
  if (isMemoryNode == true) {
    return formatv("[M{0}]", idx);
  } else {
    return "[]";
  }
}

bool operator==(const Node &lhs, const Node &rhs) {
  return (lhs.isMemoryNode == rhs.isMemoryNode) &&
         (lhs.nodeValue == rhs.nodeValue) &&
         (lhs.isFieldNode == rhs.isFieldNode) && (lhs.idx == rhs.idx);
}

std::string Edge::str() {
  if (edgeLabel != nullptr) {
    return formatv("-({0})->", getShortValueName(edgeLabel));
  }
  if (isDerefEdge() == true) {
    return formatv("-*->");
  }
  if (edgeLabel == nullptr) {
    return "->";
  }
  return "";
}

bool Edge::isDerefEdge() {
  return (edgeLabel == nullptr && src->isMemoryNode && dest->isMemoryNode);
}

bool operator==(const Edge &lhs, const Edge &rhs) {
  return (lhs.src == rhs.src && lhs.edgeLabel == rhs.edgeLabel);
}

bool MemoryGraph::hasNode(Node *node) {
  for (auto n : nodes) {
    if (*n == *node)
      return true;
  }
  return false;
}

Node *MemoryGraph::getNodeFromValue(Value *V) {
  for (auto node : nodes) {
    if (node->nodeValue == V)
      return node;
  }
  return nullptr;
}

int MemoryGraph::getNodeIdx(Node *N) {
  for (auto node : nodes) {
    if (*node == *N)

      return std::find(nodes.begin(), nodes.end(), node) - nodes.begin();
  }
  return -1;
}

Node *MemoryGraph::getTargetMemoryNode(Node *N) {
  if (hasNode(N)) {
    auto idx = getNodeIdx(N);
    if (idx == -1)
      return nullptr;
    for (auto edge : graph[idx]) {
      if (edge != nullptr && edge->dest->isMemoryNode)
        return edge->dest;
    }
  }
  return nullptr;
}

Node *MemoryGraph::getTargetFieldNode(Node *N, Value *V) {
  if (hasNode(N)) {
    auto idx = getNodeIdx(N);
    if (idx == -1)
      return nullptr;
    for (auto edge : graph[idx]) {
      if (edge && dyn_cast<ConstantInt>(edge->edgeLabel)->getValue() ==
                      dyn_cast<ConstantInt>(V)->getValue()) {
        return edge->dest;
      }
    }
  }
  return nullptr;
}

Node *MemoryGraph::getTargetDerefNode(Node *N) {
  if (hasNode(N)) {
    auto idx = getNodeIdx(N);
    if (idx == -1)
      return nullptr;
    for (auto edge : graph[idx]) {
      if (edge != nullptr && edge->isDerefEdge())
        return edge->dest;
    }
  }
  return nullptr;
}

Value *MemoryGraph::getRootValueForMemNode(Node *N) {
  if (N->isMemoryNode && N->isFieldNode) {
    return nullptr;
  }

  auto idx = getNodeIdx(N);
  for (auto vec : graph) {
    auto edge = vec[idx];
    if (edge && edge->src->nodeValue != nullptr) {
      return edge->src->nodeValue;
    }
  }
  return nullptr;
}

Node *MemoryGraph::getFieldMemoryNode(Node *N) {
  if (N->isFieldNode == false) {
    return nullptr;
  }

  int idx = getNodeIdx(N);
  for (auto vec : graph) {
    auto edge = vec[idx];
    if (edge && edge->src->isMemoryNode && !edge->src->isFieldNode)
      return edge->src;
  }
  return nullptr;
}

Node *MemoryGraph::getOrAddNode(bool isMemoryNode, bool isFieldNode, Value *v) {
  if (v != nullptr && getNodeFromValue(v) != nullptr)
    return getNodeFromValue(v);
  auto node = new Node(isMemoryNode, isFieldNode, v, memLoc);

  nodes.push_back(node);

  if (isMemoryNode && !isFieldNode)
    mallocNodes.push_back(node);
  // Add this node to the graph
  std::vector<struct Edge *> newNode;
  for (int i = 0; i < nodes.size(); ++i) {
    newNode.push_back(nullptr);
  }

  // Add an entry in the adjacency graph in every row for the newly added Node
  for (int i = 0; i < (nodes.size() - 1); ++i) {
    graph[i].push_back(nullptr);
  }
  graph.push_back(newNode);
  // if (isMemoryNode)
  memLoc++;
  return node;
}

bool MemoryGraph::hasGlobalAccessor(Node *N) {
  if (hasNode(N)) {
    int idx = getNodeIdx(N);
    for (auto vec : graph) {
      auto edge = vec[idx];
      if (edge && edge->src->nodeValue != nullptr &&
          isa<GlobalVariable>(edge->src->nodeValue))
        return true;
    }
    return false;
  }
  return false;
}

void MemoryGraph::addEdge(Node *src, Node *dest, struct Edge *edge) {
  auto srcIdx = getNodeIdx(src);
  auto destIdx = getNodeIdx(dest);

  if (srcIdx == -1 || destIdx == -1) {
    outs() << "src or dest Nodes Do not exit in graph yet!\n";
  }
  graph[srcIdx][destIdx] = edge;
  return;
}

void MemoryGraph::print() {
  int i = 0;
  for (auto vec : graph) {
    outs() << formatv("PRINTING EDGES FOR {0}\n", nodes[i]->str());

    for (auto edge : vec) {
      if (edge != nullptr)
        outs() << formatv("{0}{1}{2}", edge->src->str(), edge->str(),
                          edge->dest->str())
               << "  ,  ";
    }

    outs() << "\n";
    i++;
  }
}