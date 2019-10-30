#include "vrpoint.h"
#include "pointmanager.h"

#include <vector>
#include <utility>

/***
 * A SimilarityEvaluator is a class that implements method of computing similarity between pathlines.
 * It also implements methods to compute different things with these similarity metrics (ie find N closest paths)
 * Designed to be subclassed with different implementations of the evaluateSimilarity method.
 * 
 * It feels like I should have one abstract class for the representation of general computing similarity
 * and another for doing computations, but eh.
 */
class SimilarityEvaluator{
public:
  virtual double evaluateSimilarity(VRPoint& a, VRPoint& b) = 0;
  virtual std::vector<int> getMostSimilarPaths(PointManager* pm, int targetIndex, int numPaths = 25);
  virtual std::vector<int> getMostSimilarPaths(PointManager* pm, std::vector<int> targetIndices, int numPaths = 25);
  virtual std::vector<int> getMostSimilarPathsFromSimilarities(std::vector<std::pair<int, double>> similarities, int numPaths = 25);
  virtual std::vector<std::pair<int, double>> getAllPathSimilarities(PointManager* pm, int targetIndex);
  virtual std::vector<std::pair<int, double>> getAllPathSimilarities(PointManager* pm, std::vector<int> targetIndices);
  float threshold = 0.007;
};
