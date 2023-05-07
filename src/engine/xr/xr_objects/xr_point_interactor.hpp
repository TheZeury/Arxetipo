#pragma once

namespace arx
{
	struct XRPointInteractor : public XRSystem::Interactor
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

			glm::vec3 position;
			glm::quat rotation;

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
			virtual auto pass_event(EventType event_type, const ActionState& actions, SpaceTransform* contact_transform) -> void = 0;
		};

		struct PointInteractorTriggerCallback : public ITriggerCallback {
			PointInteractorTriggerCallback(XRPointInteractor* interactor) : interactor(interactor) {}
			XRPointInteractor* interactor;
			auto on_enter(const TriggerPair& pair) -> void override {
				auto actor = pair.otherActor;
				auto iter = interactor->xr_system->point_interactables.find(actor);
				if (iter == interactor->xr_system->point_interactables.end()) {
					log_error("Detected physics actor doesn't have a point interactable. If you don't want it be point interactable, please don't raise `SimulationFilterBits::XRPointable` in any of its shape's simulation filter data.");
					return;
				}
				Interactable* interactable = static_cast<Interactable*>(iter->second);
				interactor->add_candidate(interactable, get_transform_from_shape(pair.triggerActor, pair.triggerShape));
			}
			auto on_exit(const TriggerPair& pair) -> void override {
				auto actor = pair.otherActor;
				auto iter = interactor->xr_system->point_interactables.find(actor);
				if (iter == interactor->xr_system->point_interactables.end()) {
					log_error("Detected physics actor doesn't have a point interactable. If you don't want it be point interactable, please don't raise `SimulationFilterBits::XRPointable` in any of its shape's simulation filter data.");
					return;
				}
				Interactable* interactable = static_cast<Interactable*>(iter->second);
				interactor->remove_candidate(interactable, get_transform_from_shape(pair.triggerActor, pair.triggerShape));
			}
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsXRSystem<Systems>) {
				xr_system = systems->get<XRSystem>();
				xr_system->add_interactor(this);
			}
		}

	public:
		auto pass_actions() -> void override {
			if (controller == nullptr) return;
			uint32_t controller_id = controller->controller_id;
			auto& input_state = xr_system->xr_plugin->input_state;

			// Raw actions.
			{
				action_state.active = bool(input_state.hand_active[controller_id]);
				action_state.position = cnv<glm::vec3>(input_state.hand_locations[controller_id].pose.position);
				action_state.rotation = cnv<glm::quat>(input_state.hand_locations[controller_id].pose.orientation);
				action_state.trigger_value = input_state.trigger_states[controller_id].currentState;
				action_state.grip_value = input_state.grip_states[controller_id].currentState;
				action_state.primary_button = bool(input_state.primary_button_states[controller_id].currentState);
				action_state.secondary_button = bool(input_state.secondary_button_states[controller_id].currentState);
				action_state.thumbstick.x = input_state.thumbstick_x_states[controller_id].currentState;
				action_state.thumbstick.y = input_state.thumbstick_y_states[controller_id].currentState;
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

			for (auto& [candidate, type] : candidates_change) {
				auto& [interactable, contact_transform] = candidate;
				if (type) { // add	
					if (candidates.find(candidate) == candidates.end()) {
						candidates.insert(candidate);
						interactable->pass_event(EventType::Enter, action_state, contact_transform);
					}
				}
				else { // remove
					if (candidates.find(candidate) != candidates.end()) {
						candidates.erase(candidate);
						interactable->pass_event(EventType::Exit, action_state, contact_transform);
					}
				}
			}
			candidates_change.clear();

			for (auto& [candidate, contact_transform] : candidates) {
				candidate->pass_event(EventType::Hovering, action_state, contact_transform);
			}

			auto& [candidate, contact_transform] = selected;

			if (action_state.trigger_pressed) {
				if (candidate != nullptr) {
					log_error("Interactor tried selecting a new interactable, but it already had one selected. Releasing the old selected interactable");
					candidate->pass_event(EventType::Deselect, action_state, contact_transform);
					selected = { nullptr, nullptr };
				}
				if (!candidates.empty()) {
					selected = *candidates.begin();
					candidate->pass_event(EventType::Select, action_state, contact_transform);
				}
			}
			else if (action_state.trigger_released) {
				if (candidate != nullptr) {
					candidate->pass_event(EventType::Deselect, action_state, contact_transform);
					selected = { nullptr, nullptr };
				}
			}

			if (candidate != nullptr) {
				candidate->pass_event(EventType::Selecting, action_state, contact_transform);
			}
		}

		auto add_candidate(Interactable* interactable, SpaceTransform* contact_transform) -> void {
			candidates_change.push_back({ { interactable, contact_transform }, true });
		}
		auto remove_candidate(Interactable* interactable, SpaceTransform* contact_transform) -> void {
			candidates_change.push_back({ { interactable, contact_transform }, false });
		}

		auto add_candidate_immediate(Interactable* interactable, SpaceTransform* contact_transform) -> void {
			if (candidates.find({ interactable, contact_transform }) == candidates.end()) {
				candidates.insert({ interactable, contact_transform });
				interactable->pass_event(EventType::Enter, action_state, contact_transform);
			}
		}
		auto remove_candidate_immediate(Interactable* interactable, SpaceTransform* contact_transform) -> void {
			if (candidates.find({ interactable, contact_transform }) != candidates.end()) {
				candidates.erase({ interactable, contact_transform });
				interactable->pass_event(EventType::Exit, action_state, contact_transform);
			}
		}

		auto generate_trigger_shape(PhysXEngine* physics_engine, const PhysicsGeometry& geometry, const glm::mat4& local_transform = glm::mat4{ 1.f }) -> PhysicsShape* {
			auto shape = physics_engine->create_shape(geometry, local_transform, PhysicsShapeFlag::eVISUALIZATION | PhysicsShapeFlag::eSCENE_QUERY_SHAPE | PhysicsShapeFlag::eTRIGGER_SHAPE);
			shape->setSimulationFilterData({ PhysicsSystem::SimulationFilterBits::XRUIInteractor, 0, PhysicsSystem::SimulationFilterBits::XRPointable, 0 });
			shape->userData = new PointInteractorTriggerCallback{ this };
			return shape;
		}

	public:
		XRPointInteractor(XRController* controller) : controller{ controller } {
		}

	public:
		std::set<std::tuple<Interactable*, SpaceTransform*>> candidates;
		std::vector<std::pair<std::tuple<Interactable*, SpaceTransform*>, bool>> candidates_change; // true for add, false for remove
		std::tuple<Interactable*, SpaceTransform*> selected = { nullptr, nullptr };
		ActionState action_state;
		XRController* controller;
		XRSystem* xr_system = nullptr;

	public:
		static auto get_transform_from_shape(PhysicsActor* actor, PhysicsShape* shape) -> SpaceTransform* {
			switch (actor->getConcreteType())
			{
			case physx::PxConcreteType::eRIGID_STATIC: {
				auto component = static_cast<RigidStaticComponent*>(actor->userData);
				if (component->shape_transforms.contains(shape)) {
					return component->shape_transforms.at(shape).get();
				}
				else {
					log_error("shape not found, returning transform of the actor.");
					return component->transform;
				}
			}
			case physx::PxConcreteType::eRIGID_DYNAMIC: {
				auto component = static_cast<RigidDynamicComponent*>(actor->userData);
				if (component->shape_transforms.contains(shape)) {
					return component->shape_transforms.at(shape).get();
				}
				else {
					log_error("shape not found, returning transform of the actor.");
					return component->transform;
				}
			}
			default: {
				throw std::runtime_error("PhysicsSystem::get_transform_from_shape: Unknown actor type.");
			}
			}
		}
	};
}