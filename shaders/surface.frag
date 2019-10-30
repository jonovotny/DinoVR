#version 330

out vec4 color;
in vec2 gsColor;
in vec3 gsTexCoord;
in vec3 gsSplitCoord;
uniform sampler2D colorMap;
uniform sampler2D maskMap;
uniform float cellSize = 500;
uniform float cellLine = 0.0001;
uniform int surfaceMode;
uniform int trailNum;
uniform int layers;
uniform int direction = 0;
uniform int thickinterval = 0;

in vec3 normal;
in float gsStretch;

void main()
{
  
  vec4 baseColor = texture(colorMap, gsColor);
  if (direction == 1 && surfaceMode != 4) {
    baseColor = texture(colorMap, gsColor.yx);
  }
  vec4 mask = texture(maskMap, gsTexCoord.xy*50);

  vec3 lightDir = normalize(vec3(0,0,1));
if (direction == 1) {
    lightDir = normalize(vec3(0,1,0));
  }  


  float lambertian = abs(dot(lightDir, normal.xyz));
  float ambient = 0.4;
  float diffuse = 1. - ambient;
  color = baseColor * (ambient + diffuse * lambertian) + vec4(-0.01 * trailNum);
  
  
  bool inLayer = false;
  if (direction == 0) {
	  if (mod(layers, 2) == 0 && gsSplitCoord.z > 0.045) {
		inLayer = true;
	  }
	  if (mod(layers/2, 2) == 0 && gsSplitCoord.z > 0.03 && gsSplitCoord.z < 0.045) {
		inLayer = true;
	  }
	  if (mod(layers/4, 2) == 0 && gsSplitCoord.z > 0.015 && gsSplitCoord.z < 0.03) {
		inLayer = true;
	  }
	  if (mod(layers/8, 2) == 0 && gsSplitCoord.z < 0.015) {
		inLayer = true;
	  }
}

  if (direction == 1) {
	  if (mod(layers, 2) == 0 && gsSplitCoord.y > 0.02) {
		inLayer = true;
	  }
	  if (mod(layers/2, 2) == 0 && gsSplitCoord.y > -0.02 && gsSplitCoord.y <= 0.02) {
		inLayer = true;
	  }
	  if (mod(layers/4, 2) == 0 && gsSplitCoord.y < -0.02) {
		inLayer = true;
	  }
}
  
  if (!inLayer) {
	discard;
  }


  vec3 gridCheck = mod(gsTexCoord*cellSize,1.0);
  vec3 gridCheck2 = mod(floor(gsTexCoord*cellSize),thickinterval);
  float thickness=abs(cellSize*cellLine);
  float gridA = gridCheck.x;
  float gridlineA = gridCheck2.x;
  float gridB = gridCheck.y;
  float gridlineB = gridCheck2.y;
  if (direction == 1) {
    gridB = gridCheck.z;
	gridlineB = gridCheck2.z;
  }


  if (surfaceMode == 0) {
      if ((gridA < thickness && gridA > -thickness) || (gridB < thickness && gridB > -thickness)) {
        color = vec4(color.rgb, 1.0);
      } else {
        color = vec4(0.0);
        discard;
      }
    }
	
	
  if (surfaceMode == 1) {
    if (direction == 0) {
		  if ((gridA < thickness && gridA > -thickness) || (gridB < thickness && gridB > -thickness)) {
			color = vec4(0.0);
			discard;
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}
	if (direction == 1) {
		  if (gridB < thickness && gridB > -thickness) {
			color = vec4(0.0);
			discard;
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}
	}
	
	
  if (surfaceMode == 2) {
      color = vec4(color.rgb, 1.0);
  }
  if (surfaceMode == 3) {
  
    if (gridlineB == 0) {
		    thickness = thickness*2;
		  }
    if (direction == 0) {
		  if ((gridA < thickness && gridA > -thickness) || (gridB < thickness && gridB > -thickness)) {
		    if (gridlineA == 0 || gridlineB == 0) {
				color = vec4(1.0);
			} else {
				color = vec4(0.0);
				discard;
			}
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}

	if (direction == 1) {
		  if (gridB < thickness && gridB > -thickness) {
			if (gridlineB == 0) {
				color = vec4(1.0);
			} else {
				color = vec4(0.0);
				discard;
			}
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}
  }
	
  if (surfaceMode == 4) {
  
    if (gridlineB == 0) {
		    thickness = thickness*2;
		  }
    if (direction == 0) {
		  if ((gridA < thickness && gridA > -thickness) || (gridB < thickness && gridB > -thickness)) {
		    if (gridlineA == 0 || gridlineB == 0) {
				color = vec4(1.0);
			} else {
				color = vec4(0.0);
				discard;
			}
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}

	if (direction == 1) {
		  if (gridB < thickness && gridB > -thickness) {
			if (gridlineB == 0) {
				color = vec4(1.0);
			} else {
				color = vec4(0.0);
				discard;
			}
		  } else {
			color = vec4(color.rgb, 1.0);
		  }
		}
  }
}
