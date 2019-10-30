#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
in vec2 vertexColor[];
in vec3 vertPos[];
in vec3 texCoord[];
in vec3 splCoord[];


out vec2 gsColor;
out vec3 normal;
out vec3 gsTexCoord;
out vec3 gsSplitCoord;
out float gsStretch;

uniform mat4 mvp;
uniform float ripThreshold = 4;
//uniform sampler2D stretchTex;

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

  normal  = normalize(cross(a.xyz - b.xyz, c.xyz - b.xyz));//to within a sign

  if (dmax > 0.01){
    //return;
  }
  
  //float primID = float(gl_PrimitiveID);
  //float stretch = texture(stretchTex, vec2(primID/262144.f, 0.5f)).r;
  //float stretch = triArea;
  float stretch = length(cross(a.xyz-b.xyz, a.xyz-c.xyz))/length(ripThreshold * cross(texCoord[0]-texCoord[1], texCoord[0]-texCoord[2]));
  stretch = max(abs(length(a.xyz-b.xyz)/length(texCoord[0]-texCoord[1])), abs(length(a.xyz-c.xyz)/length(texCoord[0]-texCoord[2])));
  stretch = max(stretch, abs(length(b.xyz-c.xyz)/length(texCoord[1]-texCoord[2])));
  
  float rip = max(0, ripThreshold);
  if ( stretch <= rip) {
	  gsStretch = stretch;
	  
	  gsColor = vertexColor[0];
	  gsTexCoord = texCoord[0];
	  gsSplitCoord = splCoord[0];
	  gl_Position = mvp * a;
	  EmitVertex();

	  gsColor = vertexColor[1];
	  gsTexCoord = texCoord[1];
	  gsSplitCoord = splCoord[1];
	  gl_Position = mvp * b;
	  EmitVertex();

	  gsColor = vertexColor[2];
	  gsTexCoord = texCoord[2];
	  gsSplitCoord = splCoord[2];
	  gl_Position = mvp * c;
	  EmitVertex();
  }
}
