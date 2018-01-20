#pragma once

#include "clipper.hpp"
#include <glm/glm.hpp>
#include "DestructibleMapDrawingBatch.h"

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

	std::vector<BatchInfo *> batch_infos_;

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

	void add_batch_info(BatchInfo *info);
	void clear_batch_infos();
	BatchInfo* get_batch_info(int index)
	{
		return this->batch_infos_[index];
	}
	int get_batch_info_count()
	{
		return this->batch_infos_.size();
	}

	friend DestructibleMap;
	friend DestructibleMapDrawingBatch;
};