#include <vector>

int zimsplit(const std::vector<const char*>& args);

int main(int argc, char* argv[])
{
  std::vector<const char*> args(argv, argv + argc);
  return zimsplit(args);
}
