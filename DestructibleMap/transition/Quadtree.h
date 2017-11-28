#pragma once

#include "clipper.hpp"
#include <glm/glm.hpp>

class DestructibleMapNode;
class MeshResource;
class RenderingEngine;

class Quadtree
{
	std::vector<glm::vec2> points_;
	glm::vec2 begin_;
	glm::vec2 end_;

	Quadtree *parent_;

	Quadtree *north_west_;
	Quadtree *north_east_;
	Quadtree *south_west_;
	Quadtree *south_east_;

	ClipperLib::Paths paths_;
	std::vector<glm::vec2> vertices_;
	MeshResource *mesh_;
public:
	explicit Quadtree(Quadtree *parent, const glm::vec2 begin, const glm::vec2 end);
	Quadtree();
	~Quadtree();

	void get_lines(std::vector<glm::vec2>& lines) const;

	bool insert(const glm::vec2& point, const int max_points);

	void apply_polygon(const ClipperLib::Paths &input_paths);

	void query_range(const glm::vec2 &query_begin, const glm::vec2 &query_end, std::vector<Quadtree*> &leaves);

	void set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree);

	void simplify();

	void draw() const;
	void init(RenderingEngine *engine);

	void remove();

	friend DestructibleMapNode;
};