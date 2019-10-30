#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
in vec2 vertexColor[];
in vec3 vertexNormal[];

out vec2 gsColor;
out vec3 normal;


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

//  normal  = normalize(cross(a.xyz - b.xyz, c.xyz - b.xyz));//to within a sign
  normal = vertexNormal[0];

  if (dmax > 0.01){
    //return;
  }

  gsColor = vertexColor[0];
  gl_Position = mvp * a;
  EmitVertex();

  gsColor = vertexColor[1];
  gl_Position = mvp * b;
  EmitVertex();

  gsColor = vertexColor[2];
  gl_Position = mvp * c;
  EmitVertex();
}
