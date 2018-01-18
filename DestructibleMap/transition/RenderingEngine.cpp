#include "RenderingEngine.h"
#include "RenderingNode.h"
#include "glheaders.h"
#include <iostream>
#include "GroupNode.h"
#include "IResource.h"
#include "MainShader.h"
#include "GLDebugContext.h"
#include "AnimatorNode.h"

RenderingEngine::RenderingEngine(const glm::ivec2 viewport, bool fullscreen, int refresh_rate)
{
	this->root_node_ = new GroupNode("root");
	this->viewport_ = viewport;
	this->fullscreen_ = fullscreen;
	this->refresh_rate_ = refresh_rate;

	this->main_shader_ = new MainShader();
	this->register_resource(this->main_shader_);

	this->window_ = nullptr;
}

RenderingEngine::~RenderingEngine()
{
	delete this->root_node_;
}

void RenderingEngine::register_resource(IResource* resource)
{
	this->resources_.push_back(resource);
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

	for (auto& resource : resources_)
	{
		resource->init();
	}

	this->root_node_->init(this);

	this->drawables_ = this->root_node_->get_drawables();
	this->rendering_nodes_ = this->root_node_->get_rendering_nodes();
	this->light_nodes_ = this->root_node_->get_light_nodes();
	this->animator_nodes_ = this->root_node_->get_animator_nodes();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	double last_time = glfwGetTime();
	double last_fps_show = last_time;
	while (!glfwWindowShouldClose(this->window_))
	{
		const double current_time = glfwGetTime();
		const double delta = current_time - last_time;
		if (current_time - last_fps_show > 0.5) {
			std::cout << "FPS: " << 1 / delta << " Draw Calls (map chunk): " << map_chunks_drawn << std::endl;
			last_fps_show = current_time;
		}
		last_time = current_time;

		if (glfwGetKey(this->window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(this->window_, true);
		}

		for (auto& animator_node : this->animator_nodes_)
		{
			animator_node->update(delta);
		}

		for (auto& rendering_node : this->rendering_nodes_)
		{
			rendering_node->render(this->drawables_, this->light_nodes_);
		}

		glfwSwapBuffers(this->window_);
		glfwPollEvents();
	}
	glfwTerminate();

	for (auto& resource : resources_) {
		delete resource;
	}
}

GLFWwindow* RenderingEngine::get_window() const
{
	return this->window_;
}
