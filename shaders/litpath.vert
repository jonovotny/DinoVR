#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 color;
layout(location=2) in vec3 normal;

out vec2 vertexColor;
out vec3 vertexNormal;
uniform mat4 mvp;
void main(void){
  gl_Position = mvp *  vec4(vertexPos, 1.0);
  vertexColor = color;
  vertexNormal = normal;
}
