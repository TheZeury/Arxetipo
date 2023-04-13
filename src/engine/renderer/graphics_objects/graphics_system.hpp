#pragma once

namespace arx
{
	struct GraphicsSystem
	{
	public: // concept: System
		template<typename System>
		requires std::same_as<System, GraphicsSystem>
		auto get() noexcept -> System* {
			return this;
		}
		auto mobilize() -> void {
			renderer->mesh_models.insert(&mesh_models);
		}
		auto freeze() -> void {
			renderer->mesh_models.erase(&mesh_models);
		}
		auto update() -> void {
			// Do nothing.
		}

	public:
		GraphicsSystem(VulkanRenderer* renderer) : renderer{ renderer } {
			
		}

		auto add_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
			mesh_models.insert({ material, model, transform });
		}
		auto remove_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
#if defined(NDEBUG)
			mesh_models.erase(mesh_models.find({ material, model, transform }));
#else
			auto itr = models.find({ material, model });
			if (itr != models.end()) {
				models.erase(itr);
			}
#endif
		}

		std::multiset<std::tuple<Material*, MeshModel*, SpaceTransform*>> mesh_models;
		VulkanRenderer* renderer;
	};
}