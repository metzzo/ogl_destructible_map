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
	this->last_allocated_ = 0;
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
		delete info;
	}
}

void DestructibleMapDrawingBatch::draw()
{
	if (this->is_dirty_)
	{
		// if VBO has changed => get changed data
		/*auto start = 0, size = 0;
		if (this->last_allocated_ < this->allocated_)
		{
			start = this->last_allocated_ * 2 * sizeof(float);
			size = (this->allocated_ - this->last_allocated_) * 2 * sizeof(float);
		} else
		{
			start = this->allocated_ * 2 * sizeof(float);
			size = (this->last_allocated_ - this->allocated_) * 2 * sizeof(float);
		}
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
		if (size >= VERTICES_PER_BATCH * UPDATE_ALL_THRESHOLD * 2 * sizeof(float))
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VERTICES_PER_BATCH * 2, this->vertex_data_, GL_DYNAMIC_DRAW);
		} else
		{
			glBufferSubData(GL_ARRAY_BUFFER, start, size, this->vertex_data_);
		}*/
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VERTICES_PER_BATCH * 2, this->vertex_data_, GL_DYNAMIC_DRAW);
		this->is_dirty_ = false;
		this->last_allocated_ = this->allocated_;
	}

	if (this->allocated_ > 0) {
		map_draw_calls++;
		glBindVertexArray(this->vao_);
		glDrawArrays(GL_TRIANGLES, 0, this->allocated_ * 2);
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
	return (this->allocated_ + for_size) <= VERTICES_PER_BATCH;
}

void DestructibleMapDrawingBatch::alloc_chunk(DestructibleMapChunk *chunk, int index)
{
	assert(chunk->vertices_.size() - index*VERTICES_PER_BATCH > 0);

	const auto new_vertices_count = std::min(int(chunk->vertices_.size()) - index*VERTICES_PER_BATCH, VERTICES_PER_BATCH);
	assert(this->is_free(new_vertices_count));

	auto info = new BatchInfo();
	info->batch = this;
	info->chunk = chunk;
	info->batch_index = this->infos_.size();
	info->offset = this->allocated_;
	info->size = new_vertices_count;

	// update own array
	for (auto i = 0; i < new_vertices_count; i++)
	{
		const auto vertex = chunk->vertices_[index*VERTICES_PER_BATCH + i];

		this->vertex_data_[(this->allocated_ + i) * 2] = vertex.x;
		this->vertex_data_[(this->allocated_ + i) * 2 + 1] = vertex.y;
	}

	this->allocated_ += new_vertices_count;
	this->is_dirty_ = true;
	this->infos_.push_back(info);
	chunk->add_batch_info(info);
}

void DestructibleMapDrawingBatch::dealloc_chunk(DestructibleMapChunk* chunk, int index)
{
	auto info = chunk->get_batch_info(index);
	const auto batch_index = info->offset;
	const auto batch_size = info->size;

	assert(batch_index >= 0 && batch_size >= 0);
	assert(info->batch == this);

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
}
