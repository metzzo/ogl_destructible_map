#pragma once

#include "clipper.hpp"
#include <glm/glm.hpp>
#include "DestructibleMapDrawingBatch.h"
#include "DestructibleMapController.h"

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

	BatchInfo *batch_info_;
	bool highlighted_;
	ClipperLib::Path quad_;
	int mergeable_count_;

	void constructor();
public:

	explicit DestructibleMapChunk(DestructibleMapChunk *parent, const glm::vec2 begin, const glm::vec2 end);
	DestructibleMapChunk();
	~DestructibleMapChunk();

	void get_lines(std::vector<glm::vec2>& lines) const;

	bool insert(const glm::vec2& point, const int max_points);

	void subdivide();
	void merge();

	void apply_polygon(const ClipperLib::Paths &input_paths);

	void query_range(const glm::vec2 &query_begin, const glm::vec2 &query_end, std::vector<DestructibleMapChunk*> &leaves);

	DestructibleMapChunk *query_chunk(glm::vec2 point);

	void set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree);

	void query_dirty(std::vector<DestructibleMapChunk*>& dirty_chunks);

	void update_batch(BatchInfo *info);
	BatchInfo* get_batch_info()
	{
		return this->batch_info_;
	}

	ClipperLib::Paths& get_paths()
	{
		return paths_;
	}


	void set_highlighted(bool highlight)
	{
		this->highlighted_ = highlight;
	}

	DestructibleMapChunk *get_best_mergeable() const;


	friend DestructibleMap;
	friend DestructibleMapDrawingBatch;
};