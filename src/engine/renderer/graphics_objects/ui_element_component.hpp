#pragma once

namespace arx
{
	struct UIElementComponent
	{
	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsGraphicsSystem<Systems>) {
				systems->get<GraphicsSystem>()->add_ui_element(ui_element, bitmap, transform);
			}
		}

	public:
		UIElement* ui_element;
		Bitmap* bitmap;
		SpaceTransform* transform;
	};
}