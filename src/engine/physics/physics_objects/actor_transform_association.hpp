#pragma once

namespace arx
{
	struct ActorTransformAssociation
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsPhysicsSystem<Systems>) {
				physics_system = systems->get<PhysicsSystem>();
				physics_system->add_association(actor, transform);
			}
		}

	public:
		ActorTransformAssociation(RigidActor* actor, SpaceTransform* transform) : actor{ actor }, transform{ transform } {
		}
		template<typename ActorComponent>
		ActorTransformAssociation(ActorComponent* actor_component, SpaceTransform* transform) : actor{ actor_component->get_rigid_actor() }, transform{ transform } {
		}

	public:
		RigidActor* actor;
		SpaceTransform* transform;
		PhysicsSystem* physics_system = nullptr;
	};
}