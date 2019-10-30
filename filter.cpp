#include "filter.h"
#include "glm/glm.hpp"

bool Filter::checkPoint(VRPoint& p)
{
  return true;
}



MotionFilter::MotionFilter(){
  threshold = 0.01f;
}

MotionFilter::MotionFilter(float thresh){
  threshold = thresh;
}

bool MotionFilter::checkPoint(VRPoint& p){
  float dist;
  if (p.positions.size() == 0){
    return false;
  }
  return (p.totalPathLength()  > threshold);
}


SliceFilter::SliceFilter(float start, float gap, float size) : start(start), gap(gap), size(size){
  
}
/*
SliceFilter::SliceFilter() : start(0), gap(.04), size(.005){

}*/

bool SliceFilter::checkPoint(VRPoint& p){
  float z = p.positions[0].z;
  if ( fmod(1000*gap - start + z, gap) <= size){
    return true;
  }
  return false;
}

void SliceFilter::addStart(float amt){
  start += amt;
}

void SliceFilter::addGap(float amt){
  gap += amt;
}

void SliceFilter::addSize(float amt){
  size += amt;
}


/* Old version
bool MotionFilter::checkPoint(VRPoint& p){
  float dist;
  if (p.positions.size() == 0){
    return false;
  }
  glm::vec3 startPos = p.positions[0];

  for(int i = 1; i < p.positions.size(); i++){
    float d = (startPos - p.positions[i]).magnitude();
    if (d > dist){
      dist = d;
    }
  }
  return (dist > threshold);
}*/
