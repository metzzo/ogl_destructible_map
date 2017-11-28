#include "MeshResource.h"
#include <cstring>
#include "glheaders.h"

MeshResource::MeshResource(float *vertices, float *normals, float *uvs, const int num_vertices, unsigned int *indices, const int num_indices, const Material& material) {
	this->vao_ = -1;
	this->vbo_positions_ = -1;
	this->vbo_normals_ = -1;
	this->vbo_uvs_ = -1;
	this->ebo_ = -1;

	this->vertices_ = vertices;
	this->normals_ = normals;
	this->uvs_ = uvs;
	this->num_vertices_ = num_vertices;
	this->indices_ = indices;
	this->num_indices_ = num_indices;
	this->material_ = material;
}

MeshResource::MeshResource(std::vector<glm::vec2> vertices, const Material& material)
{
	const auto global_vertices = new float[vertices.size() * 3];
	const auto global_normal = new float[vertices.size() * 3];
	const auto global_uv = new float[vertices.size() * 2];
	auto i = 0;
	for (auto& vertex : vertices)
	{
		global_uv[i * 2] = 0;
		global_uv[i * 2 + 1] = 0;

		global_normal[i * 3] = 0;
		global_normal[i * 3 + 1] = 0;
		global_normal[i * 3 + 2] = 0;


		global_vertices[i * 3] = vertex.x;
		global_vertices[i * 3 + 1] = vertex.y;
		global_vertices[i * 3 + 2] = 0;

		i++;
	}

	this->vao_ = -1;
	this->vbo_positions_ = -1;
	this->vbo_normals_ = -1;
	this->vbo_uvs_ = -1;
	this->ebo_ = -1;

	this->vertices_ = global_vertices;
	this->normals_ = global_normal;
	this->uvs_ = global_uv;
	this->num_vertices_ = vertices.size();
	this->indices_ = nullptr;
	this->num_indices_ = 0;
	this->material_ = material;
}


MeshResource::~MeshResource()
{
	if (this->vao_ != -1) {
		glDeleteVertexArrays(1, &this->vao_);
		delete this->vertices_;
	}
	if (this->vbo_positions_ != -1) {
		glDeleteBuffers(1, &this->vbo_positions_);
	}
	if (this->vbo_normals_ != -1) {
		glDeleteBuffers(1, &this->vbo_normals_);
		delete this->normals_;
	}
	if (this->ebo_ != -1) {
		glDeleteBuffers(1, &this->ebo_);
		delete this->indices_;
	}

	if (this->vbo_uvs_ != -1) {
		glDeleteBuffers(1, &this->vbo_uvs_);
		delete this->uvs_;
	}
}

int MeshResource::get_resource_id() const
{
	return this->vao_;
}

void MeshResource::init()
{
	glGenVertexArrays(1, &this->vao_);
	glGenBuffers(1, &this->vbo_positions_);
	glGenBuffers(1, &this->vbo_uvs_);
	glGenBuffers(1, &this->vbo_normals_);
	glBindVertexArray(vao_);

	//Bind Positions to Shader-Location 0
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_positions_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*this->num_vertices_ * 3, this->vertices_, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	

	//Bind UVs to Shader-Location 1
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*this->num_vertices_ * 2, this->uvs_, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

	//Bind Normals to Shader-Location 2
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_normals_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*this->num_vertices_ * 3, this->normals_, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	
	if (this->indices_ != nullptr) {
		glGenBuffers(1, &this->ebo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*this->num_indices_, this->indices_, GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}
