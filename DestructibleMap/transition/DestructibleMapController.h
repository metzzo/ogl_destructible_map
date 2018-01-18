#pragma once
#include "AnimatorNode.h"

class DestructibleMapNode;
class CameraNode;

class DestructibleMapController :
	public AnimatorNode
{
	CameraNode* cam_;
	glm::mat4 proj_inverse_;
	DestructibleMapNode* map_;
public:
	explicit DestructibleMapController(const std::string &name, CameraNode *cam, DestructibleMapNode *map_);
	~DestructibleMapController();

	void update(double delta) override;
};

