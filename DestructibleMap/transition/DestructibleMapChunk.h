#pragma once

#include "clipper.hpp"
#include <glm/glm.hpp>

extern int map_draw_calls;

class DestructibleMap;
class RenderingEngine;
class DestructibleMapDrawingBatch;

class DestructibleMapChunk
{
	std::vector<glm::vec2> points_;
	glm::vec2 begin_;
	glm::vec2 end_;

	DestructibleMapChunk *parent_;

	DestructibleMapChunk *north_west_;
	DestructibleMapChunk *north_east_;
	DestructibleMapChunk *south_west_;
	DestructibleMapChunk *south_east_;

	ClipperLib::Paths paths_;
	std::vector<glm::vec2> vertices_;

	bool mesh_dirty_;

	DestructibleMapDrawingBatch *batch_;
	int batch_index_;
	int batch_size_;

	void constructor();
public:

	explicit DestructibleMapChunk(DestructibleMapChunk *parent, const glm::vec2 begin, const glm::vec2 end);
	DestructibleMapChunk();
	~DestructibleMapChunk();

	void get_lines(std::vector<glm::vec2>& lines) const;

	bool insert(const glm::vec2& point, const int max_points);

	void apply_polygon(const double time, const ClipperLib::Paths &input_paths);

	void query_range(const glm::vec2 &query_begin, const glm::vec2 &query_end, std::vector<DestructibleMapChunk*> &leaves);

	DestructibleMapChunk *query_random();

	void set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree);

	void remove();

	void query_dirty(std::vector<DestructibleMapChunk*>& dirty_chunks);

	void update_batch(DestructibleMapDrawingBatch *batch, int batch_index, int batch_size);
	DestructibleMapDrawingBatch* get_batch()
	{
		return this->batch_;
	}
	int get_batch_index()
	{
		return this->batch_index_;
	}

	int get_batch_size()
	{
		return this->batch_size_;
	}

	friend DestructibleMap;
	friend DestructibleMapDrawingBatch;
};