#pragma once

namespace arx
{
	struct RigidStaticComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			systems->get<PhysicsSystem>()->add_rigid_static(rigid_static, transform);
		}

	public:
		RigidStatic* rigid_static;
		SpaceTransform* transform;
	};
}