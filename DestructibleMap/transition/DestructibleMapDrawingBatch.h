#pragma once

// TODO: increase these sizes accordingly

// how many vertices are alloed per chunk?
#define VERTICES_PER_CHUNK (1024)

// how many chunks per batch?
#define CHUNKS_PER_BATCH (8)
#include "DestructibleMapShader.h"

class DestructibleMapChunk;

class DestructibleMapDrawingBatch
{
	GLuint vao_;
	GLuint vbo_;

	DestructibleMapChunk *chunks_[CHUNKS_PER_BATCH];
	float vertex_data_[CHUNKS_PER_BATCH * VERTICES_PER_CHUNK * 2];
	int free_slots_;
public:
	DestructibleMapDrawingBatch();
	~DestructibleMapDrawingBatch();

	void update_vbo(int index);
	void draw();
	void init();
	bool is_free() const;
	int alloc_chunk(DestructibleMapChunk *chunk);
};

