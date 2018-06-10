#version 330

layout (std140) uniform PerFrameUniforms
{
	mat4 projection_view_xform;
};
layout (std140) uniform PerModelUniforms
{
	mat4 model_xform;
};

// Uniforms
uniform vec3 ambient_colour;
uniform vec3 specular_colour;

// Varyings
in vec3 varying_normal;
in vec2 varying_texcoord;
in vec3 varying_P;

// Outs
layout(location = 0) out vec3 position_out;
layout(location = 1) out vec3 normal_out;
layout(location = 2) out vec3 colour_out;
layout(location = 3) out vec3 specular_out;


void main(void)
{
	position_out = varying_P;
	normal_out = normalize(varying_normal);
	colour_out = ambient_colour;
	specular_out = specular_colour;
}

