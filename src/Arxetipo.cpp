#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"
#include "engine/physics/physx_engine.hpp"
#include "engine/command/command.hpp"
#include "engine/common_objects/common_objects.hpp"
#include "engine/renderer/graphics_objects.hpp"
#include "engine/xr/xr_objects.hpp"
#include "engine/physics/physics_objects.hpp"
#include "engine/ui/ui_objects.hpp"
#include "engine/command/command_objects.hpp"

#include <chrono>

#include "scenes/demo_scene.hpp"

auto main() -> int {
	try {
		arx::VulkanRenderer renderer;
		arx::OpenXRPlugin xr_plugin(renderer);
		arx::PhysXEngine physics_engine;
		arx::CommandRuntime runtime{ std::cin, std::cout };
		auto proxy = xr_plugin.initialize();
		renderer.initialize(proxy);
		physics_engine.initialize();
		xr_plugin.initialize_session(proxy);
		renderer.initialize_session(proxy);

		auto scene = new arx::DemoScene(&renderer, &xr_plugin, &physics_engine, &runtime);
		scene->mobilize();

		auto start_time = std::chrono::high_resolution_clock::now();
		auto last_time = start_time;
		
		while (xr_plugin.poll_events()) {
			scene->systems.xr_system.update();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
			last_time = currentTime;

			scene->systems.command_system.update();

			scene->systems.physics_system.set_time_delta(time_delta);
			scene->systems.physics_system.update();

			// origin.set_local_rotation(glm::angleAxis(time_elapsed * glm::pi<float>() / 4.f, glm::vec3{ 0.0f, 1.0f, 0.0f }));

			scene->systems.graphics_system.update();
		}
	}
	catch (const std::exception& e) {
		arx::log_failure();
		arx::log_error(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
