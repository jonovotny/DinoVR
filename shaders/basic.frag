#version 330
#extension GL_EXT_gpu_shader4 : enable

in vec2 gsColor;
in float gsCluster;
in float gsSimilarity;
in vec4 normal;
in vec4 position;
in vec3 gsDist;
uniform sampler2D colorMap;
uniform sampler2D clusterMap;
uniform float numClusters = 0;
uniform float maxDistance = -1;
uniform float clusterDistance = -1;
uniform int direction = 0;
uniform int freezeStep = 0;
uniform int distMode = 0;
uniform float distFalloff = 0.3;
uniform mat4 p;
uniform mat4 mv;

in vec3 cameraSpherePos;
in float sphereRadius;		
in vec2 mapping;

float gamma = 2.2;

vec4 togamma(vec4 a){
  return pow(a, vec4(gamma));
}

vec4 fromgamma(vec4 a){
  return pow(a, vec4(1./gamma));
}

out vec4 outputColor;


void Impostor(out vec3 cameraPos, out vec3 cameraNormal)
{
	vec3 cameraPlanePos = vec3(mapping * sphereRadius, 0.0) + cameraSpherePos;
	vec3 rayDirection = normalize(cameraPlanePos);
	
	float B = 2.0 * dot(rayDirection, -cameraSpherePos);
	float C = dot(cameraSpherePos, cameraSpherePos) -
		(sphereRadius * sphereRadius);
	
	float det = (B * B) - (4 * C);
	if(det < 0.0) 
		discard;
		
	float sqrtDet = sqrt(det);
	float posT = (-B + sqrtDet)/2;
	float negT = (-B - sqrtDet)/2;
	
	float intersectT = min(posT, negT);
	cameraPos = rayDirection * intersectT;
	cameraNormal = normalize(cameraPos - cameraSpherePos);
}

void main()
{

	vec3 cameraPos;
	vec3 cameraNormal;
	
	Impostor(cameraPos, cameraNormal);
	
	//Set the depth based on the new cameraPos.
	vec4 clipPos = p * vec4(cameraPos, 1.0);
	float ndcDepth = clipPos.z / clipPos.w;
	gl_FragDepth = ((gl_DepthRange.diff * ndcDepth) + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
	
  vec4 lightDir = normalize(mv * vec4(0, 0.0, 1.0, 1.0));
  float lambertian = 0.80 * max(dot(lightDir.xyz, cameraNormal), 0.0) + 0.20 * max(dot(vec3(0.0, 0.0, 1.0), cameraNormal), 0.0);
  float ambient = 0.3; 
  float diffuse = 1. - ambient;
  vec4 baseColor;// = texture(colorMap, gsColor);
  vec3 dist = gsDist;
  

  if (maxDistance > 0){
    vec4 minColor = togamma(vec4( 0 , .9, 0 , 1));
    vec4 medColor = togamma(vec4( 1 , .2, .2, 1));
    vec4 maxColor = togamma(vec4( .4, .4,  1, 1));
    float midpoint = 0.3;
    if (clusterDistance > 0){
      midpoint = (clusterDistance / maxDistance);
    }


    float ratio = gsSimilarity / maxDistance;
    //probably do other things to the ratio
    /*ratio = 1 - ratio;
    ratio *= ratio; //bring closer to 1?
    ratio = 1 - ratio;*/
    if (ratio < 0){
      baseColor = vec4(.2, .2, .2, 1);
    }
    else if (ratio < midpoint){
      ratio /= midpoint; // restore scaling to [0,1]
      baseColor = minColor * (1 - ratio) + medColor * ratio;
    }
    else{
      ratio = (ratio - midpoint) / (1 - midpoint);

      ratio = min(1., ratio);
    if (direction == 0) {
    baseColor = texture(clusterMap, gsColor);// vec4(1, 0,0,1); 
    }
    if (direction == 1) {
    baseColor = texture(clusterMap, gsColor.yx);
    }
      baseColor = medColor * (1 - ratio) + maxColor * ratio;
    }
    baseColor = fromgamma(baseColor);
  }
  else if (numClusters > 0){
    
    baseColor = texture(clusterMap, vec2(gsCluster / numClusters, 0.5));
    if (gsCluster < -0.5){
      baseColor = vec4(0.3, 0.3, 0.4, 1.0);
    }
  }
  else{
    if (direction == 0) {
      baseColor = texture(clusterMap, gsColor);// vec4(1, 0,0,1); 
    }
    if (direction == 1) {
      baseColor = texture(clusterMap, gsColor.yx);
    }
  }

  outputColor = baseColor * (ambient + diffuse * lambertian);	
        
}
