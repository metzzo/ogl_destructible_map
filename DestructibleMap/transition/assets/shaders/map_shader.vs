#version 330 core
layout (location = 0) in vec2 aPos;

struct MVP {
	mat4 model;
	mat4 view;
	mat4 projection;
};

uniform MVP mvp;

void main()
{
	gl_Position = mvp.projection * mvp.view * vec4(vs_out.frag_pos, 1.0);
}