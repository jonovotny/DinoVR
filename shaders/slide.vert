#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 uv;

out vec2 vertexUV;

uniform mat4 mvp;

void main(void){
  gl_Position = mvp * vec4(vertexPos, 1.0);
  vertexUV = uv;
}

