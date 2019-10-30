#include "vrpoint.h"
#include <algorithm>
#include <stdio.h>


VRPoint::VRPoint()
{
	m_id = 0;
}


VRPoint::VRPoint(int id)
{
    m_id = id;
}

Vertex VRPoint::Draw(int time, glm::vec3 minV, glm::vec3 maxV)
{
    if (time > positions.size()){
        return Vertex();
        //Fail silently
    }
    glm::vec3 p = positions[time];
   // printf("Drawing point at %f, %f, %f.\r", p.x, p.y, p.z);
    //Sphere s (p, 1e-4);
    //Draw::sphere(s, rd);
    glm::vec3 op = positions[0];
 
    //rd->sendVertex(p);
    return Vertex(p, GetColor(time, minV, maxV), op);
}


glm::vec4 interp(float val, float from, float to, glm::vec4 fromVal, glm::vec4 toVal)
{
  float d = (val - from) / (to - from);
  return fromVal * d + toVal * (1.f - d);
  
}


glm::vec4 getColorHorizontal(glm::vec3 pos)
{
    float val = pos.y;
    glm::vec4 backCol = glm::vec4(0, .6, 0, 1);
    glm::vec4 frontCol = glm::vec4(.6, 0, 1, 1);
    return interp(val, 0.069, 0.21, backCol, frontCol);
}


glm::vec4 VRPoint::GetColorHorizontalPosition()
{
    return getColorHorizontal(positions[0]);

}

float interpi(float val, float from, float to, int fromVal, int toVal)
{
  float d = (1.0 * val - from) / (1.0f * to - from);
  return  fromVal *  d + toVal * (1.f - d);
}


float bound(float x){
  if (x < 0){
    return 1;
  }
  else if (x > 511){
    return 511;
  }
  else
    return x;
}

glm::vec2 VRPoint::GetColor(int time, glm::vec3 minV, glm::vec3 maxV)
{
  glm::vec3 pos = positions[0];
  float x = interpi(pos.x, minV.x, maxV.x, 0, 1);
  float y = interpi(pos.y, minV.y, maxV.y, 0, 1);
  float z = interpi(pos.z, minV.z, maxV.z, 0, 1);
  return glm::vec2(z, y);
  // return GetColorHorizontalPosition(); 

}

void VRPoint::AddPoint(glm::vec3 point)
{
    positions.push_back(point);
}



float VRPoint::GetDistance(int time, glm::vec3 point)
{
  return glm::length(point - positions[time]);
}

glm::vec3 getVertexOffset(glm::vec3 right, glm::vec3 up, float theta){
  return float(cos(theta)) * up + float(sin(theta)) * right;
}

glm::vec3 getVertexPosition(glm::vec3 right, glm::vec3 up, glm::vec3 base, float radius, float theta){
  return base + radius * float(cos(theta)) * up + radius * float(sin(theta)) * right;
}


glm::vec2 VRPoint::getColor(int position){
  float pos = interpi(position, 0, positions.size(), 0, 1);
  return glm::vec2(pos, 0.5);
}

std::vector<Vertex> VRPoint::getPathlineVerts(bool useHardY, float hardYPos)
{
  double radius = 0.00008;
  std::vector<Vertex> v;
  std::vector<Vertex> points;
  std::vector<int> indices;
  int steps_around = 5;
  for(int i = 0; i < positions.size() -1; i++){
    glm::vec3 forward = positions[i+1] - positions[i];
    glm::vec3 wup = glm::vec3(0, 0, 1);
    glm::vec3 right = glm::normalize(glm::cross(wup, forward));
    glm::vec3 up = glm::normalize(glm::cross(forward,right));
    glm::vec2 col = getColor(i);
    if (useHardY){
      col.y = hardYPos;
    }
    for(int j = 0; j < steps_around; j++){
      points.push_back(Vertex(getVertexPosition(right, up, positions[i], radius, (j * 6.28 / steps_around)), col, getVertexOffset(right, up, (j * 6.28 / steps_around))));
    }
    for (int j = 0; j < steps_around - 1; j++){
      int base = steps_around * i +j;
      indices.push_back(base);
      indices.push_back(base + steps_around);

      indices.push_back(base + steps_around + 1);
      indices.push_back(base + 1);
    }
    int j = steps_around - 1;
    int base = steps_around * i + j;
    indices.push_back(base);
    indices.push_back(base + steps_around);
    indices.push_back(base + 1);
    indices.push_back(base - steps_around + 1);

  }
  int i = positions.size()-1;
  glm::vec3 forward = positions[i] - positions[i-1];
  glm::vec3 wup = glm::vec3(0,0,1);
  glm::vec3 right = glm::normalize(glm::cross(wup, forward));
  glm::vec3 up = glm::normalize(glm::cross(forward,right));
  glm::vec2 col = getColor(i);
  if (useHardY){
    col.y = hardYPos;
  }
  for(int j = 0; j < steps_around; j++){
    points.push_back(Vertex(getVertexPosition(right, up, positions[i], radius, (j * 6.28 / steps_around)), col, getVertexOffset(right, up, (j * 6.28 / steps_around))));
  }
    
    std::vector<int> triIndices;
    for (int i =0 ; i < indices.size(); i += 4){

        triIndices.push_back(indices[i]);
        triIndices.push_back(indices[i+1]);
        triIndices.push_back(indices[i+2]);
        
        triIndices.push_back(indices[i]);
        triIndices.push_back(indices[i+2]);
        triIndices.push_back(indices[i+3]);
    }

  for(int i = 0; i < triIndices.size(); i++){
    v.push_back(points[triIndices[i]]);
    Vertex a = points[triIndices[i]];
    //printf("Point at index %d,%d: %f, %f, %f: %f, %f\n", i, triIndices[i], a.position.x, a.position.y, a.position.z, a.color.x, a.color.y);
  }

  //printf("Indices size: %d, TriIndices size : %d, Final size: %d\n", indices.size(), triIndices.size(), v.size());
  return v;
}


int VRPoint::steps()
{
  return positions.size();
}


float VRPoint::totalPathLength()
{
  float totalDistance = 0;
  for(int i = 0; i < positions.size() - 1; i++){
    float d = glm::length(positions[i] - positions[i+1]);
    totalDistance += d;
  }
  return totalDistance;
}

bool VRPoint::withinDistance(VRPoint& other, double distance)
{
  int l1 = positions.size();
  int l2  = other.positions.size();
  int length = std::min(l1, l2);
  double d = distance * distance;
  for(int i = 0; i < positions.size(); i++){
    for(int j = 0; j < other.positions.size(); j++){
      float dist = glm::length(positions[i] - other.positions[j]);
      if ( dist < distance){
        return true;
      }
    }
  }
  return false;
}
