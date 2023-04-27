#pragma once

namespace arx
{
	struct PhysicsSystem
	{
	public:
		struct SimulationFilterBits {
			enum Enum: physx::PxU32 {
				World = 1 << 0,
				UI = 1 << 1,
				XRPointable = 1 << 2,
				XRGrabable = 1 << 3,
			};
		};

	public: // concept: System
		template<typename System>
		requires std::same_as<System, PhysicsSystem>
		auto get() noexcept -> System* {
			return this;
		}
		auto mobilize() -> void {
			if (!mobilized) {
				physx_engine->scenes.insert(scene);
				mobilized = true;
			}
		}
		auto freeze() -> void {
			if (mobilized) {
				physx_engine->scenes.erase(scene);
				mobilized = false;
			}
		}
		auto update() -> void {
			if (mobilized) {
				for (auto [actor, transform] : associations) {
					actor->setGlobalPose(PhysicsTransform(cnv<PhysicsMat44>(transform->get_global_matrix())));
				}
				if (physx_engine != nullptr) {
					physx_engine->simulate(time_delta);
				}
				for (auto [rigid_dynamic, transform] : rigid_dynamics) {
					if (rigid_dynamic->isSleeping() == false) {
						transform->set_global_matrix(cnv<glm::mat4>(PhysicsMat44(rigid_dynamic->getGlobalPose())));
					}
				}
				// Normally rigid statics don't move.
				// In case they do, it must be managed by this system, so we can update the transform at that time.
				// So anyway, there's no need to update them here.
			}
		}
		
	public:
		PhysicsSystem(PhysXEngine* physx_engine) : physx_engine{ physx_engine } {
			scene = physx_engine->create_physics_scene();
		}

		auto set_time_delta(float time_delta) -> void {
			this->time_delta = time_delta;
		}
		auto get_time_delta() -> float {
			return time_delta;
		}

		auto add_rigid_dynamic(RigidDynamic* rigid_dynamic, SpaceTransform* transform) -> void {
			rigid_dynamics.insert({ rigid_dynamic, transform });
			scene->addActor(*rigid_dynamic);
		}
		auto remove_rigid_dynamic(RigidDynamic* rigid_dynamic, SpaceTransform* transform) -> void {
			rigid_dynamics.erase(rigid_dynamics.find({ rigid_dynamic, transform }));
			scene->removeActor(*rigid_dynamic);
		}

		auto add_rigid_static(RigidStatic* rigid_static, SpaceTransform* transform) -> void {
			rigid_statics.insert({ rigid_static, transform });
			scene->addActor(*rigid_static);
		}
		auto remove_rigid_static(RigidStatic* rigid_static, SpaceTransform* transform) -> void {
			rigid_statics.erase(rigid_statics.find({ rigid_static, transform }));
			scene->removeActor(*rigid_static);
		}

		auto add_association(RigidActor* actor, SpaceTransform* transform) -> void {
			associations.insert({ actor, transform });
		}
		auto remove_association(RigidActor* actor) -> void {
			associations.erase(associations.find(actor));
		}

	public:
		bool mobilized = false;
		float time_delta = 0.02f;
		std::multiset<std::tuple<RigidStatic*, SpaceTransform*>> rigid_statics;
		std::multiset<std::tuple<RigidDynamic*, SpaceTransform*>> rigid_dynamics;
		std::unordered_map<RigidActor*, SpaceTransform*> associations;
		PhysicsScene* scene = nullptr;
		PhysXEngine* physx_engine;

	public:
		static auto mesh_from_shape(PhysicsShape* shape) -> MeshBuilder {
			auto& geometry = shape->getGeometry();
			switch (geometry.getType()) {
			case PhysicsGeometryType::eBOX: {
				auto& extents = static_cast<PhysicsBoxGeometry const&>(geometry).halfExtents;
				return MeshBuilder::Box(extents.x, extents.y, extents.z);
			}
			case PhysicsGeometryType::eSPHERE: {
				auto radius = static_cast<PhysicsSphereGeometry const&>(geometry).radius;
				return MeshBuilder::Icosphere(radius, 3);
			}
			default: {
				return MeshBuilder();
			}
			}
			return MeshBuilder();
		}
	};

	template<typename Systems>
	concept ContainsPhysicsSystem = requires(Systems systems) {
		{ systems.template get<PhysicsSystem>() } -> std::convertible_to<PhysicsSystem*>;
	};
}