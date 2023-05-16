#ifndef OPENZIM_ZIMMOUNT_H
#define OPENZIM_ZIMMOUNT_H

#include <string>
#include <unordered_map>

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>

#define INVALID_INDEX 0xffffffff

class ZimFSNode
{
  std::string name;

 public:
  zim::entry_index_type entry_index;
  std::unordered_map<std::string, ZimFSNode*> children;

  ZimFSNode();
  ZimFSNode(const std::string& name, zim::entry_index_type index);

  void addEntry(const zim::Entry& entry);
  ZimFSNode* findPath(const std::string& path);
  ~ZimFSNode();
};

class ZimFS
{
  zim::Archive archive;

 public:
  ZimFSNode root;
  ZimFS(const std::string& fname, bool check_intergrity);
};

#endif  // OPENZIM_ZIMMOUNT_H
