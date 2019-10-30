#include "SimilarityEvaluator.h"
#include <vector>
#include "glm/glm.hpp"

class PathAlignmentSimilarityEvaluator : public SimilarityEvaluator{
public:
  /**
   * Initializes a PathAlignmentSimilarityEvaluator with weights given for each type of similarity.
   * Defaults to entirely dependent on matching.
   */
  PathAlignmentSimilarityEvaluator(double match = 1.0, glm::vec3 rot = glm::vec3(0.), glm::vec3 trans = glm::vec3(0.)){
    matchWeight = match;
    rotWeight = rot;
    transWeight = trans;
  }
  virtual double evaluateSimilarity(VRPoint& a, VRPoint& b);
  double doEvaluate(std::vector<glm::vec3> &a, std::vector<glm::vec3> &b);
  glm::mat3 computeBaseMatrix(std::vector<glm::vec3> &a, std::vector<glm::vec3> &b);

private:
  float matchWeight;
  glm::vec3 rotWeight;
  glm::vec3 transWeight;

};
