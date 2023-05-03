#pragma once

namespace arx
{
	struct XRGrabInteractable : public XRGrabInteractor::Interactable
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsXRSystem<Systems>) {
				xr_system = systems->get<XRSystem>();

				uint32_t shape_count = actor->getNbShapes();
				std::vector<PhysicsShape*> shapes(shape_count);
				actor->getShapes(shapes.data(), shape_count);
				for (auto shape : shapes) {
					auto filter_data = shape->getSimulationFilterData();
					shape->setSimulationFilterData({
						filter_data.word0 | PhysicsSystem::SimulationFilterBits::XRGrabable,
						filter_data.word1,
						filter_data.word2 | (only_ui ? PhysicsSystem::SimulationFilterBits::XRUIInteractor : 0),
						filter_data.word3,
						});
				}

				xr_system->grab_interactables.insert({ static_cast<PhysicsActor*>(actor), static_cast<XRGrabInteractor::Interactable*>(this) });
			}
		}

	public:
		auto pass_event(XRGrabInteractor::EventType event_type, const XRGrabInteractor::ActionState& actions) -> void override {
			switch (event_type) {
			case XRGrabInteractor::EventType::Enter: {
				if (on_enter != nullptr)
					on_enter();
				break;
			}
			case XRGrabInteractor::EventType::Exit: {
				if (on_exit != nullptr)
					on_exit();
				break;
			}
			case XRGrabInteractor::EventType::Select: {
				if (free_grab) {
					attach_transform.set_local_matrix(glm::inverse(actor_transform->get_global_matrix()) * actions.transform->get_global_matrix());
				}
				no_gravity = actor->getActorFlags() & PhysicsActorFlag::eDISABLE_GRAVITY;
				actor->setActorFlag(PhysicsActorFlag::eDISABLE_GRAVITY, true);
				if (on_select != nullptr)
					on_select();
				break;
			}
			case XRGrabInteractor::EventType::Deselect: {
				actor->setActorFlag(PhysicsActorFlag::eDISABLE_GRAVITY, no_gravity);
				if (on_deselect != nullptr)
					on_deselect();
				break;
			}
			case XRGrabInteractor::EventType::Hovering: {
				if (on_hovering != nullptr)
					on_hovering();
				break;
			}
			case XRGrabInteractor::EventType::Selecting: {
				if (kinematic) {
					// Not implemented yet.
				}
				else {
					actor->setGlobalPose(PhysicsTransform(cnv<PhysicsMat44>(actions.transform->get_global_matrix() * glm::inverse(attach_transform.get_local_matrix()))));
				}
				if (on_selecting != nullptr)
					on_selecting();
				break;
			}
			default: {
				break;
			}
			}
		}

	public: // User-defined callbacks
		std::function<void()> on_enter;
		std::function<void()> on_exit;
		std::function<void()> on_select;
		std::function<void()> on_deselect;
		std::function<void()> on_hovering;
		std::function<void()> on_selecting;

	public:
		template<typename ActorComponent>
		XRGrabInteractable(ActorComponent* actor_component, bool only_ui = false, bool free_grab = true, bool kinematic = false) : 
			actor{ actor_component->get_rigid_actor() }, 
			only_ui{ only_ui },
			free_grab { free_grab },
			kinematic{ kinematic },
			actor_transform{ actor_component->transform }, // Copy pointer.
			attach_transform{ actor_component->transform } // Set parent. Those two have the same argument but different meaning.
		{
		}

	public:
		bool no_gravity = false;

	public:
		RigidActor* actor;
		bool only_ui;
		bool free_grab;
		bool kinematic;
		SpaceTransform* actor_transform;
		SpaceTransform attach_transform;

		XRSystem* xr_system = nullptr;
	};
}