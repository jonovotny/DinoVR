#version 330
layout(points) in;
layout(triangle_strip, max_vertices=4) out;
in vec2 vertexColor[];
in vec4 vertexOrig[];
in vec3 vertexDist[];
in float vertexCluster[];
in float vertexSimilarity[];
in vec3 splCoord[];
in vec3 vcameraSpherePos[];

out vec2 gsColor;
out float gsCluster;
out vec4 normal;
out vec4 position;
out float gsSimilarity;
out vec3 gsDist;
out vec3 gsSplitCoord;

out vec3 cameraSpherePos;
out float sphereRadius;
out vec2 mapping;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat4 p;

uniform float rad;
uniform float numClusters;
uniform float maxDistance = -1;

uniform vec4 cuttingPlane; //represented as ax + by + cz + d = 0, values >0 culled
uniform float cutOnOriginal = -1;

uniform int layers = 0;
uniform int direction = 0;
uniform int distMode = 0;
uniform float distFalloff = 0.3;

const float g_boxCorrection = 1.5;


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
  vec4 origPos = vertexOrig[0];
  vec3 splitPos = splCoord[0];
  vec3 dist = vertexDist[0];
  float layers_gap = 30;
  float around_gap = 60;
  float tgap = radians(around_gap);
  float pgap = radians(layers_gap);
  float pi = 3.1415926;
  gsColor = vertexColor[0];
  gsCluster = vertexCluster[0];
  gsSimilarity = vertexSimilarity[0];
  gsDist = dist;
  gsSplitCoord = splCoord[0];
  
  bool inLayer = false;
  if (direction == 0) {
	  if ((layers & int(1)) == 0 && splitPos.z > 0.045) {
		inLayer = true;
	  }
	  if ((layers & int(2)) == 0 && splitPos.z > 0.03 && splitPos.z < 0.045) {
		inLayer = true;
	  }
	  if ((layers & int(4))== 0 && splitPos.z > 0.015 && splitPos.z < 0.03) {
		inLayer = true;
	  }
	  if ((layers & int(8)) == 0 && splitPos.z < 0.015) {
		inLayer = true;
	  }
}

  if (direction == 1) {
	  if ((layers & int(1)) == 0 && splitPos.y > 0.02) {
		inLayer = true;
	  }
	  if ((layers & int(4)) == 0 && splitPos.y > -0.02 && splitPos.y <= 0.02) {
		inLayer = true;
	  }
	  if ((layers & int(8))== 0 && splitPos.y < -0.02) {
		inLayer = true;
	  }
}
  
  if (inLayer) {


	  float radius = rad;
	  //Make particles smaller if they are nto clustered
	  if (numClusters > 0 && gsCluster < -0.5){
	    radius *= 0.5;
	  }

	  //Make particles smaller if their path is not long enough to compute similarity
	  if (maxDistance > 0 && gsSimilarity < -0.5){
	    radius *= 0.3;
	  }
	  
	  if(distMode == 1 || distMode == 3) {
	     if (abs(dist.y) >= distFalloff) {
		   radius *= 1.0;
		 } else {
		   radius *= (0.1 + ((abs(dist.y)/distFalloff)*0.9));
		 }
		 
	  }
	  //Make particles smaller if they are outside of the cutting plane.
	  float planeLocation = (pos.x * cuttingPlane.x + pos.y * cuttingPlane.y + pos.z * cuttingPlane.z + cuttingPlane.w);
	  if (cutOnOriginal > 0){ // by convention set to -1 for false or 1 for true
	    planeLocation = (origPos.x * cuttingPlane.x + origPos.y * cuttingPlane.y + origPos.z * cuttingPlane.z + cuttingPlane.w);
	  }
	  if (abs(planeLocation) > 0.0001){
	    radius = radius * .3;	gl_PrimitiveID = gl_PrimitiveIDIn;
	  }



/*
	  for (float up = 0; up <= pi; up += pgap){
	    float phi = up;
	    for (float around = 0; around <= 2 * pi; around += tgap){
	      float theta = around;
	      vec4 offsetDir = get_vertex(theta, phi);modelToCameraMatrixUnif
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
*/
  vec4 P = mv * pos;
  sphereRadius = radius*25;

	vec4 cameraCornerPos;
	//Bottom-left
	mapping = vec2(-1.0, -1.0) * g_boxCorrection;
	cameraSpherePos = P.xyz;
	cameraCornerPos = P;
	cameraCornerPos.xy += vec2(-sphereRadius, -sphereRadius) * g_boxCorrection;
	gl_Position = p * cameraCornerPos;
	EmitVertex();
	
	//Top-left
	mapping = vec2(-1.0, 1.0) * g_boxCorrection;
	cameraSpherePos = P.xyz;
	cameraCornerPos = P;
	cameraCornerPos.xy += vec2(-sphereRadius, sphereRadius) * g_boxCorrection;
	gl_Position = p * cameraCornerPos;
	EmitVertex();
	
	//Bottom-right
	mapping = vec2(1.0, -1.0) * g_boxCorrection;
	cameraSpherePos = P.xyz;
	cameraCornerPos = P;
	cameraCornerPos.xy += vec2(sphereRadius, -sphereRadius) * g_boxCorrection;
	gl_Position = p * cameraCornerPos;
	EmitVertex();
	
	//Top-right
	mapping = vec2(1.0, 1.0) * g_boxCorrection;
	cameraSpherePos = P.xyz;
	cameraCornerPos = P;
	cameraCornerPos.xy += vec2(sphereRadius, sphereRadius) * g_boxCorrection;
	gl_Position = p * cameraCornerPos;
	EmitVertex();

	}

}
