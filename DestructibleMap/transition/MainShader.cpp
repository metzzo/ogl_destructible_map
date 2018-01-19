#include "MainShader.h"
#include "DestructibleMapNode.h"
#include "TextureResource.h"
#include "MeshResource.h"

MainShader::MainShader() : ShaderResource("assets/shaders/main_shader.vs", "assets/shaders/main_shader.fs")
{
	this->model_uniform_ = -1;
	this->view_uniform_ = -1;
	this->projection_uniform_ = -1;
	this->material_diffuse_tex_uniform_ = -1;
	this->material_has_diffuse_tex_uniform_ = -1;
	this->material_shininess_ = -1;
	this->material_ambient_color_ = -1;
	this->material_diffuse_color_ = -1;
	this->material_specular_color_ = -1;
}


void MainShader::set_camera_uniforms(const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) {
	assert(this->view_uniform_ >= 0);
	assert(this->projection_uniform_ >= 0);

	glUniformMatrix4fv(this->view_uniform_, 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(this->projection_uniform_, 1, GL_FALSE, &projection_matrix[0][0]);
}
void MainShader::set_model_uniforms(const DestructibleMapNode* node, void * param)
{
	//Check Existance of Uniforms
	assert(this->model_uniform_ >= 0);
	assert(this->material_diffuse_tex_uniform_ >= 0);
	assert(this->material_has_diffuse_tex_uniform_ >= 0);
	//Give Model to Shader
	glUniformMatrix4fv(this->model_uniform_, 1, GL_FALSE, &glm::mat4()[0][0]);
	//Bind Texture and give it to Shader 
	auto material = (static_cast<bool*>(param)[0] ? node->point_distribution_resource_ : node->quadtree_resource_)->get_material();
	const TextureResource* texture = material.get_texture();
	if (texture != nullptr) {
		texture->bind(0);
		glUniform1i(this->material_has_diffuse_tex_uniform_, 1);
		glUniform1i(this->material_diffuse_tex_uniform_, 0);
		glUniform1f(this->material_shininess_, material.get_shininess());
	}
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glUniform1i(this->material_has_diffuse_tex_uniform_, 0);
	}
	glUniform3fv(this->material_ambient_color_, 1, &material.get_ambient_color()[0]);
	glUniform3fv(this->material_diffuse_color_, 1, &material.get_diffuse_color()[0]);
	glUniform3fv(this->material_specular_color_, 1, &material.get_specular_color()[0]);

}

MainShader::~MainShader()
{
}

void MainShader::init()
{
	ShaderResource::init();

	// extract uniforms
	this->model_uniform_ = get_uniform("mvp.model");
	this->view_uniform_ = get_uniform("mvp.view");
	this->projection_uniform_ = get_uniform("mvp.projection");
	this->material_diffuse_tex_uniform_ = get_uniform("material.diffuse_tex");
	this->material_has_diffuse_tex_uniform_ = get_uniform("material.has_diffuse_tex");
	this->material_shininess_ = get_uniform("material.shininess");
	this->material_ambient_color_ = get_uniform("material.ambient_color");
	this->material_diffuse_color_ = get_uniform("material.diffuse_color");
	this->material_specular_color_ = get_uniform("material.specular_color");
}
