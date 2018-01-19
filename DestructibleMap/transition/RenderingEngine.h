#pragma once
#include "GLDebugContext.h"
#include <glm/matrix.hpp>
#include "DestructibleMapChunk.h"
#include <GLFW/glfw3.h>

class MainShader;

class RenderingEngine
{
	glm::ivec2 viewport_;
	bool fullscreen_;
	int refresh_rate_;

	GLFWwindow* window_;

	glm::mat4 projection_matrix_;
	glm::mat4 view_matrix_;
public:
	explicit RenderingEngine::RenderingEngine(const glm::ivec2 viewport, bool fullscreen, int refresh_rate);
	~RenderingEngine();

	void run();

	const glm::ivec2& get_viewport() const
	{
		return viewport_;
	}

	GLFWwindow* get_window() const;

	void set_camera(glm::mat4 projection_matrix, glm::mat4 view_matrix);

	const glm::mat4 &get_projection_matrix() const
	{
		return this->projection_matrix_;
	}

	const glm::mat4 &get_view_matrix() const
	{
		return this->view_matrix_;
	}

	void set_view_matrix(const glm::mat4 &trafo) 
	{
		this->view_matrix_ = trafo;
	}
};

