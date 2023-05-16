#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

#define ZIM_PRIVATE
#include <zim/archive.h>
#include <zim/item.h>

class ZimFSNode
{
  std::string name;

public:
  zim::entry_index_type entry_index;
  std::unordered_map<std::string, ZimFSNode *> children;

  ZimFSNode() : name(""), entry_index(-1) {}
  ZimFSNode(const std::string &name, zim::entry_index_type index) : name(name), entry_index(index) {}

  void addEntry(const zim::Entry &entry)
  {
    std::stringstream ss(entry.getPath());
    std::string token;
    ZimFSNode *curr = this;
    while (getline(ss, token, '/'))
    {

      if (token.empty())
      {
        continue;
      }

      if (curr->children.find(token) == curr->children.end())
      {

        if (ss.eof())
        {
          curr->children[token] = new ZimFSNode(token, entry.getIndex());
        }
        else
        {
          curr->children[token] = new ZimFSNode(token, -1);
        }
      }
      curr = curr->children[token];
    }
  }

  ZimFSNode *findPath(const std::string &path)
  {
    std::stringstream ss(path);
    std::string token;
    ZimFSNode *curr = this;
    while (getline(ss, token, '/'))
    {
      if (token.empty())
      {
        continue;
      }

      if (curr->children.find(token) == curr->children.end())
      {
        return nullptr;
      }
      curr = curr->children[token];
    }
    return curr;
  }

  // Not used
  // void traverseTree(int depth = 0) const
  // {
  //   for (auto child : children)
  //   {
  //     std::cout << std::string(depth * 2, '-') << child.second->name << " " << child.second->getEntryIndex() << std::endl;
  //     child.second->traverseTree(depth + 1);
  //   }
  // }

  ~ZimFSNode()
  {
    for (auto child : children)
    {
      delete child.second;
    }
  }
};

class ZimFS
{
  zim::Archive archive;

public:
  ZimFSNode root;

  ZimFS(const std::string &fname, bool check_intergrity) : archive(fname)
  {

    if (check_intergrity && !archive.checkIntegrity(zim::IntegrityCheck::CHECKSUM))
    {
      throw "intergrity check failed";
    }

    // Cache all entries
    size_t counter = 0;
    for (auto &en : archive.iterEfficient())
    {
      root.addEntry(en);
      counter++;
    }
    std::cout << "All entries loaded, total: " << counter << std::endl;

    // root.traverseTree();
  }
};

ZimFS fs("test.zim", true);

static int do_getattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi)
{
  std::cout << "do_getattr: " << path << std::endl;

  ZimFSNode *node = fs.root.findPath(path);

  if (node == nullptr)
  {
    return -ENOENT;
  }

  if (node->entry_index == -1)
  {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 1;
  }
  else
  {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = 1024;
  }

  return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t fill_dir, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
  std::cout << "do_readdir: " << path << std::endl;

  ZimFSNode *node = fs.root.findPath(path);

  if (node == nullptr)
  {
    return -ENOENT;
  }

  for (auto child : node->children)
  {
    fill_dir(buf, child.first.c_str(), nullptr, 0, (fuse_fill_dir_flags)0);
  }
  return 0;
}

static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir};

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &operations, nullptr);
}
