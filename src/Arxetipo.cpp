// Arxetipo.cpp : Defines the entry point for the application.
//

#include "Arxetipo.h"
#include "engine/renderer/vulkan_renderer.hpp"
#include "engine/xr/openxr_plugin.hpp"
#include "engine/physics/physx_engine.hpp"
#include "engine/common_objects/common_objects.hpp"
#include "engine/renderer/graphics_objects.hpp"
#include "engine/xr/xr_objects.hpp"
#include "engine/physics/physics_objects.hpp"
#include "engine/command/command.hpp"

#include <chrono>

int main() {
	try {
		arx::VulkanRenderer renderer;
		arx::OpenXRPlugin xr_plugin(renderer);
		arx::PhysXEngine physics_engine;
		auto proxy = xr_plugin.initialize();
		renderer.initialize(proxy);
		physics_engine.initialize();
		xr_plugin.initialize_session(proxy);
		renderer.initialize_session(proxy);

		arx::GraphicsSystem graphics_system(&renderer, &xr_plugin);
		arx::SpaceTransform origin;
		arx::SpaceTransform transform;
		arx::SpaceTransform camera_offset_transform;
		arx::SpaceTransform left_controller_transform;
		arx::SpaceTransform right_controller_transform;
		left_controller_transform.set_parent(&camera_offset_transform);
		right_controller_transform.set_parent(&camera_offset_transform);

		transform.set_local_matrix(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 10.0f, -2.0f }));
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

		arx::XRSystem xr_system(&xr_plugin);
		arx::XRController left_controller{ 0, &left_controller_transform };
		arx::XRController right_controller{ 1, &right_controller_transform };
		left_controller.register_to_systems(&xr_system);
		right_controller.register_to_systems(&xr_system);

		auto cone_model = renderer.create_mesh_model(arx::MeshBuilder::Cone(0.01f, 0.05f, 0.1f, 16));

		auto left_cone = arx::MeshModelComponent{ cone_model, renderer.defaults.default_material, &left_controller_transform };
		auto right_cone = arx::MeshModelComponent{ cone_model, renderer.defaults.default_material, &right_controller_transform };
		right_cone.register_to_systems(&graphics_system);
		left_cone.register_to_systems(&graphics_system);

		arx::PhysicsSystem physics_system(&physics_engine);
		auto dynamic = physics_engine.create_rigid_dynamic(
			transform.get_global_matrix(),
			physics_engine.create_shape(arx::PhysicsSphereGeometry(0.2f))
		);
		auto dynamic_component = arx::RigidDynamicComponent{ dynamic, &transform };
		dynamic_component.register_to_systems(&physics_system);

		auto static_plane = physics_engine.create_rigid_static(
			origin.get_global_matrix(),
			physics_engine.create_shape(arx::PhysicsPlaneGeometry(), glm::rotate(glm::mat4{ 1.f }, 0.5f * glm::pi<float>(), { 0.f, 0.f, 1.f }))
		);
		auto plane_component = arx::RigidStaticComponent{ static_plane, &origin };
		plane_component.register_to_systems(&physics_system);

		physics_system.mobilize();

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
			auto words = arguments[0].to_string();
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
			xr_system.update();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time_elapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - start_time).count();
			float time_delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - last_time).count();
			last_time = currentTime;

			physics_system.set_time_delta(time_delta);
			physics_system.update();

			// origin.set_local_rotation(glm::angleAxis(time_elapsed * glm::pi<float>() / 4.f, glm::vec3{ 0.0f, 1.0f, 0.0f }));

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
