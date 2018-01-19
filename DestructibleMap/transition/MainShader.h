#pragma once
#include "ShaderResource.h"

class DestructibleMapNode;

const unsigned int max_nr_lights = 10;
class MainShader :
	public ShaderResource
{
	GLint model_uniform_;
	GLint view_uniform_;
	GLint projection_uniform_;
	GLint material_diffuse_tex_uniform_;
	GLint material_has_diffuse_tex_uniform_;

	GLint material_shininess_;
	GLint material_ambient_color_;
	GLint material_diffuse_color_;
	GLint material_specular_color_;
public:
	MainShader();
	~MainShader();

	void init() override;
	
	
	void set_camera_uniforms(const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) override;
	void set_model_uniforms(const DestructibleMapNode* node, void * param) override;
};

