#version 330 core

out vec4 FragColor;

uniform vec3 base_color;

void main() {
	FragColor = vec4(base_color, 1.0f);
}

