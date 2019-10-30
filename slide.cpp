#include "slide.h"

Slide::Slide(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up){
  Initialize(texture, bl, right, up);
}


void Slide::Initialize(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up){
  initialized = true; 

  //Initialize the shader
  shader = new MyShader("shaders/slide.vert", "shaders/slide.frag");
  shader->checkErrors();
  shader->loadTexture("slideTex", texture);

  //Initialize the buffers
  glGenBuffers(1, &vertBuffer);
  glGenBuffers(1, &uvBuffer);

  //Generate Vertices TODO

  verts.push_back(bl);
  uvs.push_back(glm::vec2(0, 0));
  verts.push_back(bl + right);
  uvs.push_back(glm::vec2(1, 0));
  verts.push_back(bl + right + up);
  uvs.push_back(glm::vec2(1, 1));
  
  verts.push_back(bl);
  uvs.push_back(glm::vec2(0, 0));
  verts.push_back(bl + right + up);
  uvs.push_back(glm::vec2(1, 1));
  verts.push_back(bl + up);
  uvs.push_back(glm::vec2(0, 1));


  //Put vertex data in buffers
  glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
  numVerts = verts.size();
  glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  numUVs = uvs.size();
  glBufferData(GL_ARRAY_BUFFER, numUVs * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);


}

void Slide::Draw(glm::mat4 mvp){
  glCheckError();
  shader->bindShader();
  shader->setMatrix4("mvp", mvp);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  
  glCheckError();
  glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), NULL);
  
  glCheckError();
  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), NULL);
  glCheckError();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glCheckError();


  glDrawArrays(GL_TRIANGLES, 0, 6);
  glCheckError();
  shader->unbindShader();
  glCheckError();
}
