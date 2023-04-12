// Arxetipo.cpp : Defines the entry point for the application.
//

#include "Arxetipo.h"
#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"

//using namespace std;

int main()
{
	arx::VulkanRenderer renderer;
	arx::OpenXRPlugin xr_plugin(renderer);
	auto proxy = xr_plugin.initialize();
	renderer.initialize(proxy);
	return 0;
}
