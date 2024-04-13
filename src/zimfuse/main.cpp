#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <sys/stat.h>
#include <zim/archive.h>
#include <zim/item.h>

#include <string>

#include "node.h"
#include "tree.h"

static const Node* getNode(std::string nName)
{
  Tree* const tree = static_cast<Tree*>(fuse_get_context()->private_data);
  auto node = tree->findNode(nName);
  return node;
}

void setStat(struct stat* st, const Node* node)
{
  if (node->isDir) {
    st->st_mode = S_IFDIR | 0555;
    st->st_nlink = 1;
  } else {
    st->st_mode = S_IFREG | 0444;
    st->st_nlink = 1;
    Tree* const tree = static_cast<Tree*>(fuse_get_context()->private_data);
    st->st_size = tree->getArchive()
                      ->getEntryByPath(node->originalPath.substr(1))
                      .getItem(true)
                      .getSize();
  }
}

static int zimGetAttr(const char* path, struct stat* st, fuse_file_info* fi)
{
  const Node* const node = getNode(path);
  if (!node)
    return -ENOENT;
  setStat(st, node);
  return 0;
}

static int zimReadDir(const char* path,
                      void* buf,
                      fuse_fill_dir_t fill_dir,
                      off_t offset,
                      struct fuse_file_info* fi,
                      enum fuse_readdir_flags flags)
{
  const Node* const node = getNode(path);
  if (!node)
    return -ENOENT;
  {
    struct stat st;
    setStat(&st, node);
    fill_dir(buf, ".", &st, 0, (fuse_fill_dir_flags)0);
  }

  if (const Node* const parent = node->parent) {
    struct stat st;
    setStat(&st, parent);
    fill_dir(buf, "..", &st, 0, (fuse_fill_dir_flags)0);
  } else {
    fill_dir(buf, "..", nullptr, 0, (fuse_fill_dir_flags)0);
  }

  for (auto child : node->children) {
    struct stat st;
    setStat(&st, child);
    fill_dir(buf, child->name.c_str(), &st, 0, (fuse_fill_dir_flags)0);
  }

  return 0;
}

static int zimRead(const char* path,
                   char* buf,
                   size_t size,
                   off_t offset,
                   struct fuse_file_info* fi)
{
  const Node* const node = getNode(path);
  Tree* const tree = static_cast<Tree*>(fuse_get_context()->private_data);
  if (!node)
    return -ENOENT;
  const std::string strPath(path);
  zim::Entry entry = tree->getArchive()->getEntryByPath(strPath.substr(1));
  zim::Item item = entry.getItem(true);

  if (offset >= (off_t)item.getSize()) {
    return 0;
  }

  if (offset + size > item.getSize()) {
    size = item.getSize() - offset;
  }

  memcpy(buf, item.getData().data() + offset, size);

  return size;
}

static int zimOpen(const char* path, struct fuse_file_info* fi)
{
  if ((fi->flags & 3) != O_RDONLY) {
    return -EACCES;
  }

  const Node* const node = getNode(path);
  if (!node)
    return -ENOENT;
  if (node->isDir)
    return -EISDIR;
  return 0;
}

static struct fuse_operations ops = {
    .getattr = zimGetAttr,
    .open = zimOpen,
    .read = zimRead,
    .readdir = zimReadDir,
};

enum {
  KEY_HELP,
  KEY_VERSION,
};

struct Param {
  int str_arg_count = 0;
  std::string filename;
  std::string mount_point;
  ~Param() {}
};

void printUsage()
{
  fprintf(stderr,
          R"(Mounts a ZIM file as a FUSE filesystem

Usage: zimfuse [options] <ZIM-file> [mount-point]

General options:
    --help    -h           print help
    --version              print version
)");
}

void printVersion()
{
  fprintf(stderr, "1.0");
}

static int processArgs(void* data, const char* arg, int key, fuse_args* outargs)
{
  Param& param = *static_cast<Param*>(data);

  const int KEEP = 1;
  const int DISCARD = 0;
  const int ERROR = -1;

  switch (key) {
    case KEY_HELP:
      printUsage();
      fuse_opt_add_arg(outargs, "-ho");
      fuse_main(outargs->argc, outargs->argv, &ops, nullptr);
      std::exit(EXIT_SUCCESS);

    case KEY_VERSION:
      printVersion();
      fuse_opt_add_arg(outargs, "--version");
      fuse_main(outargs->argc, outargs->argv, &ops, nullptr);
      std::exit(EXIT_SUCCESS);

    case FUSE_OPT_KEY_NONOPT:
      switch (++param.str_arg_count) {
        case 1:
          param.filename = arg;
          return DISCARD;

        case 2:
          param.mount_point = arg;
          return KEEP;

        default:
          fprintf(
              stderr,
              "zimfuse: only two arguments allowed: filename and mountpoint\n");
          return ERROR;
      }

    default:
      return KEEP;
  }
}

int main(int argc, char* argv[])
{
  fuse_args args = FUSE_ARGS_INIT(argc, argv);
  Param param;

  const fuse_opt opts[] = {FUSE_OPT_KEY("--help", KEY_HELP),
                           FUSE_OPT_KEY("-h", KEY_HELP),
                           FUSE_OPT_KEY("--version", KEY_VERSION),
                           {nullptr, 0, 0}};

  if (fuse_opt_parse(&args, &param, opts, processArgs))
    return EXIT_FAILURE;

  if (param.filename.empty()) {
    printUsage();
    return EXIT_FAILURE;
  }
  std::cout << param.filename << " " << param.mount_point << std::endl;
  auto tree = new Tree(param.filename);
  fuse_opt_add_arg(&args, "-s");

  auto ret = fuse_main(args.argc, args.argv, &ops, tree);
  fuse_opt_free_args(&args);
  return ret;
}
