#pragma once

namespace arx
{
	struct XRSystem
	{
	public:
		struct Interactor {
			virtual auto pass_actions() -> void = 0;
		};

	public: // concept: System
		template<typename System>
		requires std::same_as<System, XRSystem>
		auto get() noexcept -> System* {
			return this;
		}
		auto mobilize() -> void {
		}
		auto freeze() -> void {
		}
		auto update() -> void {
			xr_plugin->poll_actions();
			for (auto [controller_id, transform] : controllers) {
				transform->set_local_position(cnv<glm::vec3>(xr_plugin->input_state.handLocations[controller_id].pose.position));
				transform->set_local_rotation(cnv<glm::quat>(xr_plugin->input_state.handLocations[controller_id].pose.orientation));
			}
			for (auto interactor : interactors) {
				interactor->pass_actions();
			}
		}

	public:
		XRSystem(OpenXRPlugin* xr_plugin) : xr_plugin{ xr_plugin } {
		}

		auto add_controller(uint32_t controller_id, SpaceTransform* transform) -> void {
			controllers.insert({ controller_id, transform });
		}
		auto remove_controller(uint32_t controller_id) -> void {
			controllers.erase(controllers.find({ controller_id, nullptr }));
		}

		auto add_interactor(Interactor* interactor) -> void {
			interactors.insert(interactor);
		}
		auto remove_interactor(Interactor* interactor) -> void {
			interactors.erase(interactor);
		}

		std::unordered_set<Interactor*> interactors;
		std::unordered_map<PhysicsActor*, void*> point_interactables;
		std::unordered_map<PhysicsActor*, void*> grab_interactables;
		std::multiset<std::tuple<uint32_t, SpaceTransform*>> controllers;
		OpenXRPlugin* xr_plugin;
	};

	template<typename Systems>
	concept ContainsXRSystem = Contains<Systems, XRSystem>;
}