#pragma once
#include "IResource.h"
#include <glad/glad.h>
#include <vector>
#include <glm/detail/type_vec2.hpp>

class MeshResource : public IResource
{
	float *vertices_ = nullptr;
	float *normals_ = nullptr;
	float *uvs_ = nullptr;
	int num_vertices_;

	unsigned int *indices_;
	int num_indices_;
	GLuint vao_;
	GLuint vbo_positions_;
	GLuint vbo_normals_;
	GLuint vbo_uvs_;
	GLuint ebo_;
public:
	MeshResource(float *vertices, float *normals, float *uvs, int num_vertices, unsigned int *indices, int num_indices);
	MeshResource(std::vector<glm::vec2> vertices);
	~MeshResource();

	int get_resource_id() const override;
	void init() override;

	int get_num_indices() const
	{
		return num_indices_;
	}

	int get_num_vertices() const
	{
		return num_vertices_;
	}
};

