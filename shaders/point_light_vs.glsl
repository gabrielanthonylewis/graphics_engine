#version 330

uniform mat4 camera_projection_view_xform;
uniform mat4 light_model_xform;

in vec3 vertex_position;

void main(void)
{
	gl_Position = (camera_projection_view_xform * light_model_xform)  * vec4(vertex_position, 1.0);
}

