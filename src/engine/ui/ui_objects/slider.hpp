#pragma once

namespace arx
{
	struct Slider : public XRPointInteractor::Interactable
	{
	public:
		struct Settings {
			RigidDynamicComponent* actor_component;
			bool invoke_on_registering = true;
			bool no_invoke_before_releasing = false;
			float initial_value = 0.0f;
			float length;
			float anchor = 0.5f;
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsXRSystem<Systems>) {
				xr_system = systems->get<XRSystem>();
				uint32_t shape_count = slider_actor->getNbShapes();
				std::vector<PhysicsShape*> shapes(shape_count);
				slider_actor->getShapes(shapes.data(), shape_count);
				for (auto shape : shapes) {
					auto filter_data = shape->getSimulationFilterData();
					shape->setSimulationFilterData({
						filter_data.word0 | PhysicsSystem::SimulationFilterBits::XRPointable,
						filter_data.word1,
						filter_data.word2 | PhysicsSystem::SimulationFilterBits::XRUIInteractor,
						filter_data.word3,
					});
				}
				xr_system->point_interactables.insert({ static_cast<PhysicsActor*>(slider_actor), static_cast<XRPointInteractor::Interactable*>(this) });

				if (settings.invoke_on_registering && (on_value_changed != nullptr)) {
					on_value_changed(value);
				}
			}
		}

	public:
		auto pass_event(XRPointInteractor::EventType event_type, const XRPointInteractor::ActionState& actions, SpaceTransform* contact_transform) -> void override {
			switch (event_type) {
			case XRPointInteractor::EventType::Enter: {
				break;
			}
			case XRPointInteractor::EventType::Exit: {
				break;
			}
			case XRPointInteractor::EventType::Select: {
				auto select_x = (glm::inverse(anchor_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).x;
				auto select_offset_x = (glm::inverse(anchor_transform.get_local_matrix()) * glm::vec4(slider_transform->get_local_position(), 1.f)).x;
				offset_x = select_offset_x - select_x;
				break;
			}
			case XRPointInteractor::EventType::Deselect: {
				if (settings.no_invoke_before_releasing && on_value_changed != nullptr) {
					on_value_changed(value);
				}
				break;
			}
			case XRPointInteractor::EventType::Hovering: {
				break;
			}
			case XRPointInteractor::EventType::Selecting: {
				auto current_x = (glm::inverse(anchor_transform.get_global_matrix()) * glm::vec4(contact_transform->get_global_position(), 1.f)).x;
				auto x = glm::clamp(current_x + offset_x, left, right );
				glm::vec4 translate{ x, 0.f, 0.f, 1.f };
				slider_transform->set_local_position(anchor_transform.get_local_matrix() * translate);
				value = (x - left) / (right - left);
				if (!settings.no_invoke_before_releasing && on_value_changed != nullptr) {
					on_value_changed(value);
				}
				break;
			}
			default: {
				break;
			}
			}
		}

		auto get_value() const -> float {
			return value;
		}
		auto set_value(float value) -> void {
			auto x = left + value * (right - left);
			glm::vec4 translate{ x, 0.f, 0.f, 1.f };
			slider_transform->set_local_position(anchor_transform.get_local_matrix() * translate);
			this->value = value;
			if (on_value_changed != nullptr) {
				on_value_changed(value);
			}
		}

	public: // Callbacks.
		std::function<void(float)> on_value_changed;

	public:
		Slider(Settings&& settings) :
			value{ settings.initial_value },
			slider_transform{ settings.actor_component->transform },
			slider_actor{ settings.actor_component->get_rigid_actor() },
			anchor_transform{ slider_transform->get_local_matrix(), slider_transform->get_parent() },
			left{ -settings.anchor * settings.length },
			right{ (1.f - settings.anchor) * settings.length },
			settings{ std::move(settings) }
		{
			slider_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
			auto x = left + value * (right - left);
			glm::vec4 translate{ x, 0.f, 0.f, 1.f };
			slider_transform->set_local_position(anchor_transform.get_local_matrix() * translate);
		}

	private: // States.
		float value = 0.0f;
		float offset_x = 0.0f;

	public:
		SpaceTransform* slider_transform;
		RigidActor* slider_actor;
		SpaceTransform anchor_transform;
		float left;
		float right;
		Settings settings;
		XRSystem* xr_system;
	};
}