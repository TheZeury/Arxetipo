#pragma once

namespace arx
{
	struct RigidDynamicComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsPhysicsSystem<Systems>) {
				physics_system = systems->get<PhysicsSystem>();
				physics_system->add_rigid_dynamic(rigid_dynamic, transform);
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
		RigidDynamicComponent(RigidDynamic* rigid_dynamic, SpaceTransform* transform, bool debug_visualized = false, bool have_gravity = true) : rigid_dynamic{ rigid_dynamic }, transform{ transform }, debug_visualized{ debug_visualized } {
			rigid_dynamic->userData = this;

			uint32_t shape_count = rigid_dynamic->getNbShapes();
			std::vector<PhysicsShape*> shapes(shape_count);
			rigid_dynamic->getShapes(shapes.data(), shape_count);
			for (auto shape : shapes) {
				if (!shape_transforms.contains(shape)) {
					shape_transforms[shape] = std::make_unique<SpaceTransform>(cnv<glm::mat4>(PhysicsMat44(shape->getLocalPose())), transform);
				}
			}

			if (!have_gravity) {
				rigid_dynamic->setActorFlag(PhysicsActorFlag::eDISABLE_GRAVITY, true);
			}
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
		std::unordered_map<PhysicsShape*, std::unique_ptr<SpaceTransform>> shape_transforms;
		bool debug_visualized;
		PhysicsSystem* physics_system = nullptr;
		GraphicsSystem* graphics_system = nullptr;
	};
}