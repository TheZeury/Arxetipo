#pragma once

namespace arx::presets
{
	const std::array<const std::string, 4> dvorak_layout = {
		"S;qjkxbmwvz/", // `S` for `Shift`
		"Caoeuidhtns|", // `C` for `Caps Lock`
		"\t\'.,pyfgcrl?",
		"`!@#$%^&*()_",
	};

	const std::array<const std::string, 4> qwerty_layout = {
		"Szxcvbnm,./?", // `S` for `Shift`
		"Casdfghjkl;:", // `C` for `Caps Lock`
		"\tqwertyuiop|",
		"`!@#$%^&*()_",
	};

	const std::array<const std::string, 4> keypad_layout = {
		"(\"0=.+)",
		"<\'123->",
		"[~456*]",
		"{\\789/}",
	};

	const std::unordered_map<char, std::string> key_label = {
		{ ' ', "Space" },
		{ '\t', "=>" },
		{ '\n', "Enter" },
		{ '\b', "Backspace" },
		{ 'D', "Delete" },
		{ 'C', "=^" },
		{ 'S', "^^" },
		{ '`', "`" },
		{ '~', "~" },
		{ '!', "!" },
		{ '@', "@" },
		{ '#', "#" },
		{ '$', "$" },
		{ '%', "%" },
		{ '^', "^" },
		{ '&', "&" },
		{ '*', "*" },
		{ '(', "(" },
		{ ')', ")" },
		{ '-', "-" },
		{ '+', "+" },
		{ '_', "_" },
		{ '=', "=" },
		{ '{', "{" },
		{ '}', "}" },
		{ '[', "[" },
		{ ']', "]" },
		{ '|', "|" },
		{ '\\', "\\" },
		{ ':', ":" },
		{ ';', ";" },
		{ '\'', "\'" },
		{ '"', "\"" },
		{ '<', "<" },
		{ '>', ">" },
		{ ',', "," },
		{ '.', "." },
		{ '?', "?" },
		{ '/', "/" },
		{ 'a', "A" },
		{ 'b', "B" },
		{ 'c', "C" },
		{ 'd', "D" },
		{ 'e', "E" },
		{ 'f', "F" },
		{ 'g', "G" },
		{ 'h', "H" },
		{ 'i', "I" },
		{ 'j', "J" },
		{ 'k', "K" },
		{ 'l', "L" },
		{ 'm', "M" },
		{ 'n', "N" },
		{ 'o', "O" },
		{ 'p', "P" },
		{ 'q', "Q" },
		{ 'r', "R" },
		{ 's', "S" },
		{ 't', "T" },
		{ 'u', "U" },
		{ 'v', "V" },
		{ 'w', "W" },
		{ 'x', "X" },
		{ 'y', "Y" },
		{ 'z', "Z" },
		{ '0', "0" },
		{ '1', "1" },
		{ '2', "2" },
		{ '3', "3" },
		{ '4', "4" },
		{ '5', "5" },
		{ '6', "6" },
		{ '7', "7" },
		{ '8', "8" },
		{ '9', "9" },
	};

	auto to_label(char c) -> std::string {
		auto it = key_label.find(c);
		if (it != key_label.end()) {
			return it->second;
		}
		return "Error";
	}

	struct TypewriterPart
	{
	public:
		enum class NavigationKey : char
		{
			None = '\0',
			Space = ' ',
			Backspace = '\b',
			Enter = '\n',
			Delete = 'D',
		};

		struct Settings {
			SpaceTransform* transform;
			glm::vec3 key_half_extent; // extents of the models
			glm::vec2 key_space; // extents of how much space each key occupies
			struct {
				size_t bottom;
				size_t left;
				size_t top;
				size_t right;
			} region;
			glm::vec2 anchor;
			ArrayProxy<const std::string> layout;
			NavigationKey navigation_key;
			Bitmap* font;
			bool debug_visualized = false;
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			for (auto& button : key_buttons) {
				button->register_to_systems(systems);
			}
		}

	public:
		TypewriterPart(VulkanRenderer* renderer, PhysXEngine* physics_engine, Settings&& settings) {
			board_extent = {
				(settings.region.right - settings.region.left) * settings.key_space.x,
				(settings.region.top - settings.region.bottom + (settings.navigation_key == NavigationKey::None ? 0 : 1)) * settings.key_space.y,
			};
			real_anchor = {
				settings.anchor.x * board_extent.x,
				settings.anchor.y * board_extent.y,
				0.f
			};

			for (size_t layout_row = settings.region.bottom; layout_row < settings.region.top; ++layout_row) {
				for (size_t layout_col = settings.region.left; layout_col < settings.region.right; ++layout_col) {
					auto row = layout_row - settings.region.bottom + (settings.navigation_key == NavigationKey::None ? 0 : 1);
					auto col = layout_col - settings.region.left;
					char key = settings.layout.data()[layout_row][layout_col];

					auto key_transform = std::make_unique<SpaceTransform>(
						glm::vec3{ ((col + 0.5f) * settings.key_space.x), ((row + 0.5f) * settings.key_space.y), 0.f } - real_anchor,
						settings.transform
					);
					auto key_button = std::make_unique<BlankBoxButton>(renderer, physics_engine, BlankBoxButton::Settings{
						.transform = key_transform.get(),
						.half_extent = settings.key_half_extent,
						.text = to_label(key),
						.font = settings.font,
						.keep_activated = key == 'C',
						.pushable = true,
						.debug_visualized = settings.debug_visualized,
					});

					key_transforms.push_back(std::move(key_transform));
					key_buttons.push_back(std::move(key_button));
				}
			}

			if (settings.navigation_key != NavigationKey::None) {
				auto key_transform = std::make_unique<SpaceTransform>(
					glm::vec3{ 0.5f * board_extent.x, 0.5f * settings.key_space.y, 0.f } - real_anchor,
					settings.transform
				);
				auto key_button = std::make_unique<BlankBoxButton>(renderer, physics_engine, BlankBoxButton::Settings{
					.transform = key_transform.get(),
					.half_extent = glm::vec3{ board_extent.x * 0.5f - settings.key_space.x * 0.5f + settings.key_half_extent.x , settings.key_half_extent.y, settings.key_half_extent.z },
					.text = to_label(static_cast<char>(settings.navigation_key)),
					.font = settings.font,
					.keep_activated = false,
					.pushable = true,
					.debug_visualized = settings.debug_visualized,
				});
				key_transforms.push_back(std::move(key_transform));
				key_buttons.push_back(std::move(key_button));
			}
		}

	public:
		std::vector<std::unique_ptr<SpaceTransform>> key_transforms;
		std::vector<std::unique_ptr<BlankBoxButton>> key_buttons;

		glm::vec2 board_extent;
		glm::vec3 real_anchor;
	};

	struct Typewriter
	{
	public:
		enum class Layout
		{
			Qwerty,
			Dvorak,
		};

		struct Settings {
			SpaceTransform* transform;
			Layout layout;
			glm::vec3 key_half_extent; // extents of the models
			glm::vec2 key_space; // extents of how much space each key occupies
			Bitmap* font;
			bool debug_visualized = false;
		};

	public:
		template<typename Systems>
		auto register_to_systems(Systems* systems) -> void {
			typewriter_part_left.register_to_systems(systems);
			typewriter_part_right.register_to_systems(systems);
			typewriter_keypad.register_to_systems(systems);
			left_part_rigid_dynamic.register_to_systems(systems);
			right_part_rigid_dynamic.register_to_systems(systems);
			keypad_rigid_dynamic.register_to_systems(systems);
			left_part_grab_interactable.register_to_systems(systems);
			right_part_grab_interactable.register_to_systems(systems);
			keypad_grab_interactable.register_to_systems(systems);
		}

	public:
		Typewriter(VulkanRenderer* renderer, PhysXEngine* physics_engine, Settings&& settings) :
			left_part_transform{ settings.transform },
			right_part_transform{ settings.transform },
			keypad_transform{ settings.transform },
			typewriter_part_left(renderer, physics_engine, presets::TypewriterPart::Settings{
				.transform = &left_part_transform,
				.key_half_extent = settings.key_half_extent,
				.key_space = settings.key_space,
				.region = {
					.bottom = 0,
					.left = 0,
					.top = 4,
					.right = 6,
				},
				.anchor = { 1.7f, 0.5f },
				.layout = settings.layout == Layout::Dvorak ? presets::dvorak_layout : presets::qwerty_layout,
				.navigation_key = presets::TypewriterPart::NavigationKey::Backspace,
				.font = settings.font,
				.debug_visualized = settings.debug_visualized,
			}),
			typewriter_part_right(renderer, physics_engine, presets::TypewriterPart::Settings{
				.transform = &right_part_transform,
				.key_half_extent = settings.key_half_extent,
				.key_space = settings.key_space,
				.region = {
					.bottom = 0,
					.left = 6,
					.top = 4,
					.right = 12,
				},
				.anchor = { -0.7f, 0.5f },
				.layout = settings.layout == Layout::Dvorak ? presets::dvorak_layout : presets::qwerty_layout,
				.navigation_key = presets::TypewriterPart::NavigationKey::Space,
				.font = settings.font,
				.debug_visualized = settings.debug_visualized,
			}),
			typewriter_keypad(renderer, physics_engine, presets::TypewriterPart::Settings{
				.transform = &keypad_transform,
				.key_half_extent = settings.key_half_extent,
				.key_space = settings.key_space,
				.region = {
					.bottom = 0,
					.left = 0,
					.top = 4,
					.right = 7,
				},
				.anchor = { 0.5f, 0.5f },
				.layout = presets::keypad_layout,
				.navigation_key = presets::TypewriterPart::NavigationKey::Enter,
				.font = settings.font,
				.debug_visualized = settings.debug_visualized,
			}),
			left_part_rigid_dynamic{
				physics_engine->create_rigid_dynamic(
					left_part_transform.get_global_matrix(),
					physics_engine->create_shape(
						arx::PhysicsBoxGeometry({ typewriter_part_left.board_extent.x * 0.5f, typewriter_part_left.board_extent.y * 0.5f, settings.key_half_extent.z }),
						glm::translate(glm::mat4{ 1.f }, -typewriter_part_left.real_anchor + glm::vec3(typewriter_part_left.board_extent * 0.5f, settings.key_half_extent.z))
					)
				),
				&left_part_transform,
				true,
				false,
			},
			right_part_rigid_dynamic{
				physics_engine->create_rigid_dynamic(
					right_part_transform.get_global_matrix(),
					physics_engine->create_shape(
						arx::PhysicsBoxGeometry({ typewriter_part_right.board_extent.x * 0.5f, typewriter_part_right.board_extent.y * 0.5f, settings.key_half_extent.z }),
						glm::translate(glm::mat4{ 1.f }, -typewriter_part_right.real_anchor + glm::vec3(typewriter_part_right.board_extent * 0.5f, settings.key_half_extent.z))
					)
				),
				&right_part_transform,
				true,
				false,
			},
			keypad_rigid_dynamic{
				physics_engine->create_rigid_dynamic(
					keypad_transform.get_global_matrix(),
					physics_engine->create_shape(
						arx::PhysicsBoxGeometry({ typewriter_keypad.board_extent.x * 0.5f, typewriter_keypad.board_extent.y * 0.5f, settings.key_half_extent.z }),
						glm::translate(glm::mat4{ 1.f }, -typewriter_keypad.real_anchor + glm::vec3(typewriter_keypad.board_extent * 0.5f, settings.key_half_extent.z))
					)
				),
				&keypad_transform,
				true,
				false,
			},
			left_part_grab_interactable{ &left_part_rigid_dynamic, true },
			right_part_grab_interactable{ &right_part_rigid_dynamic, true },
			keypad_grab_interactable{ &keypad_rigid_dynamic, true }
		{

		}

	public:
		SpaceTransform left_part_transform;
		SpaceTransform right_part_transform;
		SpaceTransform keypad_transform;
		presets::TypewriterPart typewriter_part_left;
		presets::TypewriterPart typewriter_part_right;
		presets::TypewriterPart typewriter_keypad;
		RigidDynamicComponent left_part_rigid_dynamic;
		RigidDynamicComponent right_part_rigid_dynamic;
		RigidDynamicComponent keypad_rigid_dynamic;
		XRGrabInteractable left_part_grab_interactable;
		XRGrabInteractable right_part_grab_interactable;
		XRGrabInteractable keypad_grab_interactable;
	};
}