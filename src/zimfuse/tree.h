#ifndef _ZIM_TREE_H
#define _ZIM_TREE_H

#include <zim/archive.h>
#include "node.h"
#include <unordered_map>

class Tree
{
public:
    Tree(const std::string &path);
    std::pair<Node*, bool> attachNode(std::string path, Node* parent);
    Node* findNode(std::string name);
    zim::Archive* getArchive() { return &zimArchive; } 
    ~Tree();
private:
    std::unordered_map<std::string, Node*> mappedNodes;
    zim::Archive zimArchive;
    Node *rootNode;
};

#endif