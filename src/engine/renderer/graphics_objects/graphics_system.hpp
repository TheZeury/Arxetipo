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
			if (!mobilized) {
				renderer->mesh_models.insert(&mesh_models);
				mobilized = true;
			}
		}
		auto freeze() -> void {
			if (mobilized) {
				renderer->mesh_models.erase(&mesh_models);
				mobilized = false;
			}
		}
		auto update() -> void {
			if (mobilized) {
				SpaceTransform* last_transform = nullptr;
				for (auto transform : transforms) {
					if (transform != last_transform) {
						transform->update_matrix();
						last_transform = transform;
					}
				}
			}
		}

	public:
		GraphicsSystem(VulkanRenderer* renderer) : renderer{ renderer } {
			
		}

		auto add_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
			mesh_models.insert({ material, model, &(transform->global_matrix) });
			transforms.insert(transform);
		}
		auto remove_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
#if defined(NDEBUG)
			mesh_models.erase(mesh_models.find({ material, model, &(transform->global_matrix) }));
			transforms.erase(transforms.find(transform));
#else
			auto itr_m = models.find({ material, model, &(transform->global_matrix) });
			if (itr_m != models.end()) {
				models.erase(itr_m);
			}
			auto itr_t = transforms.find(transform);
			if (itr_t != transforms.end()) {
				transforms.erase(itr_t);
			}
#endif
		}

		bool mobilized = false;
		std::multiset<std::tuple<Material*, MeshModel*, glm::mat4*>> mesh_models;
		std::multiset<SpaceTransform*> transforms;
		VulkanRenderer* renderer;
	};
}