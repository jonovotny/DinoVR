#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 color;
layout(location=2) in vec3 origPos;
layout(location=3) in float cluster;
layout(location=4) in float similarity;
layout(location=5) in vec3 vertexDistance;
layout(location=6) in vec3 splitCoord;

out vec2 vertexColor;
out vec4 vertexOrig;
out vec3 vertexDist;
out float vertexCluster;
out float vertexSimilarity;
out vec3 splCoord;
out vec3 vcameraSpherePos;
uniform mat4 mvp;
void main(void){
//  pos =g3d_WorldToCameraMatrix *  g3d_ObjectToWorldMatrix * vertexPos;
  gl_Position =  vec4(vertexPos, 1.0);
  vcameraSpherePos = vertexPos;
  vertexOrig = vec4(origPos, 1.0);
  vertexDist = vertexDistance;
  vertexColor = color;
  vertexCluster = cluster;
  vertexSimilarity = similarity;
  splCoord = splitCoord;
}
