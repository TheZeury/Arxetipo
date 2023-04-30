#pragma once

namespace arx
{
	struct XRController
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsXRSystem<Systems>) {
				systems->get<XRSystem>()->add_controller(controller_id, transform);
			}
		}
	public:
		uint32_t controller_id;
		SpaceTransform* transform;
	};
}