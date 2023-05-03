#pragma once

namespace arx
{
	struct XRPointInteractable : public XRPointInteractor::Interactable
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
						filter_data.word0 | PhysicsSystem::SimulationFilterBits::XRPointable,
						filter_data.word1,
						filter_data.word2 | (only_ui ? PhysicsSystem::SimulationFilterBits::XRUIInteractor : 0),
						filter_data.word3,
						});
				}

				xr_system->point_interactables.insert({ static_cast<PhysicsActor*>(actor), static_cast<XRPointInteractor::Interactable*>(this) });
			}
		}

	public:
		auto pass_event(XRPointInteractor::EventType event_type, const XRPointInteractor::ActionState& actions, SpaceTransform* contact_transform) -> void override {
			switch (event_type) {
			case XRPointInteractor::EventType::Enter: {
				if (on_enter != nullptr)
					on_enter();
				break;
			}
			case XRPointInteractor::EventType::Exit: {
				if (on_exit != nullptr)
					on_exit();
				break;
			}
			case XRPointInteractor::EventType::Select: {
				if (on_select != nullptr)
					on_select();
				break;
			}
			case XRPointInteractor::EventType::Deselect: {
				if (on_deselect != nullptr)
					on_deselect();
				break;
			}
			case XRPointInteractor::EventType::Hovering: {
				if (on_hovering != nullptr)
					on_hovering();
				break;
			}
			case XRPointInteractor::EventType::Selecting: {
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
		XRPointInteractable(RigidActor* actor) : actor{ actor } {
		}
		template<typename ActorComponent>
		XRPointInteractable(ActorComponent* actor_component, bool only_ui = false) : actor{ actor_component->get_rigid_actor() }, only_ui{ only_ui } {
		}

	public:
		RigidActor* actor;
		bool only_ui;
		XRSystem* xr_system = nullptr;
	};
}