#pragma once

namespace arx
{
	using PhysicsShape = physx::PxShape;
	using PhysicsMaterial = physx::PxMaterial;
	using PhysicsActor = physx::PxActor;
	using RigidActor = physx::PxRigidActor;
	using RigidStatic = physx::PxRigidStatic;
	using RigidBody = physx::PxRigidBody;
	using RigidDynamic = physx::PxRigidDynamic;
	using ArticulationLink = physx::PxArticulationLink;
	using PhysicsScene = physx::PxScene;
	using TriggerPair = physx::PxTriggerPair;
	using PhysicsGeometry = physx::PxGeometry;
	using PhysicsGeometryType = physx::PxGeometryType::Enum;
	using PhysicsBoxGeometry = physx::PxBoxGeometry;
	using PhysicsSphereGeometry = physx::PxSphereGeometry;
	using PhysicsCapsuleGeometry = physx::PxCapsuleGeometry;
	using PhysicsPlaneGeometry = physx::PxPlaneGeometry;
	using PhysicsConvexMeshGeometry = physx::PxConvexMeshGeometry;
	using PhysicsTriangleMeshGeometry = physx::PxTriangleMeshGeometry;
	using PhysicsHeightFieldGeometry = physx::PxHeightFieldGeometry;
	using PhysicsMaterialFlags = physx::PxMaterialFlags;
	using PhysicsMaterialFlag = physx::PxMaterialFlag::Enum;
	using PhysicsShapeFlags = physx::PxShapeFlags;
	using PhysicsShapeFlag = physx::PxShapeFlag::Enum;
	using PhysicsActorFlags = physx::PxActorFlags;
	using PhysicsActorFlag = physx::PxActorFlag::Enum;
	using PhysicsSceneFlags = physx::PxSceneFlags;
	using PhysicsSceneFlag = physx::PxSceneFlag::Enum;

	using PhysicsTransform = physx::PxTransform;
	using PhysicsVec3 = physx::PxVec3;
	using PhysicsQuat = physx::PxQuat;
	using PhysicsMat33 = physx::PxMat33;
	using PhysicsMat44 = physx::PxMat44;
}