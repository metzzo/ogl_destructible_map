#include "DestructibleMapController.h"
#include <GLFW/glfw3.h>
#include "RenderingEngine.h"
#include "DestructibleMap.h"
#include <glm/gtc/matrix_transform.hpp>

DestructibleMapController::DestructibleMapController(DestructibleMap *map) 
{
	this->map_ = map;
	this->highlighted_chunk_ = nullptr;
}

DestructibleMapController::~DestructibleMapController()
{
}

void DestructibleMapController::update(double delta)
{
	const auto vp = this->rendering_engine_->get_viewport();
	double xpos, ypos;
	glfwGetCursorPos(this->rendering_engine_->get_window(), &xpos, &ypos);

	const auto x = (2.0f * xpos) / vp.x - 1.0f;
	const auto y = 1.0f - (2.0f * ypos) / vp.y;
	const auto z = 1.0f;
	const auto ray_nds = glm::vec3(x, y, z);
	const auto ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);
	auto ray_eye = this->proj_inverse_ * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	const auto itrafo = this->rendering_engine_->get_view_matrix();
	const auto trafo = glm::inverse(itrafo);
	const auto ray_world = glm::normalize(glm::vec3(trafo*ray_eye));
	const auto origin = glm::vec3(trafo[3]);
	const auto plane_normal = glm::vec3(0, 0, -1);
	const auto t = -(glm::dot(origin, plane_normal) + 1.0f) / (glm::dot(ray_world, plane_normal));
	if (t >= 0.000001)
	{
		const auto pick_pos = origin + ray_world*t;
		const auto b1 = glfwGetMouseButton(this->rendering_engine_->get_window(), GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
		const auto b2 = glfwGetMouseButton(this->rendering_engine_->get_window(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS;
		if (b1 || b2) {
			//std::cout << "Picked " << pick_pos.x << " " << pick_pos.y << " " << pick_pos.z << std::endl;
			const auto circle = make_circle(glm::ivec2(pick_pos.x * SCALE_FACTOR, pick_pos.y * SCALE_FACTOR), 10 * SCALE_FACTOR, 16);
			map_->apply_polygon_operation(circle, b1 ? ClipperLib::ctDifference : ClipperLib::ctUnion);
		}

		if (this->highlighted_chunk_)
		{
			this->highlighted_chunk_->set_highlighted(false);
		}
		this->highlighted_chunk_ = map_->get_root_chunk()->query_chunk(pick_pos);
		if (this->highlighted_chunk_)
		{
			this->highlighted_chunk_->set_highlighted(true);
		}
	}

	const auto sx = glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_A) - glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_D);
	const auto sy = glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_S) - glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_W);
	const auto zoom = glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_Q) - glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_E);

	const auto translation_trafo = glm::translate(glm::mat4(), 
		glm::vec3(sx, sy, zoom) * 
		200.0f * 
		float(delta) * 
		(
			1.0f + 10.0f * 
			glfwGetKey(this->rendering_engine_->get_window(), GLFW_KEY_LEFT_SHIFT)
		)
	);
	this->rendering_engine_->set_view_matrix(itrafo * translation_trafo);
}

void DestructibleMapController::init(RenderingEngine* rendering_engine)
{
	this->rendering_engine_ = rendering_engine;

	this->proj_inverse_ = glm::inverse(rendering_engine->get_projection_matrix());
}
