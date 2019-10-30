#include <vector>
#include "glm/glm.hpp"


#include "vertex.h"

#ifndef VRPOINT_H
#define VRPOINT_H


class VRPoint {

public:
	VRPoint();
	VRPoint(int id);
    Vertex Draw(int time, glm::vec3 minV, glm::vec3 maxV);
    void AddPoint(glm::vec3 point);
    glm::vec2 GetColor(int time, glm::vec3 minV, glm::vec3 maxV);


    float GetDistance(int time, glm::vec3 point);

    float totalPathLength();

    bool withinDistance(VRPoint& other, double distance);

    std::vector<Vertex> getPathlineVerts(bool useHardY = false, float hardYPos = 0.0);
    std::vector<glm::vec3> positions; //vector of positions the particle takes in its path

    int steps();
    int m_id; // id number to identify each particle
private:
    glm::vec2 getColor(int);
    glm::vec4 GetColorHorizontalPosition();

};
#endif // VRPOINT_H
