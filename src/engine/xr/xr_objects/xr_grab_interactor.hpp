#pragma once

namespace arx
{
	struct XRGrabInteractor : public XRSystem::Interactor
	{
	public:
		enum class EventType {
			Enter,
			Exit,
			Select,
			Deselect,
			Hovering,
			Selecting,
		};

		struct ActionState {
			bool active;

			SpaceTransform* transform = nullptr;

			float trigger_value;
			bool trigger_pressing = false;
			bool trigger_pressed = false;
			bool trigger_released = false;

			float grip_value;
			bool grip_pressing = false;
			bool grip_pressed = false;
			bool grip_released = false;

			bool primary_button;
			bool primary_pressing = false;
			bool primary_pressed = false;
			bool primary_released = false;

			bool secondary_button;
			bool secondary_pressing = false;
			bool secondary_pressed = false;
			bool secondary_released = false;

			glm::vec2 thumbstick;
			bool thumbstick_pushing = false;
			bool thumbstick_pushed = false;
			bool thumbstick_released = false;
		};

		struct Interactable {
			virtual auto pass_event(EventType event_type, const ActionState& actions) -> void = 0;
		};

		struct GrabInteractorTriggerCallback : public ITriggerCallback {
			GrabInteractorTriggerCallback(XRGrabInteractor* interactor) : interactor(interactor) {}
			XRGrabInteractor* interactor;
			auto OnEnter(const TriggerPair& pair) -> void override {
				auto actor = pair.otherActor;
				auto iter = interactor->xr_system->grab_interactables.find(actor);
				if (iter == interactor->xr_system->grab_interactables.end()) {
					log_error("Detected physics actor doesn't have a grab interactable. If you don't want it be grab interactable, please don't raise `SimulationFilterBits::XRGrabable` in any of its shape's simulation filter data.");
					return;
				}
				Interactable* interactable = static_cast<Interactable*>(iter->second);
				interactor->add_candidate(interactable);
			}
			auto OnExit(const TriggerPair& pair) -> void override {
				auto actor = pair.otherActor;
				auto iter = interactor->xr_system->grab_interactables.find(actor);
				if (iter == interactor->xr_system->grab_interactables.end()) {
					log_error("Detected physics actor doesn't have a grab interactable. If you don't want it be grab interactable, please don't raise `SimulationFilterBits::XRGrabable` in any of its shape's simulation filter data.");
					return;
				}
				Interactable* interactable = static_cast<Interactable*>(iter->second);
				interactor->remove_candidate(interactable);
			}
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			xr_system = systems->get<XRSystem>();
			xr_system->add_interactor(this);
		}

	public:
		auto pass_actions() -> void override {
			if (controller == nullptr) return;
			uint32_t controller_id = controller->controller_id;
			auto& input_state = xr_system->xr_plugin->input_state;

			// Raw actions.
			{
				action_state.active = bool(input_state.handActive[controller_id]);
				/*action_state.position = cnv<glm::vec3>(input_state.handLocations[controller_id].pose.position);
				action_state.rotation = cnv<glm::quat>(input_state.handLocations[controller_id].pose.orientation);*/
				action_state.transform = controller->transform;
				action_state.trigger_value = input_state.triggerStates[controller_id].currentState;
				action_state.grip_value = input_state.gripStates[controller_id].currentState;
				action_state.primary_button = bool(input_state.primaryButtonStates[controller_id].currentState);
				action_state.secondary_button = bool(input_state.secondaryButtonStates[controller_id].currentState);
				action_state.thumbstick.x = input_state.thumbstickXStates[controller_id].currentState;
				action_state.thumbstick.y = input_state.thumbstickYStates[controller_id].currentState;
			}

			// Processed actions.
			{
				action_state.trigger_pressed = false;
				action_state.trigger_released = false;
				if (action_state.trigger_pressing) {
					if (action_state.trigger_value < 0.5f) {
						action_state.trigger_released = true;
						action_state.trigger_pressing = false;
					}
				}
				else {
					if (action_state.trigger_value > 0.5f) {
						action_state.trigger_pressed = true;
						action_state.trigger_pressing = true;
					}
				}

				action_state.grip_pressed = false;
				action_state.grip_released = false;
				if (action_state.grip_pressing) {
					if (action_state.grip_value < 0.5f) {
						action_state.grip_released = true;
						action_state.grip_pressing = false;
					}
				}
				else {
					if (action_state.grip_value > 0.5f) {
						action_state.grip_pressed = true;
						action_state.grip_pressing = true;
					}
				}

				action_state.primary_pressed = false;
				action_state.primary_released = false;
				if (action_state.primary_pressing) {
					if (!action_state.primary_button) {
						action_state.primary_released = true;
						action_state.primary_pressing = false;
					}
				}
				else {
					if (action_state.primary_button) {
						action_state.primary_pressed = true;
						action_state.primary_pressing = true;
					}
				}

				action_state.secondary_pressed = false;
				action_state.secondary_released = false;
				if (action_state.secondary_pressing) {
					if (!action_state.secondary_button) {
						action_state.secondary_released = true;
						action_state.secondary_pressing = false;
					}
				}
				else {
					if (action_state.secondary_button) {
						action_state.secondary_pressed = true;
						action_state.secondary_pressing = true;
					}
				}

				action_state.thumbstick_pushed = false;
				action_state.thumbstick_released = false;
				if (action_state.thumbstick_pushing) {
					if (glm::length(action_state.thumbstick) < 0.5f) {
						action_state.thumbstick_released = true;
						action_state.thumbstick_pushing = false;
					}
				}
				else {
					if (glm::length(action_state.thumbstick) > 0.5f) {
						action_state.thumbstick_pushed = true;
						action_state.thumbstick_pushing = true;
					}
				}
			}

			for (auto& candidate : candidates) {
				candidate->pass_event(EventType::Hovering, action_state);
			}

			if (action_state.grip_pressed) {
				if (selected != nullptr) {
					log_error("Interactor tried selecting a new interactable, but it already had one selected. Releasing the old selected interactable");
					selected->pass_event(EventType::Deselect, action_state);
					selected = nullptr;
				}
				if (!candidates.empty()) {
					selected = *candidates.begin();
					selected->pass_event(EventType::Select, action_state);
				}
			}
			else if (action_state.grip_released) {
				if (selected != nullptr) {
					selected->pass_event(EventType::Deselect, action_state);
					selected = nullptr;
				}
			}

			if (selected != nullptr) {
				selected->pass_event(EventType::Selecting, action_state);
			}
		}

		auto add_candidate(Interactable* interactable) -> void {
			if (candidates.find(interactable) == candidates.end()) {
				candidates.insert(interactable);
				interactable->pass_event(EventType::Enter, action_state);
			}
		}

		auto remove_candidate(Interactable* interactable) -> void {
			if (candidates.find(interactable) != candidates.end()) {
				candidates.erase(interactable);
				interactable->pass_event(EventType::Exit, action_state);
			}
		}

		auto generate_trigger_shape(PhysXEngine* physics_engine, const PhysicsGeometry& geometry, const glm::mat4& local_transform = glm::mat4{ 1.f }) -> PhysicsShape* {
			auto shape = physics_engine->create_shape(geometry, local_transform, PhysicsShapeFlag::eVISUALIZATION | PhysicsShapeFlag::eSCENE_QUERY_SHAPE | PhysicsShapeFlag::eTRIGGER_SHAPE);
			shape->setSimulationFilterData({ 0, 0, PhysicsSystem::SimulationFilterBits::XRGrabable, 0 });
			shape->userData = new GrabInteractorTriggerCallback{ this };
			return shape;
		}

	public:
		XRGrabInteractor(XRController* controller) : controller{ controller } {
		}

	public:
		std::unordered_set<Interactable*> candidates;
		Interactable* selected = nullptr;
		ActionState action_state;
		XRController* controller;
		XRSystem* xr_system = nullptr;
	};
}