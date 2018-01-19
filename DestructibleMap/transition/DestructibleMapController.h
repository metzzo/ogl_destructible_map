#pragma once
#include <glm/matrix.hpp>
#include <string>
class RenderingEngine;
class DestructibleMapNode;

class DestructibleMapController
{
	glm::mat4 proj_inverse_;
	DestructibleMapNode* map_;
	RenderingEngine* rendering_engine_;
public:
	explicit DestructibleMapController(const std::string &name, DestructibleMapNode *map_);
	~DestructibleMapController();

	void update(double delta);
	void init(RenderingEngine* rendering_engine);
};

