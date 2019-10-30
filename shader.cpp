#include "shader.h"
#include <string>
#include <fstream>
#include <sstream>
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


MyShader::MyShader(std::string vertName, std::string geoName, std::string fragName) : usingGeom(true){
    setShaders( vertName, geoName, fragName);

}

MyShader::MyShader(std::string vertName, std::string fragName) : usingGeom(false){
    setShaders( vertName, "", fragName);
}

std::string MyShader::readFile(std::string name){
    std::ifstream t(name.c_str());
    std::stringstream buf;
    buf << t.rdbuf();
    return buf.str();
}


void MyShader::setShaders(std::string vertName, std::string geoName, std::string fragName){
    //printf("Loading shaders v: %s, g: %s, f:%s.\n", vertName.c_str(), geoName.c_str(), fragName.c_str());
    //printf("initial loading geom is %d.\n", usingGeom);
    std::string vertString = readFile(vertName);
    std::string fragString = readFile(fragName);
    //std::cout <<vertString << std::endl;
    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *cs = vertString.c_str();
    glShaderSource(vertShader, 1, &cs, NULL);
    cs = fragString.c_str();
    glShaderSource(fragShader, 1, &cs, NULL);
    if (usingGeom){
      //printf("loading geometry shader.\n");
      std::string geomString = readFile(geoName);
      geomShader = glCreateShader(GL_GEOMETRY_SHADER);
      cs = geomString.c_str();
      glShaderSource(geomShader, 1, &cs, NULL);
    } 
    
    glCompileShader(vertShader);
    if (usingGeom){
      glCompileShader(geomShader);
    }
    glCompileShader(fragShader);
    program = glCreateProgram();

    glAttachShader(program, vertShader);
    if (usingGeom){
      glAttachShader(program, geomShader);
    }
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    //printf("linked loading geom is %d.\n", usingGeom);
    checkErrors();
}

void MyShader::setMatrix4(std::string argument, glm::mat4 mat)
{
  float* vals = glm::value_ptr(mat);
  /* std::cout << "In Set Matrix" << std::endl;
  printf("setting matrix in shader.\n");
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 4; j++){
      std::cout << vals[4*j+i];
    }
    std::cout << std::endl;
    }*/
  GLint loc = glGetUniformLocation(program, argument.c_str());
  glUniformMatrix4fv(loc, 1, GL_FALSE, vals);
}

void MyShader::setFloat(std::string argument, float val)
{
  GLint loc = glGetUniformLocation(program, argument.c_str());
  glUniform1f(loc, val);
}

void MyShader::setInt(std::string argument, int val)
{
  GLint loc = glGetUniformLocation(program, argument.c_str());
  glUniform1i(loc, val);
}


void MyShader::setVector4(std::string argument, glm::vec4 val)
{
  GLint loc = glGetUniformLocation(program, argument.c_str());
  glUniform4f(loc, val.x, val.y, val.z, val.w);
}

void MyShader::bindShader(){
    int err;
    if ((err = glGetError()) != 0){
        printf("Error %d at start of bind shader\n", err);
    }
    glUseProgram(program);
    if ((err = glGetError()) != 0){
        printf("Error %d at use program\n", err);
    }
  /*if(hasTexture){
      GLint screenLoc = glGetUniformLocation(program, textureName.c_str());
      if ((err = glGetError()) != 0){
          printf("Error %d at ahi\n", err);
      }
      glUniform1i(screenLoc, 0);
      if ((err = glGetError()) != 0){
          printf("Error %d at b\n", err);
      }
      //glEnable(textureTarget);
      if ((err = glGetError()) != 0){
          printf("Error %d at c\n", err);
      }
      glActiveTexture(GL_TEXTURE0);
      if ((err = glGetError()) != 0){
          printf("Error %d at d\n", err);
      }
    glBindTexture(textureTarget, textureID);
  }*/

  if (textures.size() > 0){
    for(auto it = textures.begin(); it != textures.end(); it++){
      Texture t = it->second;

      GLint screenLoc = glGetUniformLocation(program, t.name.c_str());
      if ((err = glGetError()) != 0){
          printf("Error %d at ahi\n", err);
      }
      glUniform1i(screenLoc, 0);
      if ((err = glGetError()) != 0){
          printf("Error %d at b\n", err);
      }
	  /*glEnable(t.target);
      if ((err = glGetError()) != 0){
          printf("Error %d at c\n", err);
      }*/
      glActiveTexture(GL_TEXTURE0);
      if ((err = glGetError()) != 0){
          printf("Error %d at d\n", err);
      }
      glBindTexture(t.target, t.ID);
    }
  }
    if ((err = glGetError()) != 0){
        printf("Error %d at end of bind shader\n", err);
    }

}

void MyShader::unbindShader(){
    glUseProgram(0);
} 


void MyShader::checkErrors(){
  //printf("Using geometry shader is %d.\n", usingGeom);
  //printf("check vert\n");
  checkError(vertShader);
  //printf("check frag\n");
  checkError(fragShader);
  if (usingGeom){
    //printf("check geom\n");
    checkError(geomShader);
  }
  checkProgramError();
}

void MyShader::checkError(GLint sid){
  GLint isCompiled = 0;
  glGetShaderiv(sid, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE){
    GLint maxLength = 0;
    glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &maxLength);
    printf("Expecting a %d character error.\n", maxLength);
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(sid, maxLength, &maxLength, &errorLog[0]);

    std::cout << "Error compiling shader " << sid << ": " << std::endl;
    if(maxLength > 0){
      printf("%s\n", &errorLog[0]);
      std::cout << &errorLog[0] << std::endl;
    }
    else{
      printf("Couldn't get error properly.\n"); 
    }
  }
}

void MyShader::checkProgramError(){
  GLint isLinked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if(isLinked == GL_FALSE){
    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> infoLog(maxLength);
    glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

    printf("Error linking shader program:\n%s\n", &infoLog[0]);

  }

}

void MyShader::loadTexture(std::string argument, std::string fileName){
  int w, h, comp;
  unsigned char* image = stbi_load(fileName.c_str(), &w, &h, &comp, 0);

  Texture t;

  t.target = GL_TEXTURE_2D;
  glGenTextures(1, &t.ID);
  glBindTexture(GL_TEXTURE_2D, t.ID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (comp == 3){
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
  }
  else if (comp == 4){
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(image);


  t.name = argument;
  hasTexture = true;
  textures[argument] =  t;
}


void MyShader::loadFloatTexture(std::string argument, float* texData){
  Texture t;

  t.target = GL_TEXTURE_2D;
  glGenTextures(1, &t.ID);
  glBindTexture(GL_TEXTURE_2D, t.ID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 512*512, 1, 0, GL_RED, GL_FLOAT, texData);


  glBindTexture(GL_TEXTURE_2D, 0);

  t.name = argument;
  hasTexture = true;
  textures[argument] =  t;
}
