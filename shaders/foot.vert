#version 330

layout(location=0) in vec3 vertexPos;

out vec3 vertPos;

uniform mat4 mvp;
uniform mat4 transform;

void main(void){
  gl_Position = vec4(vertexPos, 1.0);
}

