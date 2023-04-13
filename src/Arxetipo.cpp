// Arxetipo.cpp : Defines the entry point for the application.
//

#include "Arxetipo.h"
#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"

int main() {
	try {
		arx::VulkanRenderer renderer;
		arx::OpenXRPlugin xr_plugin(renderer);
		auto proxy = xr_plugin.initialize();
		renderer.initialize(proxy);
		xr_plugin.initialize_session(proxy);
		renderer.initialize_session(proxy);

		while (xr_plugin.poll_events()) {
			xr_plugin.poll_actions();
			xr_plugin.update();
		}
	}
	catch (const std::exception& e) {
		arx::log_failure();
		arx::log_error(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
