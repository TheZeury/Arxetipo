#pragma once

namespace arx
{
	struct RigidDynamicComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			systems->get<PhysicsSystem>()->add_rigid_dynamic(rigid_dynamic, transform);
		}

	public:
		RigidDynamic* rigid_dynamic;
		SpaceTransform* transform;
	};
}