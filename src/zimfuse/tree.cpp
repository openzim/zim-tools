#include "tree.h"

#include <string>

std::string sanitize(const std::string& name)
{
  if (name.size() > 500)
    return name.substr(0, 500);
  return name;
}

std::pair<Node*, bool> Tree::attachFile(const std::string& name, Node* parent, int collisionCount = 0)
{
  auto sanitizedName = sanitize(name);
  if (collisionCount)
    sanitizedName += '(' + std::to_string(collisionCount) + ')';
  auto fullPath = parent->fullPath + '/' + sanitizedName;
  auto originalPath = parent->originalPath + '/' + name;
  if (mappedNodes.count(fullPath)) {
    auto node = mappedNodes[fullPath].get();
    node->collisionCount++;
    return attachFile(name, parent, node->collisionCount);
  }
  
  auto n = Node::Ptr(new Node{.name = sanitizedName,
                              .isDir = false,
                              .parent = parent,
                              .fullPath = fullPath,
                              .originalPath = originalPath});
  mappedNodes[fullPath] = std::move(n);
  return {mappedNodes[fullPath].get(), true};
}

std::pair<Node*, bool> Tree::attachNode(const std::string& path, Node* parent)
{
  auto i = path.find_first_of('/');
  if (i == std::string::npos) {
    return attachFile(path, parent);
  }

  auto ss = path.substr(0, i);
  auto fullPath = parent->fullPath + '/' + ss;
  auto n = Node::Ptr(new Node{.name = ss,
                              .isDir = true,
                              .parent = parent,
                              .fullPath = fullPath,
                              .originalPath = fullPath});
  bool attach = true;
  if (mappedNodes.count(fullPath)) {
    n = std::move(mappedNodes[fullPath]);
    attach = false;
  }
  auto child = attachNode(path.substr(i + 1), n.get());
  if (child.second && child.first) {
    n->addChild(child.first);
  }
  mappedNodes[fullPath] = std::move(n);
  return {mappedNodes[fullPath].get(), attach};
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
                        .fullPath = "",
                        .originalPath = ""});

  for (auto it : zimArchive.iterByPath()) {
    auto nChild = attachNode(it.getPath(), rootNode.get());
    if (nChild.second)
      rootNode->addChild(nChild.first);
  }
  mappedNodes["/"] = std::move(rootNode);
}

Tree::~Tree() {}