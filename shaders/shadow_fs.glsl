#version 330

layout(location = 0) out float fragmentDepth;


void main(void)
{
	fragmentDepth = gl_FragCoord.z;
}

