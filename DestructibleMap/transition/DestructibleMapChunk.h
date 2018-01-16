#pragma once

#include "clipper.hpp"
#include <glm/glm.hpp>

class DestructibleMapNode;
class MeshResource;
class RenderingEngine;

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
	MeshResource *mesh_;
public:
	explicit DestructibleMapChunk(DestructibleMapChunk *parent, const glm::vec2 begin, const glm::vec2 end);
	DestructibleMapChunk();
	~DestructibleMapChunk();

	void get_lines(std::vector<glm::vec2>& lines) const;

	bool insert(const glm::vec2& point, const int max_points);

	void apply_polygon(const ClipperLib::Paths &input_paths);

	void query_range(const glm::vec2 &query_begin, const glm::vec2 &query_end, std::vector<DestructibleMapChunk*> &leaves);

	void set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree);

	void simplify();

	void draw() const;
	void init(RenderingEngine *engine);

	void remove();

	friend DestructibleMapNode;
};