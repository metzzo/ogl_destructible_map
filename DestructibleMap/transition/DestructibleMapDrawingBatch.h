#pragma once

// TODO: increase these sizes accordingly

// how many vertices are allowed per batch?
#define VERTICES_PER_BATCH (4096)

// how many batches are available on start
#define NUM_START_BATCHES (64)

// how many vertices per chunk should be allowed
#define VERTICES_PER_CHUNK (64)

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
};

class DestructibleMapDrawingBatch
{
	GLuint vao_;
	GLuint vbo_;

	float vertex_data_[VERTICES_PER_BATCH * 2];
	int allocated_;
	bool is_dirty_;
	std::vector<BatchInfo*> infos_;
public:
	DestructibleMapDrawingBatch();
	~DestructibleMapDrawingBatch();

	void draw(DestructibleMapShader *shader);
	void init();
	bool is_free(int num_vertices) const;
	void alloc_chunk(DestructibleMapChunk *chunk);
	void dealloc_chunk(DestructibleMapChunk *chunk);

	friend DestructibleMap;
};

