#pragma once

// TODO: increase these sizes accordingly

// how many vertices are allowed per batch?
#define VERTICES_PER_BATCH (2048)

#define CHUNK_PADDING (VERTICES_PER_BATCH * 0.25)

// how many batches are available on start
#define NUM_START_BATCHES (32)

#include "DestructibleMapShader.h"
#include <vector>

class DestructibleMapChunk;
class DestructibleMapDrawingBatch;

struct BatchInfo
{
	DestructibleMapDrawingBatch *batch;
	DestructibleMapChunk *chunk;
	int offset;
	int size;
	int batch_index;
	GLsizei size_without_padding;
};

class DestructibleMapDrawingBatch
{
	GLuint vao_;
	GLuint vbo_;

	float vertex_data_[VERTICES_PER_BATCH * 2];
	int allocated_;
	bool is_all_dirty_;
	std::vector<BatchInfo*> infos_;
	bool is_sub_dirty_;
	int sub_start_offset_;
	int sub_end_offset_;
public:
	DestructibleMapDrawingBatch();
	~DestructibleMapDrawingBatch();

	void draw(DestructibleMapShader *shader);
	void init();
	bool is_free(int num_vertices) const;
	void alloc_chunk(DestructibleMapChunk *chunk);
	void dealloc_chunk(DestructibleMapChunk *chunk);
	void resize_chunk(DestructibleMapChunk *chunk);

	friend DestructibleMap;
};

