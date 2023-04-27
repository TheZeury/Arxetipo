#pragma once

namespace arx
{
	struct RigidDynamicComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			physics_system = systems->get<PhysicsSystem>();
			physics_system->add_rigid_dynamic(rigid_dynamic, transform);
		}

	public:
		RigidDynamicComponent(RigidDynamic* rigid_dynamic, SpaceTransform* transform) : rigid_dynamic{ rigid_dynamic }, transform{ transform } {
			rigid_dynamic->userData = this;
		}

		auto get_physics_actor() -> PhysicsActor* {
			return rigid_dynamic;
		}
		auto get_rigid_actor() -> RigidActor* {
			return rigid_dynamic;
		}

	public:
		RigidDynamic* rigid_dynamic;
		SpaceTransform* transform;
		PhysicsSystem* physics_system = nullptr;
	};
}