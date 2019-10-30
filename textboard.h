#include "GLUtil.h"
#include "glm/glm.hpp"
#include "pointmanager.h"

/**
 * Representation of a 2-d image in the program.
 */
class Textboard{
public:
  Textboard()  {};
  Textboard(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up);

  void Initialize(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up);

  void Draw(glm::mat4 mvp);
  void FillTheLine(glm::vec3 bLeft, int b, bool update);
  void LoadVertex();
  void LoadText();
  void UpdateTexCoord();
  void LoadID(int num);
  void LoadNumber(int num);

private:
  void ClearTexCoord();
  std::vector<glm::vec3> verts;
  std::vector<int> idDigits;
  std::vector<int> pDigits;
  std::vector<glm::vec2> uvs;
  bool initialized = false;
  MyShader* shader;

  int numDigits = 8;
  int id;
  int number = 0;

  glm::vec3 bl;
  glm::vec3 right;
  glm::vec3 up;
  glm::vec3 first;

  GLuint vertBuffer;
  GLuint uvBuffer;

  int numUVs;
  int numVerts;
};
