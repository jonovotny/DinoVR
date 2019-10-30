#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
in vec3 vertPos[];
in vec2 texCoord[];

out vec3 normal;
out vec2 gsTexCoord;

uniform mat4 mvp;

void main ()
{
  vec4 a = gl_in[0].gl_Position;
  vec4 b = gl_in[1].gl_Position;
  vec4 c = gl_in[2].gl_Position;

  float da = distance(a.xyz,b.xyz);
  float db = distance(b.xyz,c.xyz);
  float dc = distance(c.xyz,a.xyz);

  float dmax = max(da, max(db, dc));
  float dmin = min(da, min(db, dc));
  float ratio = dmax / dmin;

  normal = normalize(cross(a.xyz - b.xyz, c.xyz - b.xyz));//to within a sign


  gsTexCoord = texCoord[0];
  gl_Position = mvp * a;
  EmitVertex();

  gsTexCoord = texCoord[1];
  gl_Position = mvp * b;
  EmitVertex();

  gsTexCoord = texCoord[2];
  gl_Position = mvp * c;
  EmitVertex();
}
