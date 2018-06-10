#version 330

in vec3 worldPos;

uniform vec3 lightPosition;


layout(location = 0) out float fragmentDepth;


void main(void)
{
	vec3 lightToVertex = worldPos - lightPosition;
	float dist = distance(lightPosition, worldPos);
	//float dist = length(lightToVertex);
	fragmentDepth = clamp(dist, 0, 1);
}

