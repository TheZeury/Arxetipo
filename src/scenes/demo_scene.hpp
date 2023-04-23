#pragma once

namespace arx
{
	struct DemoScene
	{
		DemoScene(const DemoScene&) = delete;
		DemoScene& operator=(const DemoScene&) = delete;
		DemoScene(DemoScene&&) = delete;
		DemoScene& operator=(DemoScene&&) = delete;

		DemoScene(VulkanRenderer* renderer, OpenXRPlugin* xr_plugin, PhysXEngine* physics_engine) :
			fundations{ renderer, xr_plugin, physics_engine },
			systems{
				GraphicsSystem{ renderer, xr_plugin },
				XRSystem{ xr_plugin },
				PhysicsSystem{ physics_engine }
			},
			resources{
				.cone_model = renderer->create_mesh_model(arx::MeshBuilder::Cone(0.01f, 0.05f, 0.1f, 16)),
				.font = renderer->create_bitmap("C:\\Windows\\Fonts\\CascadiaMono.ttf", 1024, 1024, 150), // TODO, temperary, this font does not always exist.
			},
			entities{
				.origin = SpaceTransform{ },
				.xr_offset = SpaceTransform{ },
				.xr_controllers = {
					.left = {
						.transform = SpaceTransform{ &entities.xr_offset },
						.controller = XRController{ 0, &entities.xr_controllers.left.transform },
						.cone = MeshModelComponent{ resources.cone_model, renderer->defaults.default_material, &entities.xr_controllers.left.transform },
					},
					.right = {
						.transform = SpaceTransform{ &entities.xr_offset },
						.controller = XRController{ 1, &entities.xr_controllers.right.transform },
						.cone = MeshModelComponent{ resources.cone_model, renderer->defaults.default_material, &entities.xr_controllers.right.transform },
					},
				},
				.panel = {
					.transform = SpaceTransform{ { 0.f, 1.f, -0.3f } },
					.ui_element = UIElementComponent{ 
						renderer->create_ui_panel({ 0.3f, 0.1f }), 
						renderer->defaults.white_bitmap, 
						&entities.panel.transform 
					},
					.text = {
						.transform = SpaceTransform{ { 0.f, 0.f, 0.001f }, &entities.panel.transform },
						.ui_element = UIElementComponent{
							renderer->create_ui_text("hello world", 0.05f, resources.font),
							resources.font,
							&entities.panel.text.transform
						}
					}
				},
				.plane = {
					.rigid_static = RigidStaticComponent{
						physics_engine->create_rigid_static(
							entities.origin.get_global_matrix(),
							physics_engine->create_shape(arx::PhysicsPlaneGeometry(), glm::rotate(glm::mat4{ 1.f }, 0.5f * glm::pi<float>(), { 0.f, 0.f, 1.f }))
						),
						&entities.origin,
					}
				},
				.test_sphere = {
					.transform = SpaceTransform{ { 0.f, 10.f, -2.f } },
					.mesh_model = MeshModelComponent{ 
						renderer->defaults.sample_sphere, 
						renderer->defaults.default_material, 
						&entities.test_sphere.transform 
					},
					.rigid_dynamic = RigidDynamicComponent{
						physics_engine->create_rigid_dynamic(
							entities.test_sphere.transform.get_global_matrix(),
							physics_engine->create_shape(arx::PhysicsSphereGeometry(0.2f))
						),
						&entities.test_sphere.transform,
					}
				}
			}
		{
			systems.graphics_system.set_camera_offset_transform(&entities.xr_offset);
			entities.xr_controllers.left.cone.register_to_systems(&systems.graphics_system);
			entities.xr_controllers.right.cone.register_to_systems(&systems.graphics_system);
			entities.panel.ui_element.register_to_systems(&systems.graphics_system);
			entities.panel.text.ui_element.register_to_systems(&systems.graphics_system);
			entities.test_sphere.mesh_model.register_to_systems(&systems.graphics_system);

			entities.xr_controllers.left.controller.register_to_systems(&systems.xr_system);
			entities.xr_controllers.right.controller.register_to_systems(&systems.xr_system);

			entities.plane.rigid_static.register_to_systems(&systems.physics_system);
			entities.test_sphere.rigid_dynamic.register_to_systems(&systems.physics_system);
		}

		auto mobilize() -> void {
			systems.graphics_system.mobilize();
			systems.xr_system.mobilize();
			systems.physics_system.mobilize();
		}

		struct {
			VulkanRenderer* renderer;
			OpenXRPlugin* xr_plugin;
			PhysXEngine* physics_engine;
		} fundations;

		struct {
			GraphicsSystem graphics_system;
			XRSystem xr_system;
			PhysicsSystem physics_system;
		} systems;

		struct {
			MeshModel* cone_model;
			Bitmap* font;
		} resources;

		struct {
			SpaceTransform origin;
			SpaceTransform xr_offset;
			struct {
				struct {
					SpaceTransform transform;
					XRController controller;
					MeshModelComponent cone;
				} left;
				struct {
					SpaceTransform transform;
					XRController controller;
					MeshModelComponent cone;
				} right;
			} xr_controllers;
			struct {
				SpaceTransform transform;
				UIElementComponent ui_element;
				struct {
					SpaceTransform transform;
					UIElementComponent ui_element;
				} text;
			} panel;
			struct {
				RigidStaticComponent rigid_static;
			} plane;
			struct {
				SpaceTransform transform;
				MeshModelComponent mesh_model;
				RigidDynamicComponent rigid_dynamic;
			} test_sphere;
		} entities;
	};
}