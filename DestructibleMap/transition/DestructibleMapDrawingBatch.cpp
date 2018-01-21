#include "DestructibleMapDrawingBatch.h"
#include "DestructibleMapChunk.h"
#include <cassert>
#include <glad/glad.h>
#include <iostream>

DestructibleMapDrawingBatch::DestructibleMapDrawingBatch()
{
	this->vao_ = 0;
	this->vbo_ = 0;
	this->allocated_ = 0;
	this->first_modified_ = INT_MAX;
	this->is_dirty_ = false;
	for (auto i = 0; i < VERTICES_PER_BATCH * 2; i++)
	{
		this->vertex_data_[i] = 0.0;
	}
}


DestructibleMapDrawingBatch::~DestructibleMapDrawingBatch()
{
	glDeleteVertexArrays(1, &this->vao_);
	glDeleteBuffers(1, &this->vbo_);

	for (auto &info : this->infos_)
	{
		if (info->chunk != nullptr)
		{
			info->chunk->update_batch(nullptr);
		}
		delete info;
	}
}

void DestructibleMapDrawingBatch::draw(DestructibleMapShader *shader)
{
	if (this->is_dirty_)
	{
		assert(VERTICES_PER_BATCH >= this->allocated_);

		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
		if (this->first_modified_ < UPDATE_ALL_THRESHOLD && false)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VERTICES_PER_BATCH * 2, this->vertex_data_, GL_DYNAMIC_DRAW);
		} else
		{
			std::cout << this->first_modified_ << " " << this->allocated_ << std::endl;
			const auto start = this->first_modified_ * 2 * sizeof(float);
			const auto size = (this->allocated_ - this->first_modified_) * 2 * sizeof(float);
			glBufferSubData(GL_ARRAY_BUFFER, start, size, this->vertex_data_);
		}


		// this does not fix the issue, therefore its not because the vertex_data is out of sync
		/*for (auto &info : this->infos_)
		{
			for (auto i = info->offset; i < info->offset + info->size; i++)
			{
				auto vertex = info->chunk->vertices_[i - info->offset];

				this->vertex_data_[i * 2] = vertex.x;
				this->vertex_data_[i * 2 + 1] = vertex.y;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * this->allocated_ * 2, this->vertex_data_, GL_DYNAMIC_DRAW);*/


		this->is_dirty_ = false;
		this->first_modified_ = INT_MAX;
	}

	if (this->allocated_ > 0) {
		map_draw_calls++;
		glBindVertexArray(this->vao_);
		glDrawArrays(GL_TRIANGLES, 0, this->allocated_);
	}
}

void DestructibleMapDrawingBatch::init()
{
	glGenVertexArrays(1, &this->vao_);
	glGenBuffers(1, &this->vbo_);
	glBindVertexArray(vao_);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VERTICES_PER_BATCH * 2, this->vertex_data_, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glBindVertexArray(0);
}

bool DestructibleMapDrawingBatch::is_free(int for_size) const
{
	return (this->allocated_ + for_size) < VERTICES_PER_BATCH;
}

void DestructibleMapDrawingBatch::alloc_chunk(DestructibleMapChunk *chunk)
{
	assert(this->is_free(chunk->vertices_.size()));
	assert(chunk->get_batch_info() == nullptr);

	const auto new_vertices_count = chunk->vertices_.size();

	auto info = new BatchInfo();
	info->batch = this;
	info->chunk = chunk;
	info->batch_index = this->infos_.size();
	info->offset = this->allocated_;
	info->size = new_vertices_count;

	// update array
	for (auto i = 0; i < new_vertices_count; i++)
	{
		const auto vertex = chunk->vertices_[i];

		this->vertex_data_[(this->allocated_ + i) * 2] = vertex.x;
		this->vertex_data_[(this->allocated_ + i) * 2 + 1] = vertex.y;
	}

	this->allocated_ += new_vertices_count;
	this->is_dirty_ = true;
	this->infos_.push_back(info);
	chunk->update_batch(info);
}

void DestructibleMapDrawingBatch::dealloc_chunk(DestructibleMapChunk* chunk)
{
	auto info = chunk->get_batch_info();
	const auto batch_index = info->offset;
	const auto batch_size = info->size;

	assert(batch_index >= 0 && batch_size >= 0);
	assert(info->batch == this);

	// update array
	for (auto i = batch_index + batch_size; i < this->allocated_; i++)
	{
		this->vertex_data_[(i - batch_size) * 2] = this->vertex_data_[i * 2];
		this->vertex_data_[(i - batch_size) * 2 + 1] = this->vertex_data_[i * 2 + 1];
	}
	this->allocated_ -= batch_size;
	this->is_dirty_ = true;

	// reset batch info
	for (int i = info->batch_index + 1; i < this->infos_.size(); i++)
	{
		auto &tmp = this->infos_[i];
		tmp->batch_index--;
		tmp->offset -= batch_size;
	}
	this->infos_.erase(this->infos_.begin() + info->batch_index);
	delete info;

	chunk->update_batch(nullptr);
}
