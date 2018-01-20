#include "DestructibleMapDrawingBatch.h"
#include "DestructibleMapChunk.h"
#include <cassert>
#include <glad/glad.h>


void DestructibleMapDrawingBatch::update_vbo(int index)
{
	assert(index < CHUNKS_PER_BATCH);

	const auto chunk = this->chunks_[index];

	// update own array
	for (auto i = 0; i < chunk->vertices_.size(); i++)
	{
		const auto vertex = chunk->vertices_[i];

		this->vertex_data_[index*VERTICES_PER_CHUNK + i * 2] = vertex.x;
		this->vertex_data_[index*VERTICES_PER_CHUNK + i * 2 + 1] = vertex.y;
	}

	// now update vbo
	const auto start = index * VERTICES_PER_CHUNK * 2 * sizeof(float);
	const auto size = VERTICES_PER_CHUNK * 2 * sizeof(float);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
	glBufferSubData(GL_ARRAY_BUFFER, start, size, this->vertex_data_);
}

DestructibleMapDrawingBatch::DestructibleMapDrawingBatch()
{
	this->vao_ = 0;
	this->vbo_ = 0;
	this->free_slots_ = CHUNKS_PER_BATCH;
	for (auto i = 0; i < CHUNKS_PER_BATCH; i++)
	{
		this->chunks_[i] = nullptr;
	}
	for (auto i = 0; i < CHUNKS_PER_BATCH * VERTICES_PER_CHUNK * 2; i++)
	{
		this->vertex_data_[i] = 0.0;
	}
}


DestructibleMapDrawingBatch::~DestructibleMapDrawingBatch()
{
	glDeleteVertexArrays(1, &this->vao_);
	glDeleteBuffers(1, &this->vbo_);
}

void DestructibleMapDrawingBatch::draw()
{
	if (this->is_free())
	{
		// draw only selected columns
		glBindVertexArray(this->vao_);
		auto last = 0, i = 0;
		for (i = 0; i < CHUNKS_PER_BATCH; i++)
		{
			if (!this->chunks_[i])
			{
				map_draw_calls++;
				glDrawArrays(GL_TRIANGLES, last * VERTICES_PER_CHUNK, i * VERTICES_PER_CHUNK - 1);
				last = i;
			}
		}
		map_draw_calls++;
		glDrawArrays(GL_TRIANGLES, last * VERTICES_PER_CHUNK, i * VERTICES_PER_CHUNK - 1);
	} else
	{
		// draw entire vbo
		map_draw_calls++;
		glBindVertexArray(this->vao_);
		glDrawArrays(GL_TRIANGLES, 0, CHUNKS_PER_BATCH * VERTICES_PER_CHUNK - 1);
	}
}

void DestructibleMapDrawingBatch::init()
{
	glGenVertexArrays(1, &this->vao_);
	glGenBuffers(1, &this->vbo_);
	glBindVertexArray(vao_);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * CHUNKS_PER_BATCH * VERTICES_PER_CHUNK * 2, nullptr, GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glBindVertexArray(0);
}

bool DestructibleMapDrawingBatch::is_free() const
{
	return this->free_slots_ > 0;
}

int DestructibleMapDrawingBatch::alloc_chunk(DestructibleMapChunk *chunk)
{
	assert(this->is_free());

	for (int i = 0; i < CHUNKS_PER_BATCH; i++)
	{
		if (this->chunks_[i] == nullptr)
		{
			this->chunks_[i] = chunk;
			chunk->update_batch(this, i);
			this->update_vbo(i);
			this->free_slots_--;
			return i;
		}
	}
	return -1;
}
