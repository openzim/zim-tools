#include "zimmount.h"

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

ZimFSNode::ZimFSNode() : name(""), entry_index(INVALID_INDEX) {};
ZimFSNode::ZimFSNode(const std::string& name, zim::entry_index_type index) : name(name), entry_index(index) {};

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

ZimFS* fs;

/* callbacks for fuse below */
static int do_getattr(const char* path,
                      struct stat* stbuf,
                      struct fuse_file_info* fi)
{
  std::cout << "do_getattr: " << path << std::endl;

  ZimFSNode* node = fs->root.findPath(path);

  if (node == nullptr) {
    return -ENOENT;
  }

  if (node->entry_index == INVALID_INDEX) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 1;
  } else {
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = 1024;
  }

  return 0;
}

static int do_readdir(const char* path,
                      void* buf,
                      fuse_fill_dir_t fill_dir,
                      off_t offset,
                      struct fuse_file_info* fi,
                      enum fuse_readdir_flags flags)
{
  std::cout << "do_readdir: " << path << std::endl;

  ZimFSNode* node = fs->root.findPath(path);

  if (node == nullptr) {
    return -ENOENT;
  }

  for (auto child : node->children) {
    fill_dir(buf, child.first.c_str(), nullptr, 0, (fuse_fill_dir_flags)0);
  }
  return 0;
}

static struct fuse_operations operations
    = {.getattr = do_getattr, .readdir = do_readdir};

/* Code for processing custom args below */
struct zimfile_opt {
  char* path;
};

#define KEY_PATH 1
#define ZIM_OPT(t, m, v)                  \
  {                                       \
    t, offsetof(struct zimfile_opt, m), v \
  }

static int zimfile_opt_proc_fd(void* data,
                               const char* arg,
                               int key,
                               struct fuse_args* outargs)
{
  struct zimfile_opt* opt = (struct zimfile_opt*)data;

  switch (key) {
    case KEY_PATH:
      opt->path = strdup(arg);
      return 0; /* Discard since it has been read */
  }

  return 1; /* Keep */
}

int main(int argc, char* argv[])
{
  /* Custom args (zimfile path) */
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  struct zimfile_opt opt;
  memset(&opt, 0, sizeof(opt));

  struct fuse_opt opts[] = {ZIM_OPT("path=%s", path, KEY_PATH), FUSE_OPT_END};
  fuse_opt_parse(&args, &opt, opts, zimfile_opt_proc_fd);

  if (opt.path == nullptr) {
    std::cerr << "error: no zimfile path specified, use -o "
                 "path=/path/to/zimfile to specify the zimfile to mount."
              << std::endl;
    return 1;
  }
  /* init global var `fs` */
  fs = new ZimFS(opt.path, true);

  /* direct call to fuse_main with custom callbacks */
  return fuse_main(args.argc, args.argv, &operations, nullptr);
}
