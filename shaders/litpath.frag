#version 330

out vec4 color;
in vec2 vertexColor;
in vec3 vertexNormal;
uniform sampler2D pathMap;
uniform float clusterId = -1;
uniform float minCutoff = 0.0;
uniform float maxCutoff = 1.0;
uniform float time = -1;

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
  if ( vertexColor.x < minCutoff || vertexColor.x > maxCutoff){
    color = vec4(0.0);
    return;
  }
  color = texture(pathMap, vec2(vertexColor.x, 0.5));


  if (maxDistance > 0){
    vec4 minColor = togamma(vec4( 0 , .9, 0 , 1));
    vec4 medColor = togamma(vec4( 1 , .2, .2, 1));
    vec4 maxColor = togamma(vec4( .4 , .4 , 1, 1));

    //medColor = togamma(vec4( .4 , .4 , 1, 1));

    float midpoint = 0.3;
    if (clusterDistance > 0){
      midpoint = (clusterDistance / maxDistance);
    }
    float ratio = vertexColor.y / maxDistance;
    //probably do other things to the ratio
    /*ratio = 1 - ratio;
    ratio *= ratio; //bring closer to 1?
    ratio = 1 - ratio;*/
    if (ratio < 0){
      color = vec4(.2, .2, .2, 1);
    }

    else if (ratio == 0.0){
      color = vec4(1.);
    }

    else if (ratio < midpoint){
      ratio /= midpoint; // restore scaling to [0,1]
      color = minColor * (1 - ratio) + medColor * ratio;
    }
    else{
      ratio = (ratio - midpoint) / (1 - midpoint);
      color = medColor * (1 - ratio) + maxColor * ratio;
    }
    color = fromgamma(color);
  }
  vec3 lightDir = normalize(vec3(0, 0.0, 1));
  vec3 n = normalize(vertexNormal);
  float lambertian = max(dot(lightDir, n), 0.0);

  float ambient = 0.3;
  float diffuse = 1. - ambient;
  

  color = color * (ambient + diffuse * lambertian);

}
