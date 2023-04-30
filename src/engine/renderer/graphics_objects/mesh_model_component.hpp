#pragma once

namespace arx
{
	struct MeshModelComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsGraphicsSystem<Systems>) {
				systems->get<GraphicsSystem>()->add_mesh_model(model, material, transform);
			}
		}
	public:
		MeshModel* model;
		Material* material;
		SpaceTransform* transform;
	};
}