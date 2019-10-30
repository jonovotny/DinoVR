#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 color;
layout(location=2) in vec3 textureCoord;
layout(location=3) in vec3 splitCoord;

out vec2 vertexColor;
out vec3 vertPos;
out vec3 texCoord;
out vec3 splCoord;

uniform mat4 mvp;

void main(void){
  gl_Position = vec4(vertexPos, 1.0);
  vertPos = vertexPos;
  vertexColor = color;
  texCoord = textureCoord;
  splCoord = splitCoord;
}

