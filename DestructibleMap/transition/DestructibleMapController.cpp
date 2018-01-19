#include "DestructibleMapController.h"
#include <GLFW/glfw3.h>
#include "RenderingEngine.h"
#include "CameraNode.h"
#include "DestructibleMapNode.h"

DestructibleMapController::DestructibleMapController(const std::string& name, CameraNode *cam, DestructibleMapNode *map) : AnimatorNode(name)
{
	this->cam_ = cam;
	this->map_ = map;
	this->proj_inverse_ = glm::inverse(cam->get_projection_matrix());
}

DestructibleMapController::~DestructibleMapController()
{
}

void DestructibleMapController::update(double delta)
{
	auto vp = this->get_rendering_engine()->get_viewport();
	double xpos, ypos;
	glfwGetCursorPos(this->get_rendering_engine()->get_window(), &xpos, &ypos);

	float x = (2.0f * xpos) / vp.x - 1.0f;
	float y = 1.0f - (2.0f * ypos) / vp.y;
	float z = 1.0f;
	glm::vec3 ray_nds = glm::vec3(x, y, z);
	glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);
	glm::vec4 ray_eye = this->proj_inverse_ * ray_clip;
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

	auto ray_world = glm::normalize(glm::vec3(glm::inverse(this->cam_->get_view_matrix())*ray_eye));
	auto origin = cam_->get_position();
	auto plane_normal = glm::vec3(0, 0, -1);
	auto t = -(glm::dot(origin, plane_normal) + 1.0f) / (glm::dot(ray_world, plane_normal));
	if (t >= 0.000001)
	{
		auto pick_pos = origin + ray_world*t;
		auto b1 = glfwGetMouseButton(this->get_rendering_engine()->get_window(), GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
		auto b2 = glfwGetMouseButton(this->get_rendering_engine()->get_window(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS;
		if (b1 || b2) {
			//std::cout << "Picked " << pick_pos.x << " " << pick_pos.y << " " << pick_pos.z << std::endl;
			auto circle = make_circle(glm::ivec2(pick_pos.x * SCALE_FACTOR, pick_pos.y * SCALE_FACTOR), 10 * SCALE_FACTOR, 16);
			map_->apply_polygon_operation(circle, b1 ? ClipperLib::ctDifference : ClipperLib::ctUnion);
		}
	}

	auto sx = glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_A) - glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_D);
	auto sy = glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_S) - glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_W);
	auto zoom = glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_Q) - glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_E);

	auto trafo = Transformation::translate(-glm::vec3(sx, sy, zoom) * 200.0f * float(delta) * (1.0f + 10.0f*glfwGetKey(this->get_rendering_engine()->get_window(), GLFW_KEY_LEFT_SHIFT)));

	this->cam_->apply_transformation(trafo);
}
