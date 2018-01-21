#pragma once
#include "clipper.hpp"
#include "DestructibleMapChunk.h"
#include "DestructibleMapConfiguration.h"


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

	explicit DestructibleMap(float triangle_area_ratio = MAP_TRIANGLE_AREA_RATIO, float points_per_leaf_ratio = MAP_POINTS_PER_LEAF_RATIO);
	~DestructibleMap();

	void generate_map(int num_rects = GENERATE_NUM_RECTS, int num_circle = GENERATE_NUM_CIRCLES, int width = GENERATE_WIDTH, int height = GENERATE_HEIGHT, int min_size = GENERATE_MIN_SIZE, int max_size = GENERATE_MAX_SIZE);

	void init(RenderingEngine* rendering_engine);

	void draw();

	void apply_polygon_operation(const ClipperLib::Path polygon, ClipperLib::ClipType clip_type);

	void update_quadtree_representation();

	DestructibleMapChunk *get_root_chunk()
	{
		return &this->quad_tree_;
	}
};

