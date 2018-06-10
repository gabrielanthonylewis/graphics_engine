#version 330

in vec3 vertex_position; // modelspace

uniform mat4 depthMVP;

void main(void)
{
	gl_Position = depthMVP * vec4(vertex_position, 1.0);
}
