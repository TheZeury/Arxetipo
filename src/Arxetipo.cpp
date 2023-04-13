// Arxetipo.cpp : Defines the entry point for the application.
//

#include "Arxetipo.h"
#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"
#include "engine/common_objects/common_objects.hpp"
#include "engine/renderer/graphics_objects.hpp"

#include <chrono>

int main() {
	try {
		arx::VulkanRenderer renderer;
		arx::OpenXRPlugin xr_plugin(renderer);
		auto proxy = xr_plugin.initialize();
		renderer.initialize(proxy);
		xr_plugin.initialize_session(proxy);
		renderer.initialize_session(proxy);

		arx::GraphicsSystem graphics_system(&renderer);
		arx::SpaceTransform origin;
		arx::SpaceTransform transform;
		transform.set_local_matrix(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 1.0f, -2.0f }));
		transform.set_parent(&origin);
		auto test_sphere = arx::MeshModelComponent{ renderer.defaults.sample_sphere, renderer.defaults.default_material, &transform };
		test_sphere.register_to_systems(&graphics_system);
		graphics_system.mobilize();

		auto start_time = std::chrono::high_resolution_clock::now();
		
		while (xr_plugin.poll_events()) {
			xr_plugin.poll_actions();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			
			//origin.set_local_matrix(glm::rotate(glm::mat4{ 1.0f }, time_delta * glm::pi<float>() / 4.f, glm::vec3{0.0f, 1.0f, 0.0f}));
			origin.set_local_rotation(glm::angleAxis(time_delta * glm::pi<float>() / 4.f, glm::vec3{ 0.0f, 1.0f, 0.0f }));

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
