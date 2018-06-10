#version 330


in vec3 vertex_position;

out vec3 textureCoords;



uniform	mat4 projection_xform;
uniform mat4 view_xform;



void main(void)
{
	textureCoords = vertex_position;
	gl_Position = projection_xform * view_xform * vec4(vertex_position.xyz, 1.0);
}