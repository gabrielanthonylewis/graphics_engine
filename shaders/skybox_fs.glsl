#version 330


uniform samplerCube sampler_skybox;

in vec3 textureCoords;

layout(location = 0) out vec3 reflected_light;



void main(void)
{
	vec3 abc = textureCoords;
	abc.y = -textureCoords.y;
	vec3 colour = texture(sampler_skybox, abc).rgb;

	reflected_light = colour;// colour;// vec3(1, 1, 1);
}

