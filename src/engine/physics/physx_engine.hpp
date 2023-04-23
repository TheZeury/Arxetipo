#pragma once

#include <unordered_set>

#include <PxConfig.h>
#include <PxPhysicsAPI.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "../helpers/arx_logger.hpp"
#include "../helpers/arx_math.hpp"
#include "../helpers/arx_array_proxy.hpp"

#include "physx_engine/physx_objects.hpp"

namespace arx 
{
	struct ITriggerCallback
	{
		const char sign[16] = "TCB";
		auto verify() -> bool const { return sign[0] == 'T' && sign[1] == 'C' && sign[2] == 'B'; }

		virtual auto OnEnter(const TriggerPair& pair) -> void = 0;
		virtual auto OnExit(const TriggerPair& pair) -> void = 0;
	};

	class EventCallback : public physx::PxSimulationEventCallback
	{
		virtual auto onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) -> void { }
		virtual auto onWake(PhysicsActor** actors, physx::PxU32 count) -> void { }
		virtual auto onSleep(PhysicsActor** actors, physx::PxU32 count) -> void { }
		virtual auto onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) -> void { }
		virtual auto onTrigger(TriggerPair* pairs, physx::PxU32 count) -> void
		{
			// This function has a hidden danger: It doesn't check if `pair.triggerShape->userData` is a `ITriggerCallback*` or not.
			for (size_t i = 0; i < count; ++i)
			{
				const auto& pair = pairs[i];
				if (pair.triggerShape->userData != nullptr)
				{
					ITriggerCallback* triggerCallback = reinterpret_cast<ITriggerCallback*>(pair.triggerShape->userData);
					if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
					{
						triggerCallback->OnEnter(pair);
					}
					else if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
					{
						triggerCallback->OnExit(pair);
					}
					else
					{
						log_error("Undefined trigger state.");
					}
				}
			}
		}
		virtual void onAdvance(const RigidBody* const* bodyBuffer, const PhysicsTransform* poseBuffer, const physx::PxU32 count) { }
	};

	physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		// Let triggers through.
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		// Filter test.
		// word0: mask.
		// word1: bits to filter out if counter's mask contains any.
		// word2: bits that counter's mask must contains all.
		// word3: friend mask, ignore word0, word1 and word2 as long as two friend masks overlap.
		if (!(filterData0.word3 & filterData0.word3) && (
			(filterData0.word1 & filterData1.word0) || (filterData1.word1 & filterData0.word0) ||
			(filterData0.word2 & filterData1.word0 ^ filterData0.word2) || (filterData1.word2 & filterData0.word0 ^ filterData1.word2)))
		{
			return physx::PxFilterFlag::eKILL;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}

	struct PhysXEngine
	{
	public: // concept: PhysicsEngine
		auto initialize() -> void {
			log_step("PhysX", "Initializing PhysX");
			foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
			physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true);
			dispatcher = physx::PxDefaultCpuDispatcherCreate(physx::PxThread::getNbPhysicalCores());
			log_success();

			defaults.default_material = physics->createMaterial(0.5f, 0.5f, 0.5f);
		}
		auto simulate(float delta_time) -> void {
			for (auto& scene : scenes)
			{
				scene->simulate(delta_time);
			}
			for (auto& scene : scenes)
			{
				scene->fetchResults(true);
			}
		}

	public: // concept: PhysicsResourceManager
		auto create_material(float static_friction, float dynamic_friction, float restitution) -> PhysicsMaterial* {
			return physics->createMaterial(static_friction, dynamic_friction, restitution);
		}
		auto create_shape(const PhysicsGeometry& geometry, const PhysicsMaterial& material, bool exclusize = false, const PhysicsShapeFlags& flags = PhysicsShapeFlag::eVISUALIZATION | PhysicsShapeFlag::eSCENE_QUERY_SHAPE | PhysicsShapeFlag::eSIMULATION_SHAPE) -> PhysicsShape* {
			return physics->createShape(geometry, material, exclusize, flags);
		}
		auto create_shape(const PhysicsGeometry& geometry, const glm::mat4& local_transform = glm::mat4{ 1.f }, const PhysicsShapeFlags& flags = PhysicsShapeFlag::eVISUALIZATION | PhysicsShapeFlag::eSCENE_QUERY_SHAPE | PhysicsShapeFlag::eSIMULATION_SHAPE) -> PhysicsShape* {
			auto shape = create_shape(geometry, *(defaults.default_material), false, flags);
			shape->setLocalPose(PhysicsTransform(cnv<PhysicsMat44>(local_transform)));
			return shape;
		}
		auto create_shape(const PhysicsGeometry& geometry, const glm::mat4& local_transform, const PhysicsMaterial& material, bool exclusize = false, const PhysicsShapeFlags& flags = PhysicsShapeFlag::eVISUALIZATION | PhysicsShapeFlag::eSCENE_QUERY_SHAPE | PhysicsShapeFlag::eSIMULATION_SHAPE) -> PhysicsShape* {
			auto shape = create_shape(geometry, material, exclusize, flags);
			shape->setLocalPose(PhysicsTransform(cnv<PhysicsMat44>(local_transform)));
			return shape;
		}
		auto create_rigid_dynamic(const glm::mat4& transform) -> RigidDynamic* {
			return physics->createRigidDynamic(PhysicsTransform(cnv<PhysicsMat44>(transform)));
		}
		auto create_rigid_dynamic(const glm::mat4& transform, ArrayProxy<PhysicsShape*> shapes) -> RigidDynamic* {
			auto rigid = create_rigid_dynamic(transform);
			for (auto shape : shapes)
			{
				rigid->attachShape(*shape);
			}
			return rigid;
		}
		auto create_rigid_static(const glm::mat4& transform) -> RigidStatic* {
			return physics->createRigidStatic(PhysicsTransform(cnv<PhysicsMat44>(transform)));
		}
		auto create_rigid_static(const glm::mat4& transform, ArrayProxy<PhysicsShape*> shapes) -> RigidStatic* {
			auto rigid = create_rigid_static(transform);
			for (auto shape : shapes)
			{
				rigid->attachShape(*shape);
			}
			return rigid;
		}
		auto create_physics_scene() -> physx::PxScene* {
			physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
			{
				sceneDesc.gravity = PhysicsVec3(0.0f, -9.81f, 0.0f);
				sceneDesc.cpuDispatcher = dispatcher;
				sceneDesc.simulationEventCallback = new EventCallback();
				sceneDesc.filterShader = SimulationFilterShader;// PxDefaultSimulationFilterShader;
			}
			return physics->createScene(sceneDesc);
		}

	public:
		struct {
			PhysicsMaterial* default_material = nullptr;
		} defaults;

	public:
		physx::PxDefaultAllocator allocator;
		physx::PxDefaultErrorCallback errorCallback;
		physx::PxFoundation* foundation = nullptr;
		physx::PxPhysics* physics = nullptr;
		physx::PxDefaultCpuDispatcher* dispatcher = nullptr;

	public:
		std::unordered_set<physx::PxScene*> scenes;
	};
}