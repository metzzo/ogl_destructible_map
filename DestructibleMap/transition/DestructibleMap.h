#pragma once
#include "clipper.hpp"
#include "DestructibleMapChunk.h"

#define SCALE_FACTOR (1000.0f)
#define SCALE_FACTOR_INV (1.0f/SCALE_FACTOR)
#define TRIANGULATION_BUFFER (VERTICES_PER_CHUNK*3)


class DestructibleMapShader;
class MeshResource;

ClipperLib::Path make_rect(const glm::ivec2 pos, const glm::ivec2 size);
ClipperLib::Path make_circle(const glm::ivec2 pos, const float radius, const int num_of_points);

class DestructibleMap
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;
	std::vector<glm::vec2> points_;
	std::vector<glm::vec2> lines_;
	DestructibleMapChunk quad_tree_;
	float triangle_area_ratio_;
	float points_per_leaf_ratio_;
	DestructibleMapShader* map_shader_;

	MeshResource* point_distribution_resource_;
	MeshResource* quadtree_resource_;
	RenderingEngine* rendering_engine_;

	std::vector<DestructibleMapDrawingBatch*> batches_;

	void load(ClipperLib::Paths poly_tree);
	void update_batches();
public:

	explicit DestructibleMap(float triangle_area_ratio = 0.025f, float points_per_leaf_ratio = 0.0005f);
	~DestructibleMap();

	void load_sample();

	void init(RenderingEngine* rendering_engine);

	void draw();

	void apply_polygon_operation(const ClipperLib::Path polygon, ClipperLib::ClipType clip_type);

	void update_quadtree_representation();

	DestructibleMapChunk *get_root_chunk()
	{
		return &this->quad_tree_;
	}
};

