#include "RenderingEngine.h"
#include "GLDebugContext.h"
#include <iostream>
#include "DestructibleMapChunk.h"
#include "DestructibleMapController.h"
#include "DestructibleMap.h"

RenderingEngine::RenderingEngine(const glm::ivec2 viewport, bool fullscreen, int refresh_rate)
{
	this->viewport_ = viewport;
	this->fullscreen_ = fullscreen;
	this->refresh_rate_ = refresh_rate;

	this->window_ = nullptr;
}

RenderingEngine::~RenderingEngine()
{
}

void error_callback(int error, const char* description)
{
	std::cout << "Error: " << std::string(description) << std::endl;
}

void RenderingEngine::run()
{
	glfwSetErrorCallback(error_callback);

	glfwInit();

#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = nullptr;
	if (fullscreen_) {
		monitor = glfwGetPrimaryMonitor();
		glfwWindowHint(GLFW_REFRESH_RATE, refresh_rate_);
	}

	this->window_ = glfwCreateWindow(this->viewport_.x, this->viewport_.y, "Destructible Map", monitor, nullptr);
	if (this->window_ == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(this->window_);


	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return;
	}

	glfwSwapInterval(0);

	//Set DebugContext Callback
#if _DEBUG
	// Query the OpenGL function to register your callback function.
	const PFNGLDEBUGMESSAGECALLBACKPROC _glDebugMessageCallback = PFNGLDEBUGMESSAGECALLBACKPROC(glfwGetProcAddress("glDebugMessageCallback"));

	// Register your callback function.
	if (_glDebugMessageCallback != nullptr) {
		_glDebugMessageCallback(DebugCallback, nullptr);
	}

	// Enable synchronous callback. This ensures that your callback function is called
	// right after an error has occurred. 
	if (_glDebugMessageCallback != nullptr) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}
#endif

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	auto map = new DestructibleMap(0.001f, 0.01f);
	map->generate_map();
	map->init(this);

	auto controller = new DestructibleMapController(map);
	controller->init(this);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	double last_time = glfwGetTime();
	double last_fps_show = last_time;

	double total_fps = 0;
	int fps_count = 0;

	while (!glfwWindowShouldClose(this->window_))
	{
		const double current_time = glfwGetTime();
		const double delta = current_time - last_time;
		total_fps += 1 / delta;
		fps_count++;

		if (current_time - last_fps_show > 0.5) {
			std::cout << "Avg FPS: " << (total_fps / fps_count) << " Draw Calls: " << map_draw_calls << std::endl;
			last_fps_show = current_time;
			fps_count = 0;
			total_fps = 0;
		}
		last_time = current_time;

		if (glfwGetKey(this->window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(this->window_, true);
		}

		controller->update(delta);

		glViewport(0, 0, this->viewport_.x, this->viewport_.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		map->draw();

		glfwSwapBuffers(this->window_);
		glfwPollEvents();
	}
	glfwTerminate();

	delete map;
	delete controller;
}

GLFWwindow* RenderingEngine::get_window() const
{
	return this->window_;
}

void RenderingEngine::set_camera( glm::mat4 projection_matrix, glm::mat4 view_matrix)
{
	this->projection_matrix_ = projection_matrix;
	this->view_matrix_ = view_matrix;
}
