#pragma once
#include "clipper.hpp"
#include "IDrawable.h"
#include "DestructibleMapChunk.h"
#include "TransformationNode.h"

class MeshResource;
class DestructibleMapNode :
	public IDrawable,
	public TransformationNode
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;
	std::vector<glm::vec2> points_;
	std::vector<glm::vec2> lines_;
	DestructibleMapChunk quad_tree_; 

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
	void remove(const ClipperLib::Path polygon);

	void update_quadtree_representation();
};

