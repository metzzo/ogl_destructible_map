#version 330 core
layout (location = 0) in vec2 aPos;

struct VP {
	mat4 view;
	mat4 projection;
};

uniform VP vp;

void main()
{
	gl_Position = vp.projection * vp.view * vec4(aPos, 0.0, 1.0);
}