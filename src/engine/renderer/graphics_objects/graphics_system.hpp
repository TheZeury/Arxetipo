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
				renderer->ui_elements.insert(&ui_elements);
				renderer->debug_mesh_models.insert(&debug_mesh_models);
				mobilized = true;
			}
		}
		auto freeze() -> void {
			if (mobilized) {
				renderer->mesh_models.erase(&mesh_models);
				renderer->ui_elements.erase(&ui_elements);
				renderer->debug_mesh_models.erase(&debug_mesh_models);
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
				if (xr_plugin != nullptr) {
					xr_plugin->update(camera_offset_transform != nullptr ? camera_offset_transform->get_global_matrix() : glm::mat4{ 1.f });
				}
			}
		}

	public:
		GraphicsSystem(VulkanRenderer* renderer, OpenXRPlugin* xr_plugin = nullptr) :
			renderer{ renderer }, xr_plugin{ xr_plugin },
			materials{
				.collider_green = renderer->create_material(nullptr, nullptr, { .2f, .8f, .2f, 1.f }),
				.trigger_blue = renderer->create_material(nullptr, nullptr, { 0.f, .7f, .9f, 1.f }),
			}
		{
			
		}

		auto set_camera_offset_transform(SpaceTransform* transform) -> void {
			camera_offset_transform = transform;
		}
		auto get_camera_offset_transform() -> SpaceTransform* {
			return camera_offset_transform;
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
			auto itr_m = mesh_models.find({ material, model, &(transform->global_matrix) });
			if (itr_m != mesh_models.end()) {
				mesh_models.erase(itr_m);
			}
			auto itr_t = transforms.find(transform);
			if (itr_t != transforms.end()) {
				transforms.erase(itr_t);
			}
#endif
		}

		auto add_debug_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
			debug_mesh_models.insert({ material, model, &(transform->global_matrix) });
			transforms.insert(transform);
		}
		auto remove_debug_mesh_model(MeshModel* model, Material* material, SpaceTransform* transform) -> void {
#if defined(NDEBUG)
			debug_mesh_models.erase(debug_mesh_models.find({ material, model, &(transform->global_matrix) }));
			transforms.erase(transforms.find(transform));
#else
			auto itr_m = debug_mesh_models.find({ material, model, &(transform->global_matrix) });
			if (itr_m != debug_mesh_models.end()) {
				debug_mesh_models.erase(itr_m);
			}
			auto itr_t = transforms.find(transform);
			if (itr_t != transforms.end()) {
				transforms.erase(itr_t);
			}
#endif
		}

		auto add_ui_element(UIElement* element, Bitmap* bitmap, SpaceTransform* transform) -> void {
			ui_elements.push_back({ bitmap, element, &(transform->global_matrix) });
			transforms.insert(transform);
		}
		auto remove_ui_element(UIElement* element, Bitmap* bitmap, SpaceTransform* transform) -> void {
#if defined(NDEBUG)
			for (auto itr = ui_elements.begin(); itr != ui_elements.end(); ++itr) { // TODO: to be optimized.
				if (*itr == std::tuple{ bitmap, element, &(transform->global_matrix) }) {
					ui_elements.erase(itr);
					break;
				}
			}
			transforms.erase(transforms.find(transform));
#else
			auto itr_u = ui_elements.find({ bitmap, element, &(transform->global_matrix) });
			if (itr_u != ui_elements.end()) {
				ui_elements.erase(itr_u);
			}
			auto itr_t = transforms.find(transform);
			if (itr_t != transforms.end()) {
				transforms.erase(itr_t);
			}
#endif
		}

		struct {
			Material* collider_green = nullptr;
			Material* trigger_blue = nullptr;
		} materials;

		bool mobilized = false;
		SpaceTransform* camera_offset_transform = nullptr;
		std::multiset<std::tuple<Material*, MeshModel*, glm::mat4*>> mesh_models;
		std::list<std::tuple<Bitmap*, UIElement*, glm::mat4*>> ui_elements;
		std::multiset<std::tuple<Material*, MeshModel*, glm::mat4*>> debug_mesh_models;
		std::multiset<SpaceTransform*> transforms;
		VulkanRenderer* renderer;
		OpenXRPlugin* xr_plugin;
	};

	template<typename Systems>
	concept ContainsGraphicsSystem = requires(Systems systems) {
		{ systems.template get<GraphicsSystem>() } -> std::convertible_to<GraphicsSystem*>;
	};
}