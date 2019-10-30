#include "SimilarityEvaluator.h"

#include <algorithm>



///To pass to std::nth_element for sorting purposes. 
struct pair_second_comp{
  bool operator()(std::pair<int, double>  a, std::pair<int, double> b){
    return a.second < b.second;
  }
};

std::vector<int> SimilarityEvaluator::getMostSimilarPathsFromSimilarities(std::vector<std::pair<int, double>> similarities, int numPaths){
  std::nth_element(similarities.begin(), similarities.begin() + numPaths, similarities.end(), pair_second_comp());
  std::vector<int> ret(numPaths);
  for (int i = 0; i < numPaths; i++){
    ret[i] = similarities[i].first;
  }
  return ret;

}

std::vector<int> SimilarityEvaluator::getMostSimilarPaths(PointManager* pm, int targetIndex, int numPaths){
  std::vector<std::pair<int, double> > similarities = getAllPathSimilarities(pm, targetIndex); //get the set of all similarity values
  std::nth_element(similarities.begin(), similarities.begin() + numPaths, similarities.end(), pair_second_comp());
  std::vector<int> ret(numPaths);
  for (int i = 0; i < numPaths; i++){
    ret[i] = similarities[i].first;
  }
  return ret;

}

std::vector<int> SimilarityEvaluator::getMostSimilarPaths(PointManager* pm, std::vector<int> targetIndices, int numPaths) {
	std::vector<std::pair<int, double> > similarities = getAllPathSimilarities(pm, targetIndices); //get the set of all similarity values
	std::nth_element(similarities.begin(), similarities.begin() + numPaths, similarities.end(), pair_second_comp());
	std::vector<int> ret(numPaths);
	for (int i = 0; i < numPaths; i++) {
		ret[i] = similarities[i].first;
	}
	return ret;
}

std::vector<std::pair<int, double> > SimilarityEvaluator::getAllPathSimilarities(PointManager* pm, int targetIndex){
  int numPoints = pm->points.size();
  std::vector<std::pair<int, double> > values; 
  for (int i = 0; i < numPoints; i++){
    if( i == targetIndex){
      continue;
    }
    if (pm->points[i].totalPathLength() < threshold){
      continue;
    }
    values.push_back(std::make_pair(i, evaluateSimilarity(pm->points[targetIndex], pm->points[i]))); 
  }
  return values;
}

std::vector<std::pair<int, double> > SimilarityEvaluator::getAllPathSimilarities(PointManager* pm, std::vector<int> targetIndices) {
	int numPoints = pm->points.size();
	std::vector<std::pair<int, double> > values;
	for (int i = 0; i < numPoints; i++) {
		if (std::find(targetIndices.begin(), targetIndices.end(), i) != targetIndices.end()) {
			continue;
		}
		if (pm->points[i].totalPathLength() < threshold) {
			continue;
		}
		double minDiff = DBL_MAX;
		for (int j = 0; j < targetIndices.size(); j++) {
			minDiff = std::min(minDiff, evaluateSimilarity(pm->points[targetIndices[j]], pm->points[i]));
		}
		values.push_back(std::make_pair(i, minDiff));
	}
	return values;
}
