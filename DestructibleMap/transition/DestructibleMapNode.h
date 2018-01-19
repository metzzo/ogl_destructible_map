#pragma once
#include "clipper.hpp"
#include "DestructibleMapChunk.h"

#define SCALE_FACTOR (1000.0f)
#define SCALE_FACTOR_INV (1.0f/SCALE_FACTOR)

class ShaderResource;
ClipperLib::Path make_rect(const glm::ivec2 pos, const glm::ivec2 size);
ClipperLib::Path make_circle(const glm::ivec2 pos, const float radius, const int num_of_points);

class MeshResource;
class DestructibleMapNode
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;
	std::vector<glm::vec2> points_;
	std::vector<glm::vec2> lines_;
	DestructibleMapChunk quad_tree_;
	float triangle_area_ratio_;
	float points_per_leaf_ratio_;

	void load(ClipperLib::Paths poly_tree);
public:
	MeshResource* point_distribution_resource_;
	MeshResource* quadtree_resource_;

	explicit DestructibleMapNode(const std::string& name, float triangle_area_ratio = 0.025f, float points_per_leaf_ratio = 0.0005f);
	~DestructibleMapNode();

	void load_from_svg(const std::string& path);
	void load_sample();

	void init(RenderingEngine* rendering_engine);

	
	void draw(ShaderResource* shader) const;

	void apply_polygon_operation(const ClipperLib::Path polygon, ClipperLib::ClipType clip_type);

	void update_quadtree_representation();
};

