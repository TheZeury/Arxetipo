// Arxetipo.cpp : Defines the entry point for the application.
//

#include "Arxetipo.h"
#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"
#include "engine/common_objects/common_objects.hpp"
#include "engine/renderer/graphics_objects.hpp"
#include "engine/command/command.hpp"

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
		transform.set_local_matrix(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 1.0f, -2.0f }));
		transform.set_parent(&origin);
		auto test_sphere = arx::MeshModelComponent{ renderer.defaults.sample_sphere, renderer.defaults.default_material, &transform };
		test_sphere.register_to_systems(&graphics_system);

		arx::SpaceTransform panel_transform;
		panel_transform.set_local_position({ 0.f, 1.f, -0.3f });
		//panel_transform.set_parent(&camera_offset_transform);
		arx::UIElement* ui_panel = renderer.create_ui_panel({ 0.3f, 0.1f });
		auto panel = arx::UIElementComponent{ ui_panel, renderer.defaults.white_bitmap, &panel_transform };
		panel.register_to_systems(&graphics_system);

		arx::SpaceTransform text_transform;
		text_transform.set_local_position({ 0.f, 0.f, 0.001f });
		text_transform.set_parent(&panel_transform);
		auto font = renderer.create_bitmap("C:\\Windows\\Fonts\\CascadiaMono.ttf", 1024, 1024, 150); // TODO, temperary, this font does not always exist.
		auto ui_text = renderer.create_ui_text("hello world", 0.05f, font);
		auto text = arx::UIElementComponent{ ui_text, font, &text_transform };
		text.register_to_systems(&graphics_system);

		graphics_system.mobilize();
		graphics_system.set_camera_offset_transform(&camera_offset_transform);

		arx::CommandRuntime runtime{ std::cin, std::cout };
		runtime.kernel.add_method("set", [&](const std::vector<arx::CommandValue>& arguments, arx::CommandValue& result) {
			if (arguments.size() != 3) {
				throw arx::CommandException{ "`set` requires 3 arguments." };
			}
			for (auto& argument : arguments) {
				if (argument.type != arx::CommandValue::Type::Number) {
					throw arx::CommandException{ "coordinates must be numbers." };
				}
			}
			origin.set_local_position({ std::get<float>(arguments[0].value), std::get<float>(arguments[1].value), std::get<float>(arguments[2].value) });
		}, true);

		// TODO. This approach is just for testing and example purpose, 
		// normally there shouldn't be multiple threads using vulkan in this way 
		// without sychronizing between threads and between CPU and GPU.
		runtime.kernel.add_method("say", [&](const std::vector<arx::CommandValue>& arguments, arx::CommandValue& result) { 
			if (arguments.size() != 1) {
				throw arx::CommandException{ "`say` requires 1 argument." };
			}
			auto& argument = arguments[0];
			if (argument.type != arx::CommandValue::Type::String) {
				throw arx::CommandException{ "argument must be a string." };
			}
			auto& words =std::get<std::string>(argument.value);
			graphics_system.remove_ui_element(ui_text, font, &text_transform);
			ui_text = renderer.create_ui_text(words, 0.05f, font);
			text = arx::UIElementComponent{ ui_text, font, &text_transform };
			text.register_to_systems(&graphics_system);
		}, true);

		std::jthread command_thread([&runtime]() {
			runtime.run();
		});

		auto start_time = std::chrono::high_resolution_clock::now();
		auto last_time = start_time;
		
		while (xr_plugin.poll_events()) {
			xr_plugin.poll_actions();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
			last_time = currentTime;

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
