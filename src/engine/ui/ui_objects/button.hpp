#pragma once

namespace arx
{
	struct Button : public XRPointInteractor::Interactable
	{
	public:
		struct Settings {
			RigidDynamicComponent* actor_component;
			bool keep_activated = true;
			bool pushable = false;
			float bottom_depth;
			float activating_factor = 0.6f;
			float switch_factor = 0.8f;
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
				if (settings.pushable) {
					enter_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
				}
				break;
			}
			case XRPointInteractor::EventType::Exit: {
				if (settings.pushable) {
					switched = false;
					if (activating && !settings.keep_activated && !keep_activating) {
						activating = false;
						top = 0.f;
						if (on_release != nullptr) {
							on_release();
						}
					}
					button_transform->set_local_position(initial_transform.get_local_matrix() * glm::vec4(0.f, 0.f, top, 1.f));
				}
				break;
			}
			case XRPointInteractor::EventType::Select: {
				if (!activating) {
					button_transform->set_local_position(initial_transform.get_local_matrix() * glm::vec4(0.f, 0.f, -activating_depth, 1.f));
					activating = true;
					if (!settings.keep_activated) {
						keep_activating = true;
						// `settings.keep_activated` and `keep_activating` can't be both true.
						// `keep_activating` is turned on when `settings.keep_activated` is false and trigger is being hold.
						// `keep_activating` will disable pushing temporarily.
						// If they are both turned on, problems is that pushing will still be disabled after trigger is released.
					}
					top = -activating_depth;
					if (on_press != nullptr) {
						on_press();
					}
				}
				else {
					if (settings.keep_activated) {
						button_transform->set_local_position(initial_transform.get_local_position());
						activating = false;
						top = 0.f;
						if (on_release != nullptr) {
							on_release();
						}
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Deselect: {
				if (!settings.keep_activated) {
					button_transform->set_local_position(initial_transform.get_local_position());
					activating = false;
					keep_activating = false;
					top = 0.f;
					if (on_release != nullptr) {
						on_release();
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Hovering: {
				if (settings.pushable && !keep_activating) {
					auto current_z = (glm::inverse(initial_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
					auto z = glm::clamp(current_z - enter_z, -settings.bottom_depth, top);
					glm::vec4 translate{ 0.f, 0.f, z, 1.f };
					button_transform->set_local_position(initial_transform.get_local_matrix() * translate);
					if (!switched && z < -switch_depth) {
						switched = true;
						if (activating) {
							activating = false;
							top = 0.f;
							if (on_release != nullptr) {
								on_release();
							}
						}
						else {
							activating = true;
							top = -activating_depth;
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

	public: // Callbacks.
		std::function<void()> on_press;
		std::function<void()> on_release;

	public:
		Button(Settings&& settings) :
			button_transform{ settings.actor_component->transform },
			button_actor{ settings.actor_component->get_rigid_actor() },
			initial_transform{ button_transform->get_local_matrix(), button_transform->get_parent() },
			activating_depth{ settings.bottom_depth * settings.activating_factor },
			switch_depth{ settings.bottom_depth * settings.switch_factor },
			settings{ std::move(settings) }
		{
			button_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		}

	private: // State.
		float enter_z = 0.0f;
		bool activating = false;
		bool switched = false;
		bool keep_activating = false;
		float top = 0.0f;

	public: // Settings.
		SpaceTransform* button_transform;
		RigidActor* button_actor;
		Settings settings;
		SpaceTransform initial_transform;
		float activating_depth;
		float switch_depth;
		XRSystem* xr_system = nullptr;
	};
}