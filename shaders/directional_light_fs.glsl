#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;
uniform sampler2DRect sampler_world_ambient;
uniform sampler2DRect sampler_world_specular;
uniform sampler2DShadow sampler_shadow;
// NOTE HOW DO I PASS MATIERAL AMBIENT ETC? WELL IN GBUFFER PASS TO A TEXTURE?

uniform mat4 light_projection_view_xform;
uniform vec3 light_direction;
uniform vec3 light_intensity;


layout(location = 0) out vec3 reflected_light;


const int pcfCount = 5; // 1 either side of pixel (3x3 area)
const float totalTexels = (pcfCount * 2.0 + 1.0) * (pcfCount * 2.0 + 1.0);
const float shadowMapSize = 4096.0; //todo, make uniform so dont have to change if change in C++
const float texelSize = 1.0 / shadowMapSize;

//Function Prototypes
vec3 GetDirectionalLight(vec3, vec3);
float CalculateShadowVisibility(vec3, vec3);
vec3 Specular(vec3, vec3, vec3, vec3, float, vec3);

void main(void)
{
	vec3 texel_world_pos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_world_normal = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_world_ambient = texelFetch(sampler_world_ambient, ivec2(gl_FragCoord.xy)).rgb;

	//
	float total = 0.0;

	for (int x = -pcfCount; x <= pcfCount; x++)
	{
		for (int y = -pcfCount; y <= pcfCount; y++)
		{
			
			float visibility = CalculateShadowVisibility(texel_world_pos, vec3(x, y, 0) * texelSize);
			if (visibility >= 1.0)
				total += 1.0;
		}
	}
	total /= totalTexels;
	total = max(0.33, total);
	//




	reflected_light = /*texel_world_ambient **/   total * GetDirectionalLight(texel_world_pos, texel_world_normal);
}

float CalculateShadowVisibility(vec3 pos, vec3 offset)
{
	vec4 shadowCoord = light_projection_view_xform * vec4(pos, 1.0);
	//shadowCoord.xyz /= shadowCoord.w; // ONLY FOR PERSPECTIVE
	shadowCoord.xyz += 1.0;
	shadowCoord.xyz *= 0.5;
	shadowCoord.xyz += offset;

	float bias = 0.0000;// ASK keith about improving bias, len(acos(angleEye and reflection)??
	//bias = max(0.005 * (1.0 - dot(normalize(normal), light_direction)), 0.001);
	return texture(sampler_shadow, vec3(shadowCoord.xy, shadowCoord.z /*- bias*/));
}


vec3 GetDirectionalLight(vec3 position, vec3 normal)
{
	vec3 P = position;
	vec3 N = normalize(normal);
	vec3 L = normalize(-light_direction);
	float LdotN = max(0.0, dot(L, N));

	// SPECULAR
	vec3 specularColour = Specular(light_intensity, P, -L, N, LdotN, light_direction);

	// DIFFUSE
	vec3 texel_world_ambient = texelFetch(sampler_world_ambient, ivec2(gl_FragCoord.xy)).rgb;
	vec3 diffuseColour = vec3(LdotN) * texel_world_ambient.rgb * light_intensity;

	return  (diffuseColour + specularColour) ;// vec3(1.0, 1.0, 1.0);//light_intensity;
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