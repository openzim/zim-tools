#include <vector>

int zimsplit(const std::vector<const char*>& args);

int main(int argc, char* argv[])
{
  return zimsplit({argv + 1, argv + argc});
}
