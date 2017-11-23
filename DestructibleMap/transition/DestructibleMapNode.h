#pragma once
#include "Node.h"
#include "clipper.hpp"

class DestructibleMapNode :
	public Node
{
	glm::mat4 trafo_;
	glm::mat4 itrafo_;

	void load(ClipperLib::Paths poly_tree);
public:
	explicit DestructibleMapNode(const std::string& name);
	~DestructibleMapNode();

	void load_from_svg(const std::string& path);
	void load_sample();

	void init(RenderingEngine* rendering_engine) override;

	void apply_transformation(const glm::mat4& transformation, const glm::mat4& inverse_transformation) override;
};

