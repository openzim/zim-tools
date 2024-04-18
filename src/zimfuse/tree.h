#ifndef _ZIM_TREE_H
#define _ZIM_TREE_H

#include <zim/archive.h>
#include "node.h"
#include <unordered_map>

class Tree
{
public:
    Tree(const std::string &path);
    std::pair<Node*, bool> attachNode(const std::string& path, Node* parent);
    std::pair<Node*, bool> attachFile(const std::string& path, Node* parent);
    Node* findNode(const std::string& name);
    zim::Archive* getArchive() { return &zimArchive; } 
    ~Tree();
private:
    std::unordered_map<std::string, Node::Ptr> mappedNodes;
    zim::Archive zimArchive;
    Node::Ptr rootNode;
};

#endif