#version 330

out vec4 color;
in vec2 gsTexCoord;
in vec3 normal;
uniform float cellSize = 25;
uniform float cellLine = 0.005;

uniform int sweepMode;
uniform int frame;

void main()
{
  vec4 baseColor = vec4(1.0);

  vec3 lightDir = normalize(vec3(0,0,1));

  float lambertian = abs(dot(lightDir, normal.xyz));
  float ambient = 0.4;
  float diffuse = 1. - ambient;
  color = baseColor * (ambient + diffuse * lambertian);

  vec2 gridCheck = mod(gsTexCoord*cellSize,1.0);
  float thickness=abs(cellSize*cellLine);
  if (sweepMode == 0){
	  if ((gridCheck.x < thickness && gridCheck.x > -thickness) || (gridCheck.y < thickness && gridCheck.y > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
  }
  if (sweepMode == 1) {
	if ((gridCheck.x < thickness && gridCheck.x > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
  }

  if (sweepMode == 2) {
	if ((gridCheck.x < thickness && gridCheck.x > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
  }

  if (sweepMode == 3) {
	if ((gridCheck.y < thickness && gridCheck.y > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
if (gsTexCoord.y > frame*0.025) {
    discard;
  }
  }

  if (sweepMode == 4) {
	if ((gridCheck.x < thickness && gridCheck.x > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
if (gsTexCoord.y > frame*0.025) {
    discard;
  }
  }

  if (sweepMode == 5) {
        float y = abs(gsTexCoord.y-frame*0.025)*cellSize;
        if (y != 0) {y = mod(sqrt(y)*10.0, 1.0);}
	if ((y < thickness && y > -thickness)) {
		color = vec4(color.rgb, 1.0);
	  } else {
		color = vec4(0.0);
		discard;
	  }
if (gsTexCoord.y > frame*0.025) {
    discard;
  }
  }

  //color = abs(vec4(gsTexCoord*50,1.0));
  //color = abs(vec4(normal, 1.0));
}
