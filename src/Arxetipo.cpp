#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"
#include "engine/physics/physx_engine.hpp"
#include "engine/common_objects/common_objects.hpp"
#include "engine/renderer/graphics_objects.hpp"
#include "engine/xr/xr_objects.hpp"
#include "engine/physics/physics_objects.hpp"
#include "engine/command/command.hpp"

#include <chrono>

#include "scenes/demo_scene.hpp"

auto main() -> int {
	try {
		arx::VulkanRenderer renderer;
		arx::OpenXRPlugin xr_plugin(renderer);
		arx::PhysXEngine physics_engine;
		auto proxy = xr_plugin.initialize();
		renderer.initialize(proxy);
		physics_engine.initialize();
		xr_plugin.initialize_session(proxy);
		renderer.initialize_session(proxy);

		auto scene = new arx::DemoScene(&renderer, &xr_plugin, &physics_engine);
		scene->mobilize();

		arx::CommandRuntime runtime{ std::cin, std::cout };
		runtime.kernel.add_method("teleport", [&](const std::vector<arx::CommandValue>& arguments, arx::CommandValue& result) {
			if (arguments.size() != 3) {
				throw arx::CommandException{ "`teleport` requires 3 arguments. \nHint: `teleport` is used to set xr_offset. \nUsage: `teleport(x, y, z)`" };
			}
			for (auto& argument : arguments) {
				if (argument.type != arx::CommandValue::Type::Number) {
					throw arx::CommandException{ "coordinates must be numbers. \nHint: `teleport` is used to set xr_offset. \nUsage: `teleport(x, y, z)`" };
				}
			}
			scene->entities.xr_offset.set_local_position({ std::get<float>(arguments[0].value), std::get<float>(arguments[1].value), std::get<float>(arguments[2].value) });
		}, true);

		// TODO. This approach is just for testing and example purpose, 
		// normally there shouldn't be multiple threads using vulkan in this way 
		// without sychronizing between threads and between CPU and GPU.
		runtime.kernel.add_method("say", [&](const std::vector<arx::CommandValue>& arguments, arx::CommandValue& result) { 
			if (arguments.size() != 1) {
				throw arx::CommandException{ "`say` requires 1 argument." };
			}
			auto words = arguments[0].to_string();
			scene->systems.graphics_system.remove_ui_element(scene->entities.panel.text.ui_element.ui_element, scene->entities.panel.text.ui_element.bitmap, scene->entities.panel.text.ui_element.transform);
			scene->entities.panel.text.ui_element.ui_element = renderer.create_ui_text(words, 0.05f, scene->resources.font);
			scene->entities.panel.text.ui_element.register_to_systems(&scene->systems.graphics_system);
		}, true);

		runtime.kernel.add_method("move", [&](const std::vector<arx::CommandValue>& arguments, arx::CommandValue& result) {
			if (arguments.size() != 4) {
				throw arx::CommandException{ "`move` requires 4 arguments. \nHint: `move`, different from `teleport`, is used to move rigid actors. \nUsage: `move(\"name_of_an_object\", x, y, z)`" };
			}
			if (arguments[0].type != arx::CommandValue::Type::String) {
				throw arx::CommandException{ "first argument must be a string as a object's name. \nHint: `move`, different from `teleport`, is used to move rigid actors. \nUsage: `move(\"name_of_an_object\", x, y, z)`" };
			}
			for (size_t i = 1; i < arguments.size(); ++i) {
				if (arguments[i].type != arx::CommandValue::Type::Number) {
					throw arx::CommandException{ "coordinates must be numbers. \nHint: `move`, different from `teleport`, is used to move rigid actors. \nUsage: `move(\"name_of_an_object\", x, y, z)`" };
				}
			}
			auto name = std::get<std::string>(arguments[0].value);
			if (name != "test_sphere") {
				throw arx::CommandException{ "object not found." };
			}
			scene->entities.test_sphere.rigid_dynamic.rigid_dynamic->setGlobalPose({ std::get<float>(arguments[1].value), std::get<float>(arguments[2].value), std::get<float>(arguments[3].value) });
		}, true);

		std::jthread command_thread([&runtime]() {
			runtime.run();
		});

		auto start_time = std::chrono::high_resolution_clock::now();
		auto last_time = start_time;
		
		while (xr_plugin.poll_events()) {
			scene->systems.xr_system.update();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
			last_time = currentTime;

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
