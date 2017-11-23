#pragma once
#include "clipper.hpp"
#include "TransformationNode.h"
#include "IDrawable.h"
#include "MeshResource.h"

class MeshResource;
class DestructibleMapNode :
	public TransformationNode,
	public IDrawable
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;
	std::vector<glm::vec2> vertices_;

	void load(ClipperLib::Paths poly_tree);
public:
	MeshResource* total_map_resource_;

	explicit DestructibleMapNode(const std::string& name);
	~DestructibleMapNode();

	void load_from_svg(const std::string& path);
	void load_sample();

	void init(RenderingEngine* rendering_engine) override;

	std::vector<IDrawable*> get_drawables() override;
	
	void draw(ShaderResource* shader) const override;
};

