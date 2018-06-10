#version 330

uniform sampler2DRect sampler_world_colour;

layout(location = 0) out vec3 reflected_light;


// Function prototypes
vec3 GammaCorrection(vec3);

void main(void)
{
	vec3 result = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
	result = GammaCorrection(result);
	
	reflected_light = result;
}


vec3 GammaCorrection(vec3 pixelColour)
{
	// Note: USE GL_SRGB INSTEAD OF GL_RGBA for texture setup
	vec3 gamma = vec3(1.0 / 2.2);
	return vec3(pow(pixelColour.x , gamma.x) 
		, pow(pixelColour.y, gamma.y)
		, pow(pixelColour.z , gamma.z));
}