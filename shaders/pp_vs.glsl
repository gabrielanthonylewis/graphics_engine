#version 330


in vec2 vertex_position;

//out vec2 textureCoords;

void main(void)
{
	gl_Position = vec4(vertex_position, 0.0, 1.0);
	//textureCoords = vertex_position * 0.5 + 0.5;
}