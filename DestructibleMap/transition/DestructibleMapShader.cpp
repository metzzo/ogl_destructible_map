#include "DestructibleMapShader.h"
#include "DestructibleMap.h"
#include "TextureResource.h"
#include "MeshResource.h"

DestructibleMapShader::DestructibleMapShader() : ShaderResource("assets/shaders/map_shader.vs", "assets/shaders/map_shader.fs")
{
	this->view_uniform_ = -1;
	this->projection_uniform_ = -1;
	this->base_color_uniform_ = -1;
}


void DestructibleMapShader::set_camera_uniforms(const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) {
	assert(this->view_uniform_ >= 0);
	assert(this->projection_uniform_ >= 0);

	glUniformMatrix4fv(this->view_uniform_, 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(this->projection_uniform_, 1, GL_FALSE, &projection_matrix[0][0]);
}

void DestructibleMapShader::set_base_color(const glm::vec3& color) const
{
	glUniform3fv(this->base_color_uniform_, 1, &color[0]);
}

DestructibleMapShader::~DestructibleMapShader()
{
}

void DestructibleMapShader::init()
{
	ShaderResource::init();

	// extract uniforms
	this->view_uniform_ = get_uniform("vp.view");
	this->projection_uniform_ = get_uniform("vp.projection");
	this->base_color_uniform_ = get_uniform("base_color");
}
