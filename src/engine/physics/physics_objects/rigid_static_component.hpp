#pragma once

namespace arx
{
	struct RigidStaticComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsPhysicsSystem<Systems>) {
				physics_system = systems->get<PhysicsSystem>();
				physics_system->add_rigid_static(rigid_static, transform);
			}

			if constexpr (ContainsGraphicsSystem<Systems>) {
				if (debug_visualized) {
					graphics_system = systems->get<GraphicsSystem>();

					for (auto& [shape, shape_transform] : shape_transforms) {
						auto shape_mesh = graphics_system->renderer->create_mesh_model(PhysicsSystem::mesh_from_shape(shape));
						auto mesh_material = (shape->getFlags() & PhysicsShapeFlag::eTRIGGER_SHAPE) ? graphics_system->materials.trigger_blue : graphics_system->materials.collider_green;
						graphics_system->add_debug_mesh_model(shape_mesh, mesh_material, shape_transform.get());
					}
				}
			}
		}

	public:
		RigidStaticComponent(RigidStatic* rigid_static, SpaceTransform* transform, bool debug_visualized = false) : rigid_static{ rigid_static }, transform{ transform }, debug_visualized{ debug_visualized } {
			rigid_static->userData = this;

			uint32_t shape_count = rigid_static->getNbShapes();
			std::vector<PhysicsShape*> shapes(shape_count);
			rigid_static->getShapes(shapes.data(), shape_count);
			for (auto shape : shapes) {
				if (!shape_transforms.contains(shape)) {
					shape_transforms[shape] = std::make_unique<SpaceTransform>(cnv<glm::mat4>(PhysicsMat44(shape->getLocalPose())), transform);
				}
			}
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
		std::unordered_map<PhysicsShape*, std::unique_ptr<SpaceTransform>> shape_transforms;
		bool debug_visualized;
		PhysicsSystem* physics_system = nullptr;
		GraphicsSystem* graphics_system = nullptr;
	};
}