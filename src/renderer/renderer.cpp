#include "pch.h"
#include "renderer/renderer.h"

void engine::renderer::init()
{
	// initialize Vulkan API using GLFW
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(RE_DEFAULTWINDOWWIDTH, RE_DEFAULTWINDOWHEIGHT, "Radium Engine", nullptr, nullptr);

	uint32_t extensionCount = 0;
	//vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::cout << "Number of extensions is " << extensionCount << "\n";

	getchar();
}
