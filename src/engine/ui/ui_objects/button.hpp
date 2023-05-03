#pragma once

namespace arx
{
	struct Button : public XRPointInteractor::Interactable
	{
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
				if (pushable) {
					/*enter_z = (glm::inverse(button_transform->get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
					original_position = button_transform->get_local_position();*/
				}
				break;
			}
			case XRPointInteractor::EventType::Exit: {
				break;
			}
			case XRPointInteractor::EventType::Select: {
				if (!activating) {
					//button_transform->set_local_position({ original_position.x, original_position.y, original_position.z - activating_depth });
					button_transform->set_local_position(button_transform->get_local_matrix() * glm::vec4(0.f, 0.f, -activating_depth, 1.f));
					activating = true;
					if (on_press != nullptr) {
						on_press();
					}
				}
				else {
					if (keep_activated) {
						button_transform->set_local_position(original_position);
						activating = false;
						if (on_release != nullptr) {
							on_release();
						}
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Deselect: {
				if (!keep_activated) {
					button_transform->set_local_position(original_position);
					activating = false;
					if (on_release != nullptr) {
						on_release();
					}
				}
				break;
			}
			case XRPointInteractor::EventType::Hovering: {
				if (pushable) {
					/*auto current_z = (glm::inverse(button_transform->get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).z;
					auto translation = button_transform->get_local_matrix() * glm::vec4{ 0.f, 0.f, current_z - enter_z, 0.f };
					button_transform->set_local_position(original_position + glm::vec3{ translation });*/
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
		Button(RigidDynamicComponent* actor_component, float bottom_depth, float activating_factor = 0.8f, bool keep_activated = true, bool pushable = false, float switch_factor = 0.9f) :
			button_transform{ actor_component->transform },
			button_actor{ actor_component->get_rigid_actor() },
			original_position{ actor_component->transform->get_local_position() },
			bottom_depth{ bottom_depth },
			activating_depth{ bottom_depth * activating_factor },
			keep_activated{ keep_activated },
			pushable{ pushable },
			switch_depth{ bottom_depth * switch_factor }
		{
			actor_component->rigid_dynamic->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
		}

	public: // State.
		float enter_z;
		bool activating = false;

	public: // Settings.
		SpaceTransform* button_transform;
		RigidActor* button_actor;
		glm::vec3 original_position;
		float bottom_depth;
		float activating_depth;
		float switch_depth;
		bool keep_activated;
		bool pushable;
		XRSystem* xr_system = nullptr;
	};
}