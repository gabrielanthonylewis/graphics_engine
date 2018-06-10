#version 330

in vec3 vertex_position; // modelspace

uniform mat4 modelViewProjection;
uniform mat4 model_xform;

out vec3 worldPos;

void main(void)
{
	
	worldPos = (model_xform * vec4(vertex_position, 1.0)).xyz;
	gl_Position = modelViewProjection * vec4(vertex_position, 1.0);
}
