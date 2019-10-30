#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 textureCoord;

out vec3 vertPos;
out vec2 texCoord;

uniform mat4 mvp;

void main(void){
  gl_Position = vec4(vertexPos, 1.0);
  vertPos = vertexPos;
  texCoord = textureCoord;
}

