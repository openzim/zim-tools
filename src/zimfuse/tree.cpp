#include "tree.h"

#include <string>

std::pair<Node*, bool> Tree::attachNode(const std::string& path, Node* parent)
{
  auto i = path.find_first_of('/');
  if (i == std::string::npos) {
    auto ss = path;
    auto ogPath = parent->originalPath + '/' + ss;
    if (mappedNodes.count(ogPath))
      return {mappedNodes[ogPath].get(), false};
    auto n = Node::Ptr(new Node{.name = ss,
                                .isDir = false,
                                .parent = parent,
                                .originalPath = ogPath});
    mappedNodes[ogPath] = std::move(n);
    return {mappedNodes[ogPath].get(), true};
  }

  auto ss = path.substr(0, i);
  auto ogPath = parent->originalPath + '/' + ss;
  auto n = Node::Ptr(new Node{.name = ss,
                              .isDir = true,
                              .parent = parent,
                              .originalPath = ogPath});
  bool attach = true;
  if (mappedNodes.count(ogPath)) {
    n = std::move(mappedNodes[ogPath]);
    attach = false;
  }
  auto child = attachNode(path.substr(i + 1), n.get());
  if (child.second && child.first) {
    n->addChild(child.first);
  }
  mappedNodes[ogPath] = std::move(n);
  return {mappedNodes[ogPath].get(), attach};
}

Node* Tree::findNode(const std::string& name)
{
  if (mappedNodes.count(name))
    return mappedNodes[name].get();
  return nullptr;
}

Tree::Tree(const std::string& path) : zimArchive(path)
{
  rootNode = Node::Ptr(new Node{
                        .name = "/", 
                        .isDir = true, 
                        .parent = nullptr, 
                        .originalPath = ""});

  for (auto it : zimArchive.iterByPath()) {
    auto nChild = attachNode(it.getPath(), rootNode.get());
    if (nChild.second)
      rootNode->addChild(nChild.first);
  }
  mappedNodes["/"] = std::move(rootNode);
}

Tree::~Tree() {}