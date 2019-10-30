#version 330

in vec3 Color;
 
out vec4 FragColor;
 
void main()
{
	// assign vertex color 
    FragColor = vec4(Color, 1.0);
}
