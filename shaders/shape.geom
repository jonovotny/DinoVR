#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

out vec3 normal;

uniform mat4 mvp;

void main ()
{
  vec4 a = gl_in[0].gl_Position;
  vec4 b = gl_in[1].gl_Position;
  vec4 c = gl_in[2].gl_Position;

  normal  = normalize(cross(a.xyz - b.xyz, c.xyz - b.xyz));//to within a sign

  gl_Position = mvp * a;
  EmitVertex();

  gl_Position = mvp * b;
  EmitVertex();

  gl_Position = mvp * c;
  EmitVertex();
}
