#pragma once

namespace arx
{
	struct Text 
	{
	public:
		struct Settings {
			SpaceTransform* transform;
			std::string content;
			Bitmap* font;
			float height;
			glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
			glm::vec2 anchor = { 0.0f, 0.0f };
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			if constexpr (ContainsGraphicsSystem<Systems>) {
				graphics_system = systems->get<GraphicsSystem>();
			}
			ui_element.register_to_systems(systems);
		}

	public:
		auto get_content() const -> std::string {
			return settings.content;
		}
		auto get_content_ref() const -> const std::string& {
			return settings.content;
		}
		auto set_content(const std::string& new_content) -> void {
			settings.content = new_content;
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}
		auto set_content(std::string&& new_content) -> void {
			settings.content = std::move(new_content);
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}
		auto content_push_back(char c) -> void {
			settings.content.push_back(c);
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}
		auto content_pop_back() -> void {
			if (!settings.content.empty()) {
				settings.content.pop_back();
				graphics_system->renderer->delete_ui_element(ui_element.ui_element);
				ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
				ui_element.refresh_through_iteracor();
			}
		}

		auto get_font() const -> Bitmap* {
			return settings.font;
		}
		auto set_font(Bitmap* new_font) -> void {
			settings.font = new_font;
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.bitmap = new_font;
			ui_element.refresh_through_iteracor();
		}

		auto get_height() const -> float {
			return settings.height;
		}
		auto set_height(float new_height) -> void {
			settings.height = new_height;
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}

		auto get_color() const -> glm::vec4 {
			return settings.color;
		}
		auto set_color(glm::vec4 new_color) -> void {
			settings.color = new_color;
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}

		auto get_anchor() const -> glm::vec2 {
			return settings.anchor;
		}
		auto set_anchor(glm::vec2 new_anchor) -> void {
			settings.anchor = new_anchor;
			graphics_system->renderer->delete_ui_element(ui_element.ui_element);
			ui_element.ui_element = graphics_system->renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor);
			ui_element.refresh_through_iteracor();
		}
		/*auto set_anchor_stay(glm::vec2 new_anchor) -> void {
			set_anchor(new_anchor);
			transform->set_local_position(transform->get_local_position() + glm::vec3{ new_anchor.x, new_anchor.y, 0.0f });
		}*/

	public:
		Text(VulkanRenderer* renderer, Settings&& settings) : 
			transform{ settings.transform },
			ui_element{
				.ui_element = renderer->create_ui_text(settings.content, settings.height, settings.font, settings.color, settings.anchor),
				.bitmap = settings.font,
				.transform = transform,
			},
			settings{ std::move(settings) }
		{ }

	public:
		SpaceTransform* transform;
		UIElementComponent ui_element;
		Settings settings;
		GraphicsSystem* graphics_system;
	};
}