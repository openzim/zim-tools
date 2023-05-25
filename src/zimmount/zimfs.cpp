#include "zimfs.h"

#include <sstream>

ZimFSNode::ZimFSNode() : name(""), entry_index(INVALID_INDEX){};
ZimFSNode::ZimFSNode(const std::string& name, zim::entry_index_type index)
    : name(name), entry_index(index){};

void ZimFSNode::addEntry(const zim::Entry& entry)
{
  std::stringstream ss(entry.getPath());
  std::string token;
  ZimFSNode* curr = this;
  while (getline(ss, token, '/')) {
    if (token.empty()) {
      continue;
    }

    if (curr->children.find(token) == curr->children.end()) {
      if (ss.eof()) {
        curr->children[token] = new ZimFSNode(token, entry.getIndex());
      } else {
        curr->children[token] = new ZimFSNode(token, INVALID_INDEX);
      }
    }
    curr = curr->children[token];
  }
}

ZimFSNode* ZimFSNode::findPath(const std::string& path)
{
  std::stringstream ss(path);
  std::string token;
  ZimFSNode* curr = this;
  while (getline(ss, token, '/')) {
    if (token.empty()) {
      continue;
    }

    if (curr->children.find(token) == curr->children.end()) {
      return nullptr;
    }
    curr = curr->children[token];
  }
  return curr;
}

ZimFSNode::~ZimFSNode()
{
  for (auto child : children) {
    delete child.second;
  }
}

ZimFS::ZimFS(const std::string& fname, bool check_intergrity) : archive(fname)
{
  if (check_intergrity
      && !archive.checkIntegrity(zim::IntegrityCheck::CHECKSUM)) {
    throw std::runtime_error("zimfile intergrity check failed");
  }

  // Cache all entries
  size_t counter = 0;
  for (auto& en : archive.iterByPath()) {
    root.addEntry(en);
    counter++;
  }
  std::cout << "All entries loaded, total: " << counter << std::endl;
}
