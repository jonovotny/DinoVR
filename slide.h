#include "GLUtil.h"
#include "glm/glm.hpp"
#include "pointmanager.h"
#include <vector>
#include <string>

/**
 * Representation of a 2-d image in the program.
 */
class Slide{
public:
  Slide()  {};
  Slide(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up);
  /**
   * Initializes the slide given the image to load and coordinates.
   * @param texture File path of the image to load and show on the slide.
   * @param bl bottom left corner of the slide in 3d space
   * @param right vector in space showing direction and distance to the bottom right corner.
   * @param up vector in space showing direction and distance to the top left corner.
   */
  void Initialize(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up);

  /**
   * Call to draw the slide.
   * @param mvp MVP matrix to draw the slide with
   * note to self: Should probably put some management of orientation within this class at some point.
   */
  void Draw(glm::mat4 mvp);

private:
  std::vector<glm::vec3> verts;
  std::vector<glm::vec2> uvs;
  bool initialized = false;
  MyShader* shader;

  GLuint vertBuffer;
  GLuint uvBuffer;

  int numUVs;
  int numVerts;
};
