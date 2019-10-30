#version 330

out vec4 color;
in vec2 vertexUV;
uniform sampler2D slideTex;

void main()
{
  color = texture(slideTex, vertexUV);
  /*color.g = 0.3;

  color.rb = vertexUV;
  if (vertexUV.x < vertexUV.y){
    color.rb = vertexUV;
  }
  else{
    color = vec4(1,0,0,1);
  }*/
}
