#include "GLUtil.h"
#include "glm/glm.hpp"
#include "pointmanager.h"
#include <vector>
#include <string>

/**
 * Representation of pointer.
 */
class Cursor{
public:
  Cursor() {};
  Cursor(float length);

  /**
   * Initialize
   * @param length
   */
  void Initialize(float length);

  /**
   * Call to draw a pointer.
   * @param mvp
   */
  void Draw(glm::mat4 mvp);

private:
  std::vector<glm::vec3> verts;
  float len = 0.01;
  MyShader* shader;
  GLuint vao;
  GLuint vbo;
  int numUVs;
  int numVerts;
};
