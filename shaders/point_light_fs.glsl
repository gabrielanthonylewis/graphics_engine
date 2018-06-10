#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;
uniform sampler2DRect sampler_world_ambient;
uniform sampler2DRect sampler_world_specular;
uniform sampler2DShadow  sampler_shadow;
uniform samplerCube sampler_shadow_cube;

uniform mat4 light_projection_view_xform;
uniform vec3 light_position;
uniform vec3 light_intensity;
uniform float light_range; // * 2.0f?


out vec3 reflected_light;

// Function Prototypes
vec3 GetPointLight(vec3, vec3);
vec3 Attenuation(float, vec3, float, float);
float CalculateShadowVisibility(vec3, vec3);
float CalculateShadowVisibilityCube(vec3, vec3, vec3);
vec3 Specular(vec3, vec3, vec3, vec3, float, vec3);

void main(void)
{
	vec3 texel_world_pos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb; 
	vec3 texel_world_normal = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb; 


	vec3 result = 0.5 + 0.5 * normalize(texel_world_normal);

	// Shadow
	//float visibility = CalculateShadowVisibility(texel_world_pos, texel_world_normal);

	vec3 fromLightToFragment = light_position - texel_world_pos.xyz;
	float visibility = CalculateShadowVisibilityCube(texel_world_pos, texel_world_normal, normalize(fromLightToFragment));
	 visibility = 1.0;
	reflected_light = visibility * GetPointLight(texel_world_pos, texel_world_normal); //visibility * 
}

float CalculateShadowVisibilityCube(vec3 pos, vec3 normal, vec3 lightDirection)
{
//	vec4 shadowCoord = (light_projection_view_xform) * vec4(pos, 1.0);
	//shadowCoord.xyz/= shadowCoord.w; //  FOR PERSPECTIVE
	//shadowCoord.xyz += 1.0;
	//shadowCoord.xyz *= 0.5;

//	vec3 worldPosition = (light_projection_view_xform * vec4(pos, 1.0)).xyz;
	//vec3 worldPosition = shadowCoord.xyz;
	vec3 worldPosition = pos; // NEED FROM 2D TEXTURE SPACE TO 3D but only have Light modelxform, not object
	//convert Light into texture space??
	// then convert back into world space for the texture()??


	// difference between position of the light source and position of the fragment
	vec3 fromLightToFragment = light_position - worldPosition; // do light_projection?
	//normlized distance to the point light source
	float distanceToLight = length(fromLightToFragment);
	float currentDistanceToLight = (distanceToLight - 0.01) / (100 - 0.01);
	currentDistanceToLight = clamp(currentDistanceToLight, 0, 1);
	// normalized direction from light source for samplign
	fromLightToFragment = normalize(fromLightToFragment);
	
	// sample shadow cube map 
	float referenceDistanceToLight = texture(sampler_shadow_cube, -fromLightToFragment).r;
	// compare distances to determine wether the fragment is in the shadow
	float shadowFactor = float(referenceDistanceToLight > currentDistanceToLight);

	if (referenceDistanceToLight > currentDistanceToLight)
		return 1.0;
	else
		return 0.0;

	/*vec3 lightToPixel = (light_projection_view_xform * vec4(pos, 1.0)).xyz - light_position;
	float depth = texture(sampler_shadow_cube, normalize(lightToPixel)).r;

	if (depth < length(lightToPixel))
	{
		return 0.5;
	}

	return 1.0;*/

	
	/*float sampledDist = texture(sampler_shadow_cube, lightDirection).r;

	float dist = length(lightDirection);

	float bias = 0.005;
	if (dist < sampledDist + bias)
		return 1.0;
	else
		return 0.0;*/
}

float CalculateShadowVisibility(vec3 pos, vec3 normal)
{
	vec4 shadowCoord = light_projection_view_xform * vec4(pos, 1.0);
	shadowCoord.xyz /= shadowCoord.w; //  FOR PERSPECTIVE
	shadowCoord.xyz += 1.0;
	shadowCoord.xyz *= 0.5;

	float bias = 0.000; // 0.005
	return texture(sampler_shadow, vec3(shadowCoord.xy, shadowCoord.z - bias));
}

vec3 GetPointLight(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 N = normalize(normal);
	vec3 L = normalize(light_position - P);
	float LdotN = max(0.0, dot(L, N));

	// DIFFUSE
	vec3 texel_world_ambient = texelFetch(sampler_world_ambient, ivec2(gl_FragCoord.xy)).rgb;
	vec3 diffuseColor = vec3(LdotN) * texel_world_ambient.rgb * light_intensity;
	//vec3 diffuseColor = vec3(LdotN) * light_intensity;

	vec3 attenuation = Attenuation(light_range, light_intensity, distance(light_position, P), LdotN);

	// SPECULAR
	vec3 specularColour = Specular(light_intensity, P, -L, N, LdotN, light_position - P);

	return (diffuseColor + specularColour) * attenuation;
}

vec3 Attenuation(float light_range, vec3 light_colour_intensity, float distance, float LdotN)
{
	vec3 attenuation = vec3(1, 1, 1);

	const float ATTENUATION_CONSTANT = 1.0;
	const float ATTENUATION_LINEAR = 0.1;
	const float ATTENUATION_QUADRATIC = 0.01;
	const float ATTENUATION_FACTOR = (ATTENUATION_CONSTANT + ATTENUATION_LINEAR + ATTENUATION_QUADRATIC);


	distance -= smoothstep(light_range * 0.75, light_range, distance); // 0.75 as need to "Fades 25% of light range"

	float rd = (1.0 / ATTENUATION_FACTOR);
	float fd = 1.0 - smoothstep(0, light_range, distance);
	float I = fd * LdotN * rd;
	attenuation = vec3(I) * light_colour_intensity;

	return attenuation;
}

vec3 Specular(vec3 light_colour_intensity, vec3 P, vec3 incidence, vec3 N, float LdotN, vec3 source_direction)
{
	// SPECULAR
	vec3 texel_world_specular = texelFetch(sampler_world_specular, ivec2(gl_FragCoord.xy)).rgb;

	vec3 specularColour = vec3(0, 0, 0);
	if (texel_world_specular.r + texel_world_specular.g + texel_world_specular.b > 0.0)
	{
		// only calculate if facing light source 
		if (LdotN > 0.0)
		{
			// Reflection of light from the angle of incidence
			vec3 R = reflect(incidence, N);

			// Eye vector, which is from the surface towards the camera
			vec3 E = normalize(source_direction - P);

			float cosAlpha = max(0.0, dot(E, R));
			float specularFactor = pow(cosAlpha, texel_world_specular.r + texel_world_specular.g + texel_world_specular.b); // was shinyness

			specularColour = specularFactor  * light_colour_intensity; //* texel_world_specular but there is no colour
		}
	}
	return specularColour;
}