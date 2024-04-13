#include "tree.h"

#include <string>

std::pair<Node*, bool> Tree::attachNode(std::string path, Node* parent)
{
  auto i = path.find_first_of('/');
  if (i == std::string::npos) {
    auto ss = path;
    auto ogPath = parent->originalPath + '/' + ss;
    if (mappedNodes.count(ogPath))
      return {mappedNodes[ogPath], false};
    auto n = new Node{
        .name = ss, .isDir = false, .parent = parent, .originalPath = ogPath};
    mappedNodes[ogPath] = n;
    return {n, true};
  }

  auto ss = path.substr(0, i);
  auto ogPath = parent->originalPath + '/' + ss;
  auto n = new Node{
      .name = ss, .isDir = true, .parent = parent, .originalPath = ogPath};
  bool attach = true;
  if (mappedNodes.count(ogPath)) {
    n = mappedNodes[ogPath];
    attach = false;
  }
  auto child = attachNode(path.substr(i + 1), n);
  if (child.second && child.first) {
    n->addChild(child.first);
  }
  mappedNodes[ogPath] = n;
  return {n, attach};
}

void printTree(Node* root)
{
  for (auto child : root->children) {
    std::cout << child->originalPath << std::endl;
    if (child->isDir)
      printTree(child);
  }
}

Node* Tree::findNode(std::string name)
{
  if (mappedNodes.count(name))
    return mappedNodes[name];
  return nullptr;
}

Tree::Tree(const std::string& path) : zimArchive(path)
{
  rootNode = new Node{
      .name = "/", .isDir = true, .parent = nullptr, .originalPath = ""};
  mappedNodes["/"] = rootNode;

  for (auto it : zimArchive.iterByPath()) {
    auto nChild = attachNode(it.getPath(), rootNode);
    if (nChild.second)
      rootNode->addChild(nChild.first);
  }
}

void cleanup(Node* root)
{
  for (auto child : root->children) {
    if (child->isDir)
      cleanup(child);
    delete child;
  }
}

Tree::~Tree()
{
  cleanup(rootNode);
  delete rootNode;
}