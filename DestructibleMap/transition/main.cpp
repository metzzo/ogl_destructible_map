#include <iostream>
#include "RenderingEngine.h"
#include "CameraNode.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GroupNode.h"
#include "ColladaImporter.h"
#include "LightNode.h"
#include "DestructibleMapNode.h"
#include "AnimatorNode.h"
#include "DestructibleMapController.h"

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
		glm::perspective(glm::radians(60.0f), float(WINDOW_WIDTH) / float(WINDOW_HEIGHT), 0.1f, 1000.0f)		
	);
	cam->set_view_matrix(glm::lookAt(glm::vec3(0, 0, 50), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
	root->add_node(cam);
	
	auto map = new DestructibleMapNode("map",0.001f, 0.01f);
	map->load_sample();
	root->add_node(map);

	auto controller = new DestructibleMapController("map controller", cam, map);
	root->add_node(controller);

	engine->run();

	delete engine;

    return 0;
}

