#pragma once

// TODO: increase these sizes accordingly

// how many vertices are alloed per chunk?
#define VERTICES_PER_BATCH (1024)

// if size of changed VBO is greater than threshold * VERTICES_PER_BATCH, just update entire VBO
#define UPDATE_ALL_THRESHOLD (0.9f)

#include "DestructibleMapShader.h"

class DestructibleMapChunk;

class DestructibleMapDrawingBatch
{
	GLuint vao_;
	GLuint vbo_;

	float vertex_data_[VERTICES_PER_BATCH * 2];
	int allocated_;
	int last_allocated_;
	bool is_dirty_;
public:
	DestructibleMapDrawingBatch();
	~DestructibleMapDrawingBatch();

	void draw();
	void init();
	bool is_free(int num_vertices) const;
	void alloc_chunk(DestructibleMapChunk *chunk);
	void dealloc_chunk(DestructibleMapChunk *chunk);
};

