#pragma once
#include <glm/matrix.hpp>

class RenderingEngine;
class DestructibleMap;
class DestructibleMapChunk;

class DestructibleMapController
{
	glm::mat4 proj_inverse_;
	DestructibleMap* map_;
	RenderingEngine* rendering_engine_;
	DestructibleMapChunk *highlighted_chunk_;
public:
	explicit DestructibleMapController(DestructibleMap *map_);
	~DestructibleMapController();

	void update(double delta);
	void init(RenderingEngine* rendering_engine);
};

