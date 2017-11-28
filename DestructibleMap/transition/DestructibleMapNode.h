#pragma once
#include "clipper.hpp"
#include "TransformationNode.h"
#include "IDrawable.h"
#include "MeshResource.h"

class DestructibleMapNode;

class Quadtree
{
	int max_points_;
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
	explicit Quadtree(Quadtree *parent, const int max_points, const glm::vec2 begin, const glm::vec2 end);
	Quadtree();
	~Quadtree();

	void get_lines(std::vector<glm::vec2>& lines) const;

	bool insert(const glm::vec2& point);
	
	void apply_polygon(const ClipperLib::Paths &input_paths);

	void query_range(const glm::vec2 &query_begin, const glm::vec2 &query_end, std::vector<Quadtree*> &leaves);

	void set_paths(const ClipperLib::Paths &paths, const ClipperLib::PolyTree &poly_tree);

	void simplify();

	void draw() const;
	void init(RenderingEngine *engine);

	void remove();

	friend DestructibleMapNode;
};

class MeshResource;
class DestructibleMapNode :
	public TransformationNode,
	public IDrawable
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;
	std::vector<glm::vec2> points_;
	std::vector<glm::vec2> lines_;
	Quadtree quad_tree_;

	void load(ClipperLib::Paths poly_tree);
public:
	MeshResource* total_map_resource_;
	MeshResource* point_distribution_resource_;
	MeshResource* quadtree_resource_;

	explicit DestructibleMapNode(const std::string& name);
	~DestructibleMapNode();

	void load_from_svg(const std::string& path);
	void load_sample();

	void init(RenderingEngine* rendering_engine) override;

	std::vector<IDrawable*> get_drawables() override;
	
	void draw(ShaderResource* shader) const override;
	void remove_rect(const glm::vec2 &begin, const glm::vec2 &end);

	void update_quadtree_representation();
};

