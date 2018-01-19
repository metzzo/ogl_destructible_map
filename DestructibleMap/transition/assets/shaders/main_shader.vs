#version 330 core
layout (location = 0) in vec2 aPos;

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} vs_out;

struct MVP {
	mat4 model;
	mat4 view;
	mat4 projection;
};

uniform MVP mvp;

void main()
{
	vs_out.frag_pos = vec3(mvp.model * vec4(aPos, 0.0, 1.0));
	vs_out.normal = vec3(0);
	vs_out.tex_coords = vec2(0);
	
	gl_Position = mvp.projection * mvp.view * vec4(vs_out.frag_pos, 1.0);
}