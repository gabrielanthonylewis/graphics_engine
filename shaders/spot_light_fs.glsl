#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;
uniform sampler2DRect sampler_world_specular;
uniform sampler2DRect sampler_world_ambient;
uniform sampler2DShadow sampler_shadow;

out vec3 reflected_light;

uniform mat4 light_projection_view_xform;
uniform vec3 light_colour_intensity;
uniform vec3 light_position;
uniform float light_range;
uniform vec3 light_direction;
uniform int cast_shadow;
uniform float cone_angle_degrees;



const int pcfCount = 5; // 1 either side of pixel (3x3 area)
const float totalTexels = (pcfCount * 2.0 + 1.0) * (pcfCount * 2.0 + 1.0);
const float shadowMapSize = 4096.0; //todo, make uniform so dont have to change if change in C++
const float texelSize = 1.0 / shadowMapSize;

//
vec3 CalculateSpotLight(vec3, vec3);
vec3 GetPointLight(vec3, vec3);
vec3 SLNew(vec3, vec3);
vec3 SLNew2(vec3, vec3);
vec3 SLNew3(vec3, vec3);
vec3 Attenuation(float, vec3, float, float);
float CalculateShadowVisibility(vec3, vec3, vec3);
vec3 Specular(vec3, vec3, vec3, vec3, float, vec3);

void main(void)
{

	vec3 texel_world_pos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb; 
	vec3 texel_world_normal = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb; 

	//vec3 Nresult = 0.5 + 0.5 * normalize(texel_world_normal);
	//
	float total = 0.0;

	for (int x = -pcfCount; x <= pcfCount; x++)
	{
		for (int y = -pcfCount; y <= pcfCount; y++)
		{

			float visibility = CalculateShadowVisibility(texel_world_pos, texel_world_normal, vec3(x, y, 0) * texelSize);
			if (visibility >= 1.0)
				total += 1.0;
		}
	}
	total /= totalTexels;
	total = max(0.33, total);
	//

	reflected_light = SLNew3(texel_world_pos, texel_world_normal);// *CalculateShadowVisibility(texel_world_pos, texel_world_normal, vec3(0, 0, 0));// CalculateSpotLight(texel_world_pos, texel_world_normal);  // vec3(1.0, 0.33, 0.0) * 
}

float CalculateShadowVisibility(vec3 pos, vec3 normal, vec3 offset)
{
	vec4 shadowCoord = light_projection_view_xform * vec4(pos, 1.0);
	//shadowCoord.xyz /= shadowCoord.w; // ONLY FOR PERSPECTIVE
	shadowCoord.xyz += 1.0;
	shadowCoord.xyz *= 0.5;
	shadowCoord.xyz += offset;

	float bias = 0.0000;// ASK keith about improving bias, len(acos(angleEye and reflection)??
						//bias = max(0.005 * (1.0 - dot(normalize(normal), light_direction)), 0.001);
	return texture(sampler_shadow, vec3(shadowCoord.xy / shadowCoord.w, shadowCoord.z / shadowCoord.w/*- bias*/));
}

// maybe use this https://www.opengl.org/discussion_boards/showthread.php/177668-GLSL-Stationary-spotlight
//https://www.tomdalling.com/blog/modern-opengl/08-even-more-lighting-directional-lights-spotlights-multiple-lights/

vec3 SLNew3(vec3 position, vec3 normal)
{
	vec3 lPos = light_position; // lPos in light space?
	vec3 toLight = normalize(lPos - position); // position into world?
	vec3 lDir = normalize(light_direction);

	float angle = acos(dot(-toLight, lDir));
	float cutoffAngle = radians(cone_angle_degrees); // was radians

	if (angle > cutoffAngle)
		return vec3(0.0, 0.0, 0.0);
	else
	{
		/*vec3 P = position;
		vec3 N = normalize(normal);
		vec3 L = normalize(light_position - P);
		float LdotN = max(0.0, dot(L, N));*/

		return GetPointLight(position, normal);// *Attenuation(light_range, light_colour_intensity, distance(light_position, P), LdotN);
	}
}

vec3 GetPointLight(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 N = normalize(normal);
	vec3 L = normalize(light_position - P);
	float LdotN = max(0.0, dot(L, N));

	// DIFFUSE
	vec3 texel_world_ambient = texelFetch(sampler_world_ambient, ivec2(gl_FragCoord.xy)).rgb;
	vec3 diffuseColor = vec3(LdotN) * texel_world_ambient.rgb * light_colour_intensity;

	vec3 attenuation = Attenuation(light_range, light_colour_intensity, distance(light_position, P), LdotN);

	// SPECULAR
	vec3 specularColour = Specular(light_colour_intensity, P, -L, N, LdotN, light_direction);

	return  (diffuseColor + specularColour) * attenuation;
}

vec3 SLNew2(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 L = normalize(light_position - P);

	float minCos = cos(radians(cone_angle_degrees));
	float maxCos = mix(minCos, 1.0f, 0.5f);
	float cosAngle = dot(light_direction, -L);

	float coneLight =  smoothstep(minCos, maxCos, cosAngle);

	vec3 N = normalize(normal);
	float LdotN = max(0.0, dot(L, N));
	vec3 diffuseColour = vec3(LdotN) /** surface_colour.rgb*/ * light_colour_intensity;


	vec3 attenuation = Attenuation(light_range, light_colour_intensity, distance(light_position, P), LdotN);

	return vec3(coneLight) * light_colour_intensity * attenuation;
}

vec3 SLNew(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 L = normalize(light_position - P);

	vec3 coneDirection = normalize(light_direction);
	vec3 rayDirection = -L;

	float lightToSurfaceAngle = degrees(acos(dot(rayDirection, coneDirection)));

	vec3 attenuation = vec3(1.0, 1.0, 1.0);
	if (lightToSurfaceAngle > cone_angle_degrees)
	{
		attenuation = vec3(0.0, 0.0, 0.0);
	}

	vec3 N = normalize(normal);
	float LdotN = max(0.0, dot(L, N));
	vec3 diffuseColour = vec3(LdotN) /** surface_colour.rgb*/ * light_colour_intensity;


	return diffuseColour * attenuation;
}

vec3 CalculateSpotLight(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 N = normalize(normal);
	vec3 L = normalize(light_position - P); // light direction
	float LdotN = max(0.0, dot(L, N));


	vec3 attenuation = vec3(1, 1, 1);

	if (LdotN > 0.0)
	{
		// Check to see if in cone
		float lightToSurfaceAngle = dot(-L, normalize(light_direction));
		if (acos(lightToSurfaceAngle) > radians(cone_angle_degrees))
		{
			attenuation = vec3(0, 0, 0);
		}
	}
	


	// DIFFUSE
	vec3 diffuseColour = vec3(LdotN) /** surface_colour.rgb*/ * light_colour_intensity;
	
	vec3 distAttenuation = Attenuation(light_range, light_colour_intensity, length(L), LdotN);//distance(light_position, P)

	return  diffuseColour * attenuation;// *distAttenuation;
}

vec3 Attenuation(float light_range, vec3 light_colour_intensity, float distance, float LdotN)
{
	vec3 attenuation = vec3(1, 1, 1);

	const float ATTENUATION_CONSTANT = 1.0;
	const float ATTENUATION_LINEAR = 0.1;
	const float ATTENUATION_QUADRATIC = 0.01;
	const float ATTENUATION_FACTOR = (ATTENUATION_CONSTANT + ATTENUATION_LINEAR + ATTENUATION_QUADRATIC);


	distance -= smoothstep(light_range * 0.75, light_range * 0.75, distance); // 0.75 as need to "Fades 25% of light range"

	float rd = (1.0 / ATTENUATION_FACTOR);
	float fd = 1.0 - smoothstep(0, light_range, distance);
	float I = fd * LdotN * rd;
	attenuation = vec3(I) *light_colour_intensity;

	return attenuation * 0.25 ;
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