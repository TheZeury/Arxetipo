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
				.uv_sphere_model = renderer->create_mesh_model(arx::MeshBuilder::UVSphere(0.2f, 16, 16)),
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
						.point_interactor = XRPointInteractor{ &entities.xr_controllers.left.controller },
							.grab_interactor = XRGrabInteractor{ &entities.xr_controllers.left.controller },
							.rigid_static = RigidStaticComponent{
								physics_engine->create_rigid_static(
									entities.xr_controllers.left.transform.get_global_matrix(),
									{
										entities.xr_controllers.left.point_interactor.generate_trigger_shape(
											physics_engine,
											arx::PhysicsSphereGeometry(0.01f),
											glm::translate(glm::mat4{ 1.f }, { 0.f, -0.05f, 0.f })
										),
										entities.xr_controllers.left.grab_interactor.generate_trigger_shape(
											physics_engine,
											arx::PhysicsSphereGeometry(0.05f),
											glm::mat4{ 1.f }
										),
									}
								),
								&entities.xr_controllers.left.transform,
								true
						},
							.association = ActorTransformAssociation{ &entities.xr_controllers.left.rigid_static, &entities.xr_controllers.left.transform },
					},
					.right = {
						.transform = SpaceTransform{ &entities.xr_offset },
						.controller = XRController{ 1, &entities.xr_controllers.right.transform },
						.cone = MeshModelComponent{ resources.cone_model, renderer->defaults.default_material, &entities.xr_controllers.right.transform },
						.point_interactor = XRPointInteractor{ &entities.xr_controllers.right.controller },
						.grab_interactor = XRGrabInteractor { &entities.xr_controllers.right.controller },
						.rigid_static = RigidStaticComponent {
							physics_engine->create_rigid_static(
								entities.xr_controllers.right.transform.get_global_matrix(),
								{
									entities.xr_controllers.right.point_interactor.generate_trigger_shape(
										physics_engine,
										arx::PhysicsSphereGeometry(0.01f),
										glm::translate(glm::mat4{ 1.f }, { 0.f, -0.05f, 0.f })
									),
									entities.xr_controllers.right.grab_interactor.generate_trigger_shape(
										physics_engine,
										arx::PhysicsSphereGeometry(0.05f),
										glm::mat4{ 1.f }
									),
								}
								),
								& entities.xr_controllers.right.transform,
									true
						},
						.association = ActorTransformAssociation{ &entities.xr_controllers.right.rigid_static, &entities.xr_controllers.right.transform },
						},
				},
				.panel = {
					.transform = SpaceTransform{
						{ 0.3f, 1.f, 0.f },
						glm::rotate(glm::quat(1.f, 0.f, 0.f, 0.f), glm::pi<float>() * 0.5f, glm::vec3{ 0.f, -1.f, 0.f }),
					},
					.ui_element = UIElementComponent{
						renderer->create_ui_panel({ 0.3f, 0.5f }),
						renderer->defaults.white_bitmap,
						&entities.panel.transform
					},
					.text = {
						.transform = SpaceTransform{ { 0.f, 0.f, 0.001f }, &entities.panel.transform },
						.text = Text{
							renderer,
							Text::Settings{
								.transform = &entities.panel.text.transform,
								.content = "Hello, World",
								.font = resources.font,
								.height = 0.05f,
								.anchor = { 0.5f, 0.5f },
							}
						},
					},
					.button_a = {
						.transform = SpaceTransform{
							{ -0.05f, -0.1f, 0.005f + 0.001f },
							&entities.panel.transform,
						},
						.mesh_model = MeshModelComponent{
							renderer->create_mesh_model(MeshBuilder::Box(0.02, 0.01, 0.005)),
							renderer->defaults.default_material,
							&entities.panel.button_a.transform
						},
						.rigid_dynamic = RigidDynamicComponent {
							physics_engine->create_rigid_dynamic(
								entities.panel.button_a.transform.get_global_matrix(),
								physics_engine->create_shape(PhysicsBoxGeometry({ 0.02f, 0.01f, 0.005f }))
							),
							&entities.panel.button_a.transform,
							true
						},
						.association = ActorTransformAssociation{ &entities.panel.button_a.rigid_dynamic, &entities.panel.button_a.transform },
						.button = Button({
							.actor_component = &entities.panel.button_a.rigid_dynamic,
							.keep_activated = false,
							.pushable = true,
							.bottom_depth = 0.01f,
						}),
					},
					.button_b = {
						.transform = SpaceTransform{
							{ 0.05f, -0.1f, 0.005f + 0.001f },
							&entities.panel.transform,
						},
						.mesh_model = MeshModelComponent{
							renderer->create_mesh_model(MeshBuilder::Box(0.02, 0.01, 0.005)),
							renderer->defaults.default_material,
							&entities.panel.button_b.transform
						},
						.rigid_dynamic = RigidDynamicComponent {
							physics_engine->create_rigid_dynamic(
								entities.panel.button_b.transform.get_global_matrix(),
								physics_engine->create_shape(PhysicsBoxGeometry({ 0.02f, 0.01f, 0.005f }))
							),
							&entities.panel.button_b.transform,
							true
						},
						.association = ActorTransformAssociation{ &entities.panel.button_b.rigid_dynamic, &entities.panel.button_b.transform },
						.button = Button({
							.actor_component = &entities.panel.button_b.rigid_dynamic,
							.keep_activated = true,
							.pushable = true,
							.bottom_depth = 0.01f,
						}),
					},
					.button_c = {
						.transform = SpaceTransform{
							{ 0.f, -0.1f, 0.001f },
							&entities.panel.transform,
						},
						.button = presets::BlankBoxButton{
							renderer,
							physics_engine,
							presets::BlankBoxButton::Settings{
								.transform = &entities.panel.button_c.transform,
								.half_extent = { 0.02f, 0.01f, 0.005f },
								.text = "Button",
								.font = resources.font,
								.debug_visualized = true,
							}
						}
					},
					.slider = {
						.transform = SpaceTransform{
							{ 0.f, 0.1f, 0.005f },
							&entities.panel.transform,
						},
						.mesh_model = MeshModelComponent{
							renderer->create_mesh_model(MeshBuilder::Icosphere(0.01f, 3)),
							renderer->defaults.default_material,
							&entities.panel.slider.transform
						},
						.rigid_dynamic = RigidDynamicComponent {
							physics_engine->create_rigid_dynamic(
								entities.panel.slider.transform.get_global_matrix(),
								physics_engine->create_shape(PhysicsSphereGeometry(0.01f))
							),
							&entities.panel.slider.transform,
							true
						},
						.association = ActorTransformAssociation{ &entities.panel.slider.rigid_dynamic, &entities.panel.slider.transform },
						.slider = Slider({
							.actor_component = &entities.panel.slider.rigid_dynamic,
							.no_invoke_before_releasing = true,
							.initial_value = 0.5f,
							.length = 0.2f,
						}),
					},
					.rigid_dynamic = RigidDynamicComponent {
						physics_engine->create_rigid_dynamic(
							entities.panel.transform.get_global_matrix(),
							physics_engine->create_shape(arx::PhysicsBoxGeometry({ 0.15f, 0.5f, 0.00001f }))
						),
						&entities.panel.transform,
						true,
						false,
					},
					.grab_interactable = XRGrabInteractable{ &entities.panel.rigid_dynamic, true },
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
						resources.uv_sphere_model, 
						renderer->defaults.default_material, 
						&entities.test_sphere.transform 
					},
					.rigid_dynamic = RigidDynamicComponent{
						physics_engine->create_rigid_dynamic(
							entities.test_sphere.transform.get_global_matrix(),
							physics_engine->create_shape(arx::PhysicsSphereGeometry(0.2f))
						),
						&entities.test_sphere.transform,
						true
					},
					.point_interactable = XRPointInteractable{ &entities.test_sphere.rigid_dynamic },
					.grab_interactable = XRGrabInteractable{ &entities.test_sphere.rigid_dynamic },
				},
			}
		{
			systems.graphics_system.set_camera_offset_transform(&entities.xr_offset);
			entities.xr_controllers.left.cone.register_to_systems(&systems.graphics_system);
			entities.xr_controllers.left.rigid_static.register_to_systems(&systems.graphics_system);
			entities.xr_controllers.right.cone.register_to_systems(&systems.graphics_system);
			entities.xr_controllers.right.rigid_static.register_to_systems(&systems.graphics_system);
			entities.panel.ui_element.register_to_systems(&systems.graphics_system);
			entities.panel.text.text.register_to_systems(&systems.graphics_system);
			entities.panel.button_a.mesh_model.register_to_systems(&systems.graphics_system);
			entities.panel.button_a.rigid_dynamic.register_to_systems(&systems.graphics_system);
			entities.panel.button_b.mesh_model.register_to_systems(&systems.graphics_system);
			entities.panel.button_b.rigid_dynamic.register_to_systems(&systems.graphics_system);
			entities.panel.button_c.button.register_to_systems(&systems.graphics_system);
			entities.panel.slider.mesh_model.register_to_systems(&systems.graphics_system);
			entities.panel.slider.rigid_dynamic.register_to_systems(&systems.graphics_system);
			entities.plane.rigid_static.register_to_systems(&systems.graphics_system);
			entities.test_sphere.mesh_model.register_to_systems(&systems.graphics_system);
			entities.test_sphere.rigid_dynamic.register_to_systems(&systems.graphics_system);

			entities.xr_controllers.left.controller.register_to_systems(&systems.xr_system);
			entities.xr_controllers.left.point_interactor.register_to_systems(&systems.xr_system);
			entities.xr_controllers.left.grab_interactor.register_to_systems(&systems.xr_system);
			entities.xr_controllers.right.controller.register_to_systems(&systems.xr_system);
			entities.xr_controllers.right.point_interactor.register_to_systems(&systems.xr_system);
			entities.xr_controllers.right.grab_interactor.register_to_systems(&systems.xr_system);
			entities.panel.button_a.button.register_to_systems(&systems.xr_system);
			entities.panel.button_b.button.register_to_systems(&systems.xr_system);
			entities.panel.button_c.button.register_to_systems(&systems.xr_system);
			entities.panel.slider.slider.register_to_systems(&systems.xr_system);
			entities.panel.grab_interactable.register_to_systems(&systems.xr_system);
			entities.test_sphere.point_interactable.register_to_systems(&systems.xr_system);
			entities.test_sphere.grab_interactable.register_to_systems(&systems.xr_system);

			entities.xr_controllers.left.rigid_static.register_to_systems(&systems.physics_system);
			entities.xr_controllers.left.association.register_to_systems(&systems.physics_system);
			entities.xr_controllers.right.rigid_static.register_to_systems(&systems.physics_system);
			entities.xr_controllers.right.association.register_to_systems(&systems.physics_system);
			entities.panel.button_a.rigid_dynamic.register_to_systems(&systems.physics_system);
			entities.panel.button_a.association.register_to_systems(&systems.physics_system);
			entities.panel.button_b.rigid_dynamic.register_to_systems(&systems.physics_system);
			entities.panel.button_b.association.register_to_systems(&systems.physics_system);
			entities.panel.button_c.button.register_to_systems(&systems.physics_system);
			entities.panel.slider.rigid_dynamic.register_to_systems(&systems.physics_system);
			entities.panel.slider.association.register_to_systems(&systems.physics_system);
			entities.panel.rigid_dynamic.register_to_systems(&systems.physics_system);
			entities.plane.rigid_static.register_to_systems(&systems.physics_system);
			entities.test_sphere.rigid_dynamic.register_to_systems(&systems.physics_system);

			entities.panel.button_a.button.on_press = [&]() {
				auto value = entities.panel.slider.slider.get_value();
				auto height = 0.5f + value * (10.f - 0.5f);
				entities.test_sphere.rigid_dynamic.rigid_dynamic->setGlobalPose({ 0.f, height, -2.f });
			};
			entities.panel.button_b.button.on_press = [&]() {
				fundations.renderer->debug_mode = VulkanRenderer::DebugMode::OnlyDebug;
			};
			entities.panel.button_b.button.on_release = [&]() {
				fundations.renderer->debug_mode = VulkanRenderer::DebugMode::Mixed;
			};
			entities.panel.button_c.button.on_press = [&]() {
				entities.panel.text.text.content_push_back('{');
			};
			entities.panel.button_c.button.on_release = [&]() {
				entities.panel.text.text.content_push_back('}');
			};
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
			MeshModel* uv_sphere_model;
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
					XRPointInteractor point_interactor;
					XRGrabInteractor grab_interactor;
					RigidStaticComponent rigid_static;
					ActorTransformAssociation association;
				} left;
				struct {
					SpaceTransform transform;
					XRController controller;
					MeshModelComponent cone;
					XRPointInteractor point_interactor;
					XRGrabInteractor grab_interactor;
					RigidStaticComponent rigid_static;
					ActorTransformAssociation association;
				} right;
			} xr_controllers;
			struct {
				SpaceTransform transform;
				UIElementComponent ui_element;
				struct {
					SpaceTransform transform;
					Text text;
				} text;
				struct {
					SpaceTransform transform;
					MeshModelComponent mesh_model;
					RigidDynamicComponent rigid_dynamic;
					ActorTransformAssociation association;
					Button button;
				} button_a;
				struct {
					SpaceTransform transform;
					MeshModelComponent mesh_model;
					RigidDynamicComponent rigid_dynamic;
					ActorTransformAssociation association;
					Button button;
				} button_b;
				struct {
					SpaceTransform transform;
					presets::BlankBoxButton button;
				} button_c;
				struct {
					SpaceTransform transform;
					MeshModelComponent mesh_model;
					RigidDynamicComponent rigid_dynamic;
					ActorTransformAssociation association;
					Slider slider;
				} slider;
				RigidDynamicComponent rigid_dynamic;
				XRGrabInteractable grab_interactable;
			} panel;
			struct {
				RigidStaticComponent rigid_static;
			} plane;
			struct {
				SpaceTransform transform;
				MeshModelComponent mesh_model;
				RigidDynamicComponent rigid_dynamic;
				XRPointInteractable point_interactable;
				XRGrabInteractable grab_interactable;
			} test_sphere;
		} entities;
	};
}