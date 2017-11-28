#include <iostream>
#include "RenderingEngine.h"
#include "CameraNode.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GroupNode.h"
#include "ColladaImporter.h"
#include "LightNode.h"
#include "DestructibleMapNode.h"

int main()
{
	const int WINDOW_WIDTH = 1600;
	const int WINDOW_HEIGHT = 900;
	const bool WINDOW_FULLSCREEN = false;
	const int REFRESH_RATE = 60;

	auto engine = new RenderingEngine(glm::ivec2(WINDOW_WIDTH, WINDOW_HEIGHT), WINDOW_FULLSCREEN, REFRESH_RATE);
	auto root = engine->get_root_node();

	const auto cam = new CameraNode("MainCamera",
		engine->get_viewport(),
		glm::perspective(glm::radians(60.0f), float(WINDOW_WIDTH) / float(WINDOW_HEIGHT), 0.1f, 100.0f)		
	);
	cam->set_view_matrix(glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
	root->add_node(cam);
	
	auto map = new DestructibleMapNode("map");
	auto m = glm::scale(glm::mat4x4(), glm::vec3(0.025, 0.025, 0.0));
	map->set_transformation(m);
	map->load_sample();
	map->remove_rect(glm::vec2(8, 8), glm::vec2(18, 18));
	root->add_node(map);

	engine->run();

	delete engine;

    return 0;
}

