#pragma once

namespace arx
{
	struct UIElementComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsGraphicsSystem<Systems>) {
				iterator_in_graphics_system = systems->get<GraphicsSystem>()->add_ui_element(ui_element, bitmap, transform);
			}
		}

	public:
		auto refresh_through_iteracor() -> void {
			if (iterator_in_graphics_system != std::list<std::tuple<Bitmap*, UIElement*, glm::mat4*>>::iterator{ }) {
				std::get<0>(*iterator_in_graphics_system) = bitmap;
				std::get<1>(*iterator_in_graphics_system) = ui_element;
				std::get<2>(*iterator_in_graphics_system) = &(transform->global_matrix);
			}
		}

	public:
		UIElement* ui_element;
		Bitmap* bitmap;
		SpaceTransform* transform;

	public:
		std::list<std::tuple<Bitmap*, UIElement*, glm::mat4*>>::iterator iterator_in_graphics_system = { };
	};
}