#include <iostream>
#include "RenderingEngine.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DestructibleMapNode.h"
#include "DestructibleMapController.h"

int main()
{
	const int WINDOW_WIDTH = 1600;
	const int WINDOW_HEIGHT = 900;
	const bool WINDOW_FULLSCREEN = false;
	const int REFRESH_RATE = 60;

	auto engine = new RenderingEngine(glm::ivec2(WINDOW_WIDTH, WINDOW_HEIGHT), WINDOW_FULLSCREEN, REFRESH_RATE);
	engine->set_camera(
		glm::perspective(glm::radians(60.0f), float(WINDOW_WIDTH) / float(WINDOW_HEIGHT), 0.1f, 5000.0f),
		glm::lookAt(glm::vec3(0, 0, 100), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))
	);
	
	

	engine->run();

	delete engine;

    return 0;
}

