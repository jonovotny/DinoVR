#version 330

out vec4 color;
in vec2 gsColor;
in float gsCluster;
in float gsSimilarity;
in vec4 normal;
in vec4 position;
uniform sampler2D colorMap;
uniform sampler2D clusterMap;
uniform float numClusters = 0;
uniform float maxDistance = -1;
uniform float clusterDistance = -1;

float gamma = 2.2;

vec4 togamma(vec4 a){
  return pow(a, vec4(gamma));
}

vec4 fromgamma(vec4 a){
  return pow(a, vec4(1./gamma));
}

void main()
{
  vec3 lightDir = normalize(vec3(0, 0.0, 1));
  float lambertian = max(dot(lightDir, normal.xyz), 0.0);
  float ambient = 0.3; 
  float diffuse = 1. - ambient;
  vec4 baseColor;// = texture(colorMap, gsColor);

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
    baseColor = texture(clusterMap, gsColor);// vec4(1, 0,0,1);
  }
  color = baseColor * (ambient + diffuse * lambertian);
}
