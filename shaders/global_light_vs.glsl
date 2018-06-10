#version 330

layout (std140) uniform PerFrameUniforms
{
	mat4 projection_view_xform;
};
layout (std140) uniform PerModelUniforms
{
	mat4 model_xform;
};


in vec3 vertex_position; // 2D or 3D?
in vec3 vertex_normal;
in vec2 vertex_texcoord;

out vec3 varying_normal;
out vec2 varying_texcoord;
out vec3 varying_P; // P is surface position, named as P due to the known formulas.


void main(void)
{
	// Normal, translated into world-space and in the [0,1] range
	varying_normal = normalize(vec3(model_xform * vec4(vertex_normal, 0.0)).xyz);

	// Texture coordinate
	varying_texcoord = vertex_texcoord;

	// Position on the the surface (in world)
	varying_P = mat4x3(model_xform) * vec4(vertex_position, 1.0);

    gl_Position = projection_view_xform * model_xform * vec4(vertex_position, 1.0); // if 2d: vertex_position, 0.0, 1.0
}
