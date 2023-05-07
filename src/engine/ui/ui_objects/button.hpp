#pragma once

namespace arx
{
	struct Button : public XRPointInteractor::Interactable
	{
	public:
		struct Settings {
			RigidDynamicComponent* actor_component;
			bool is_switch = true;
			bool pushable = false;
			float bottom_depth;
			float activating_factor = 0.6f;
			float invoke_factor = 0.8f;
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsXRSystem<Systems>) {
				xr_system = systems->get<XRSystem>();

				uint32_t shape_count = button_actor->getNbShapes();
				std::vector<PhysicsShape*> shapes(shape_count);
				button_actor->getShapes(shapes.data(), shape_count);
				for (auto shape : shapes) {
					auto filter_data = shape->getSimulationFilterData();
					shape->setSimulationFilterData({
						filter_data.word0 | PhysicsSystem::SimulationFilterBits::XRPointable,
						filter_data.word1,
						filter_data.word2 | PhysicsSystem::SimulationFilterBits::XRUIInteractor,
						filter_data.word3,
					});
				}

				xr_system->point_interactables.insert({ static_cast<PhysicsActor*>(button_actor), static_cast<XRPointInteractor::Interactable*>(this) });
			}
		}

	public:
		auto pass_event(XRPointInteractor::EventType event_type, const XRPointInteractor::ActionState& actions, SpaceTransform* contact_transform) -> void override {
			switch (event_type) {
			case XRPointInteractor::EventType::Enter: {
				activating_before_select = activating;
				enter_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z + (activating ? activating_depth : 0.f);
				invoked = false;
				break;
			}
			case XRPointInteractor::EventType::Exit: {
				if (settings.pushable && !keep_activating) {
					if (settings.is_switch && activating) {
						goto_active(contact_transform);
					}
					else {
						goto_inactive(contact_transform);
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Select: {
				keep_activating = true;
				if (!activating_before_select) {
					goto_active(contact_transform);
				}
				else {
					if (settings.is_switch) {
						goto_inactive(contact_transform);
						invoked = false;
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Deselect: {
				keep_activating = false;
				invoked = false;
				if (!settings.is_switch) {
					goto_inactive(contact_transform);
				}
				break;
			}
			case XRPointInteractor::EventType::Hovering: {
				if (settings.pushable && !keep_activating) {
					auto current_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
					auto z = glm::clamp(current_z - enter_z, -settings.bottom_depth, top);
					glm::vec4 translate{ 0.f, 0.f, z, 1.f };
					button_transform->set_local_position(initial_transform.get_local_matrix() * translate);
					if (!invoked && z < -invoke_depth) {
						invoked = true;
						if (activating) {
							activating = false;
							top = 0.f;
							if (on_release != nullptr) {
								on_release();
							}
						}
						else {
							activating = true;
							top = settings.is_switch ? -activating_depth : 0.f;
							if (on_press != nullptr) {
								on_press();
							}
						}
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Selecting: {
				break;
			}
			default: {
				break;
			}
			}
		}

	private:
		auto goto_active(SpaceTransform* contact_transform) -> void {
			button_transform->set_local_position(initial_transform.get_local_matrix() * glm::vec4(0.f, 0.f, -activating_depth, 1.f));
			top = -activating_depth;
			if (!activating) {
				activating = true;
				if (on_press != nullptr) {
					on_press();
				}
			}
			activating_before_select = true;
			enter_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z + activating_depth;
		}

		auto goto_inactive(SpaceTransform* contact_transform) -> void {
			button_transform->set_local_position(initial_transform.get_local_position());
			top = 0.f;
			if (activating) {
				activating = false;
				if (on_release != nullptr) {
					on_release();
				}
			}
			activating_before_select = false;
			enter_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
		}

	public: // Callbacks.
		std::function<void()> on_press;
		std::function<void()> on_release;

	public:
		Button(Settings&& settings) :
			button_transform{ settings.actor_component->transform },
			button_actor{ settings.actor_component->get_rigid_actor() },
			initial_transform{ button_transform->get_local_matrix(), button_transform->get_parent() },
			activating_depth{ settings.bottom_depth * settings.activating_factor },
			invoke_depth{ settings.bottom_depth * settings.invoke_factor },
			settings{ std::move(settings) }
		{
			button_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		}

	private: // State.
		float enter_z = 0.0f;
		bool activating = false;
		bool invoked = false;
		bool keep_activating = false;
		float top = 0.0f;
		bool activating_before_select = false;

	public: // Settings.
		SpaceTransform* button_transform;
		RigidActor* button_actor;
		Settings settings;
		SpaceTransform initial_transform;
		float activating_depth;
		float invoke_depth;
		XRSystem* xr_system = nullptr;
	};

	namespace presets {
		struct BlankBoxButton {
		public:
			struct Settings {
				SpaceTransform* transform;
				glm::vec3 half_extent;
				std::string text = { };
				Bitmap* font = nullptr;
				bool is_switch = true;
				bool pushable = true;
				float activating_factor = 0.6f;
				float invoke_factor = 0.8f;
				bool debug_visualized = false;
				std::function<void()> on_press = nullptr;
				std::function<void()> on_release = nullptr;
			};

		public:
			template<typename Systems>
			auto register_to_systems(Systems* systems) -> void {
				mesh_model.register_to_systems(systems);
				rigid_dynamic.register_to_systems(systems);
				association.register_to_systems(systems);
				button.register_to_systems(systems);
				if (text.ui_element.ui_element != nullptr) {
					text.ui_element.register_to_systems(systems);
				}
			}

		public:
			BlankBoxButton(VulkanRenderer* renderer, PhysXEngine* physics_engine, Settings&& settings) :
				transform{ settings.transform },
				mesh_model{
					renderer->create_mesh_model(
						MeshBuilder::Box(settings.half_extent.x, settings.half_extent.y, settings.half_extent.z)
							.transform(glm::translate(glm::mat4{ 1.f }, { 0.f, 0.f, settings.half_extent.z }))
					),
					renderer->defaults.default_material,
					transform
				},
				rigid_dynamic{
					physics_engine->create_rigid_dynamic(
						transform->get_global_matrix(),
						physics_engine->create_shape(
							PhysicsBoxGeometry({ settings.half_extent.x, settings.half_extent.y, settings.half_extent.z }),
							glm::translate(glm::mat4{ 1.f }, { 0.f, 0.f, settings.half_extent.z })
						)
					),
					transform,
					settings.debug_visualized,
				},
				association{ &rigid_dynamic, transform },
				button{ {
					.actor_component = &rigid_dynamic,
					.is_switch = settings.is_switch,
					.pushable = settings.pushable,
					.bottom_depth = settings.half_extent.z * 2.f,
					.activating_factor = settings.activating_factor,
					.invoke_factor = settings.invoke_factor,
				} },
				text{
					.transform = SpaceTransform{ glm::vec3(0.f, 0.f, settings.half_extent.z * 2.f + 0.001f), transform },
					.ui_element = UIElementComponent{
						(settings.text.empty() || settings.font == nullptr) ? nullptr : renderer->create_ui_text(settings.text, settings.half_extent.y * 2.f, settings.font),
						settings.font,
						&text.transform,
					}
				},
				on_press{ button.on_press },
				on_release{ button.on_release }
			{
				button.on_press = std::move(settings.on_press);
				button.on_release = std::move(settings.on_release);
			}

		public:
			SpaceTransform* transform;
			MeshModelComponent mesh_model;
			RigidDynamicComponent rigid_dynamic;
			ActorTransformAssociation association;
			Button button;
			struct {
				SpaceTransform transform;
				UIElementComponent ui_element;
			} text;

		public: // Callbacks.
			std::function<void()>& on_press;
			std::function<void()>& on_release;
		};

	}
}