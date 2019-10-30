#pragma once

#if defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <string>
#include "glm/glm.hpp"
#include <unordered_map>

/**
 * Texture is a simple struct for storing the information necessary to use a texture with OpenGL.
 */
struct Texture{
public:
  Texture(std::string texName, GLuint texTarget, GLuint texID) {
    name = texName;
    target = texTarget;
    ID = texID;
  }
  Texture() {};
  std::string name;
  GLuint target;
  GLuint ID;
};

/**
 * MyShader is a simple abstraction of an OpenGL shader. 
 * It is not complete and is being added to as I need more features. 
 */
class MyShader{
public:
  MyShader(){};
  /**
   * Constructor to create a shader program with a geometry shader.
   * @param vertName File path to the vertex shader.
   * @param geomName File path to the geometry shader.
   * @param fragName File path to the fragment shader.
   */
  MyShader(std::string vertName, std::string geomName, std::string fragName);
  /**
   * Constructor to create a shader program without a geometry shader.
   * @param vertName File path to the vertex shader.
   * @param fragName File path to the fragment shader.
   */
  MyShader(std::string vertName, std::string fragName);

  /**
   * Copy constructor that I made because c++ was giving me errors and I'm 90% sure is unnecessary now.
   */
  MyShader & operator=(const MyShader & other)
  {
    vertShader = other.vertShader;
    geomShader = other.geomShader;
    fragShader = other.fragShader;
    program    = other.program   ;
    return *this;
  }

  /**
   * Set a uniform argument for a matrix
   * @param argument The name of the variable in the shader.
   * @param mat The matrix value to be set.
   */
  void setMatrix4(std::string argument, glm::mat4 mat);
   /**
   * Set a uniform argument for a float
   * @param argument The name of the variable in the shader.
   * @param val The float value to be set.
   */
  void setFloat(std::string argument, float val);
  void setInt(std::string argument, int val);
  
  /**
   * Loads a texture into OpenGL for this shader.
   * @param argument The name of the variable for which to set the texture.
   * @param fileName The name of the file to load the texture from.
   */
  void setVector4(std::string argument, glm::vec4 val);

  void loadTexture(std::string argument, std::string fileName);
  void loadFloatTexture(std::string argument, float* data);

  /**
   * Binds the shader to be the active program. 
   * Should be called before each draw call using this shader.
   */
  void bindShader();
  /**
   * Unbinds the shader.
   * Should be called after you are done drawing with this shader.
   */
  void unbindShader();

  /**
   * Checks to see if there have been any errors loading or linking this shader.
   * @return Should probably exist. Currently just prints output.
   */
  void checkErrors();
  ///Why are you public?
  void checkError(GLint shader_id);
  ///Same question to you.
  void checkProgramError();
  // You're public because debugging was going horribly wrong.
  bool usingGeom;

private:


  std::string readFile(std::string name);
  
  void setShaders(std::string, std::string, std::string);
  

  GLuint textureTarget;
  GLuint textureID;
  std::string textureName;
  std::unordered_map<std::string, Texture> textures;
  GLuint vertShader;
  GLuint geomShader;
  GLuint fragShader;

  GLuint program;

  bool hasTexture = false;
};

