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

		arx::GraphicsSystem graphics_system(&renderer, &xr_plugin);
		arx::SpaceTransform origin;
		arx::SpaceTransform transform;
		arx::SpaceTransform camera_offset_transform;
		bool camera_offset_set = false;
		transform.set_local_matrix(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 1.0f, -2.0f }));
		transform.set_parent(&origin);
		camera_offset_transform.set_global_position(glm::vec3{ 0.0f, -1.f, 1.0f });
		auto test_sphere = arx::MeshModelComponent{ renderer.defaults.sample_sphere, renderer.defaults.default_material, &transform };
		test_sphere.register_to_systems(&graphics_system);
		graphics_system.mobilize();

		auto start_time = std::chrono::high_resolution_clock::now();
		auto last_time = start_time;
		
		while (xr_plugin.poll_events()) {
			xr_plugin.poll_actions();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
			last_time = currentTime;
			
			if (time_elapsed > 10.f && !camera_offset_set) {
				graphics_system.set_camera_offset_transform(&camera_offset_transform);
				camera_offset_set = true;
			}

			origin.set_local_rotation(glm::angleAxis(time_elapsed * glm::pi<float>() / 4.f, glm::vec3{ 0.0f, 1.0f, 0.0f }));

			graphics_system.update();
		}
	}
	catch (const std::exception& e) {
		arx::log_failure();
		arx::log_error(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
