#include "textboard.h"

Textboard::Textboard(std::string texture, glm::vec3 bl, glm::vec3 right, glm::vec3 up){
  Initialize(texture, bl, right, up);
}


void Textboard::Initialize(std::string texture, glm::vec3 b, glm::vec3 r, glm::vec3 u){
  initialized = true; 
  bl = b;
  right = r;
  up = u;
  first = b + r + r + r;
  //Initialize the shader
  shader = new MyShader("shaders/slide.vert", "shaders/slide.frag");
  shader->checkErrors();
  shader->loadTexture("slideTex", texture);

  //Initialize the buffers
  glGenBuffers(1, &vertBuffer);
  glGenBuffers(1, &uvBuffer);

  verts.push_back(bl);
  uvs.push_back(glm::vec2(0.0, 1.0));
  verts.push_back(bl + right + right + right + up);
  uvs.push_back(glm::vec2(0.3, 0.5));
  verts.push_back(bl + up);
  uvs.push_back(glm::vec2(0.0, 0.5));

  verts.push_back(bl);
  uvs.push_back(glm::vec2(0.0, 1.0));
  verts.push_back(bl + right + right + right );
  uvs.push_back(glm::vec2(0.3, 1.0));
  verts.push_back(bl + right + right +right + up);
  uvs.push_back(glm::vec2(0.3, 0.5));

  FillTheLine(first, numDigits, false);
  FillTheLine(bl-up, numDigits+3, false);

  glm::vec3 bLeft2 = bl -up;
  LoadVertex();
  LoadID(50);
  LoadNumber(60);
//  UpdateTexCoord();
  }

void Textboard::LoadVertex(){
  //Put vertex data in buffers
  glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
  numVerts = verts.size();
  glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
  numUVs = uvs.size();
  glBufferData(GL_ARRAY_BUFFER, numUVs * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Textboard::FillTheLine(glm::vec3 bLeft, int b, bool update){
    for (int i =0; i<b; i++){
        if (!update) {
            verts.push_back(bLeft);
            verts.push_back(bLeft + right + up);
            verts.push_back(bLeft + up);
            verts.push_back(bLeft);
            verts.push_back(bLeft + right);
            verts.push_back(bLeft + right + up);
            bLeft = bLeft +right;
        }
        uvs.push_back(glm::vec2(0.3, 1.0));
        uvs.push_back(glm::vec2(0.4, 0.5));
        uvs.push_back(glm::vec2(0.3, 0.5));

        uvs.push_back(glm::vec2(0.3, 1.0));
        uvs.push_back(glm::vec2(0.4, 1.0));
        uvs.push_back(glm::vec2(0.4, 0.5));
    }
}

void Textboard::UpdateTexCoord(){
    ClearTexCoord();
    FillTheLine(first, numDigits-idDigits.size(), true); //fill the preceding blanks
    for (int i=idDigits.size()-1; i >= 0;i--){
        int off = idDigits.at(i);
        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.0));
        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.0));

        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.0));
    }
    FillTheLine(bl-up, numDigits+3-pDigits.size(), true); //fill the preceding blanks
    for (int i=pDigits.size()-1; i >= 0;i--){
        int off = pDigits.at(i);
        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.0));
        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.0));

        uvs.push_back(glm::vec2(0.0 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.5));
        uvs.push_back(glm::vec2(0.1 + off*0.1, 0.0));
    }
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numUVs * sizeof(glm::vec2), uvs.data());
}

void Textboard::ClearTexCoord(){
    uvs.clear();
    uvs.push_back(glm::vec2(0.0, 1.0));
    uvs.push_back(glm::vec2(0.3, 0.5));
    uvs.push_back(glm::vec2(0.0, 0.5));
    uvs.push_back(glm::vec2(0.0, 1.0));
    uvs.push_back(glm::vec2(0.3, 1.0));
    uvs.push_back(glm::vec2(0.3, 0.5));
}

void Textboard::LoadID(int num) {
    id = num;
    idDigits.clear();
    while(num != 0 ){
        idDigits.push_back(num%10);
        num = num/10;
    }
    UpdateTexCoord();
}

void Textboard::LoadNumber(int num) {
    pDigits.clear();
    while(num != 0 ){
        pDigits.push_back(num%10);
        num = num/10;
    }
    UpdateTexCoord();
}

void Textboard::Draw(glm::mat4 mvp){
  LoadID(id);
//  UpdateTexCoord();
  id +=1;
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

  glDrawArrays(GL_TRIANGLES, 0, verts.size() *3);
  glCheckError();
  shader->unbindShader();
  glCheckError();
}
