#version 150
layout(points) in;
layout(triangle_strip, max_vertices=85) out;
in vec2 vertexColor[];
in float vertexCluster[];
in float vertexSimilarity[];

out vec2 gsColor;
out float gsCluster;
out vec4 normal;
out vec4 position;
out float gsSimilarity;

uniform mat4 mvp;

uniform float rad;
uniform float numClusters;
uniform float maxDistance = -1;
//float rad = 0.002;

vec4 get_vertex(float theta, float phi){
  return  vec4(sin(theta) * sin(phi), cos(phi), cos(theta) * sin(phi), 1.0);
}

vec4 transform(vec4 i){
  return mvp * i;
}

void main ()
{ 
  vec4 pos = gl_in[0].gl_Position;
  float layers_gap = 30;
  float around_gap = 60;
  float tgap = radians(around_gap);
  float pgap = radians(layers_gap);
  float pi = 3.1415926;
  gsColor = vertexColor[0];
  gsCluster = vertexCluster[0];
  gsSimilarity = vertexSimilarity[0];


  float radius = rad;
  if (numClusters > 0 && gsCluster < -0.5){
    radius *= 0.5;
  }
  if (maxDistance > 0 && gsSimilarity < -0.5){
    radius *= 0.3;
  }

  for (float up = 0; up <= pi; up += pgap){
    float phi = up;
    for (float around = 0; around <= 2 * pi; around += tgap){
      float theta = around;
      vec4 offsetDir = get_vertex(theta, phi);
      gl_Position = transform(pos + radius * offsetDir);
      position = gl_Position;
      normal = offsetDir;
      EmitVertex();
      offsetDir = get_vertex(theta, phi + pgap);
      gl_Position = transform(pos + radius * offsetDir);
      position = gl_Position;
      normal = offsetDir;
      EmitVertex();
    }
    EndPrimitive();

  }

}
