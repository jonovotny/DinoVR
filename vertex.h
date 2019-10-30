#ifndef VERTEX_H
#define VERTEX_H
#include "glm/glm.hpp"

/**
 * Represents a vertex to send to OpenGL
 * Currently for convenience only have the one form for all possible applications (should change this probably).
 */
struct Vertex{
public:
  glm::vec3 position;
  glm::vec2 color;
  glm::vec3 spare;
  Vertex(glm::vec3 pos, glm::vec2 col, glm::vec3 s){
    position = pos;
    color = col;
    spare = s;
  }
  Vertex(glm::vec3 pos, glm::vec2 col){
    position = pos;
    color = col;
  }
  Vertex(glm::vec3 pos){
    position = pos;
    color = glm::vec2(0.0, 0.0);
  }
  Vertex(){
    position = glm::vec3(0.0, 0.0, 0.0);
    color = glm::vec2(0.0, 0.0);
    spare = glm::vec3(0.0, 0.0, 0.0);
  }
};

#endif // POINTMANAGER_H
