#include "pointmanager.h"
#include <string>
#include "vrpoint.h"

/*
 * PathFilter provides an alternate entry point to the program.
 * It allows for precomputation of sets of particles to display as path.
 * Currently only the Poisson method is implemented.
 */
class PathFilter
{
public:
  void run(std::string pointFile, std::string outFile, int numPaths = 1000, double gap = 0.005, double requiredLength = 0.003);

  void filterPoisson(int number=100, double gap = 0.001, double requiredLength = 0.01);

private:
  void runFilter();
  PointManager pm;
  std::vector<int> paths;
};
