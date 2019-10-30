#version 330

out vec4 color;
in vec2 vertexColor;
uniform sampler2D pathMap;
uniform float clusterId = -1;
uniform float minCutoff = 0.0;
uniform float maxCutoff = 1.0;

void main()
{
  if ( vertexColor.x < minCutoff || vertexColor.x > maxCutoff){
    color = vec4(0.0);
    return;
  }
  color = texture(pathMap, vertexColor);
  /*if(clusterId > -.5){
    color = texture(pathMap, vec2(clusterId));
  }*/
}
