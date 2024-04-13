#ifndef _ZIM_NODE_H
#define _ZIM_NODE_H

#include <memory>
#include <string>
#include <vector>

struct Node {
  using Ptr = std::unique_ptr<Node>;
  std::string name;
  bool isDir = false;
  Node* parent;
  std::string originalPath;
  std::vector<Node*> children;

  void addChild(Node* child)
  {
    if (child)
      children.push_back(child);
  }
};

#endif