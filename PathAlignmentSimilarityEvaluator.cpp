#include "PathAlignmentSimilarityEvaluator.h"
//#include "svd3.h"
#include <algorithm>
#include "glm_svd.h"
#include "glm/gtx/norm.hpp"

void printMat3(glm::mat3 m){
  for (int i = 0; i< 3; i++){
    for (int j = 0; j < 3; j++){
      printf("%f  ", m[j][i]);
    }
    printf("\n");
  }
  printf("\n");

}


#define TIMING
#define PRINT_DEBUG
#ifdef TIMING
  #undef PRINT_DEBUG
#endif

double PathAlignmentSimilarityEvaluator::evaluateSimilarity(VRPoint& a, VRPoint& b){
  return doEvaluate(a.positions, b.positions);
}

double PathAlignmentSimilarityEvaluator::doEvaluate(std::vector<glm::vec3> &a, std::vector<glm::vec3> &b){
  glm::mat3 H = computeBaseMatrix(a, b);
  glm::mat3 U, S, V;
  svd(H, U, S, V);

  #ifdef PRINT_DEBUG 
  printf("U\n");
  printMat3(U);
  printf("S\n");
  printMat3(S);
  printf("V\n");
  printMat3(V);
  #endif

  #ifdef PRINT_DEBUG
  printf("U*S*V (should be identical to base)\n");
  printMat3(U * S * glm::transpose( V));
  #endif

  glm::mat3 R = V * glm::transpose(U);
  //I'm pretty certain I can find a better way
  glm::vec3 a_mean, b_mean;
  int size = std::min(a.size(), b.size());
  for (int i = 0; i < size; i++){
    a_mean += a[i];
    b_mean += b[i];
  }
  a_mean /= size;
  b_mean /= size;
  glm::vec3 T = - R * a_mean + b_mean;


  double error = 0;
  for (int i = 0; i < size; i++){
    error += glm::l2Norm(R * a[i] + T, b[i]);
  }
 
  glm::quat rotationQuat = glm::toQuat(R);
  glm::vec3 rotationAngles = glm::eulerAngles(rotationQuat);
  
  #ifdef PRINT_DEBUG
  printf("Shape similarity: %f.\n", error);
  printf("Rotation: \n");
  printMat3(R);
  printf("Translation: %f, %f, %f.\n", T.x, T.y, T.z);
  printf("rotation angles: %f, %f, %f.\n", rotationAngles.x, rotationAngles.y, rotationAngles.z);
  #endif

  
  double similarity = error * matchWeight + glm::dot(T, transWeight) + glm::dot(rotationAngles, rotWeight);
  return similarity;
}

glm::mat3 PathAlignmentSimilarityEvaluator::computeBaseMatrix(std::vector<glm::vec3> &a, std::vector<glm::vec3> &b){
  int size = std::min(a.size(), b.size());
  //return glm::mat3();
  glm::vec3 a_mean, b_mean;
  for (int i = 0; i < size; i++){
    a_mean += a[i];
    b_mean += b[i];
  }
  a_mean /= size;
  b_mean /= size;

  glm::mat3 s(0);
  for (int i = 0; i < size; i++){
    s += glm::outerProduct(a[i] - a_mean, b[i] - b_mean);
  }
  #ifdef PRINT_DEBUG
  printf("Base matrix.\n");
  printMat3(s);
  #endif
  return s;
}


void test(){
  VRPoint a(0);
  VRPoint b(1);
  a.AddPoint(glm::vec3(0,0,0));
  a.AddPoint(glm::vec3(0,0,3));
  a.AddPoint(glm::vec3(0,3,3));
  b.AddPoint(glm::vec3(0,0,0));
  b.AddPoint(glm::vec3(0,0,3));
  b.AddPoint(glm::vec3(3,0,3));
  
  PathAlignmentSimilarityEvaluator p;
  
  double d;
  #ifdef TIMING
  clock_t start, end;
  start = clock();
  int n = 1e5;
  for (int i = 0; i < n; i++){
  #endif
  d = p.evaluateSimilarity(a,b);
  #ifdef TIMING
  }
  end = clock();
  printf("Average time was %f milliseconds.\n", 1e3 * (double(end - start) / n / CLOCKS_PER_SEC));
  #endif

  printf("Evaluated as %f.\n", d);
}

#ifdef TESTING
int main(){
  test();
  return 0;
}
#endif
