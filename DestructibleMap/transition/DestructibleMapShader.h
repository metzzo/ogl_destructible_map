#pragma once
#include "ShaderResource.h"

class DestructibleMap;

const unsigned int max_nr_lights = 10;
class DestructibleMapShader :
	public ShaderResource
{
	GLint view_uniform_;
	GLint projection_uniform_;
	GLint base_color_uniform_;
public:
	DestructibleMapShader();
	~DestructibleMapShader();

	void init() override;
	
	
	void set_camera_uniforms(const glm::mat4 &view_matrix, const glm::mat4 &projection_matrix) override;
	void set_base_color(const glm::vec3 &color) const;
};

