#pragma once

namespace arx
{
	struct RigidStaticComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			physics_system = systems->get<PhysicsSystem>();
			physics_system->add_rigid_static(rigid_static, transform);
		}

	public:
		RigidStaticComponent(RigidStatic* rigid_static, SpaceTransform* transform) : rigid_static{ rigid_static }, transform{ transform } {
			rigid_static->userData = this;
		}

		auto get_physics_actor() -> PhysicsActor* {
			return rigid_static;
		}
		auto get_rigid_actor() -> RigidActor* {
			return rigid_static;
		}

	public:
		RigidStatic* rigid_static;
		SpaceTransform* transform;
		PhysicsSystem* physics_system = nullptr;
	};
}