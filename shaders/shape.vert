#version 330

layout (location = 0) in vec3 VertexPosition;
 
out vec3 Color;

uniform mat4 mvp;
uniform vec3 mode;
 
void main()
{
    gl_Position = mvp * vec4(VertexPosition, 1.0);
    Color = vec3(0.8, 0.8, 0.8);
}

