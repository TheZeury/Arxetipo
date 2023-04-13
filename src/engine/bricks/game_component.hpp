#pragma once

namespace arx
{

	template<typename C> // C for Component & Composition.
	concept Registrable = requires(C c, DynamicSystemComposition* example) {
		c.template register_to_systems<DynamicSystemComposition>(example);
	};

	template<typename... types>
	struct StaticComponentComposition;

	template<>
	struct StaticComponentComposition<>
	{

	};

	template<typename This, typename... Rest>
	//	requires Registrable<This> && (Registrable<Rest> && ...)
	struct StaticComponentComposition<This, Rest...>
	{
	public:
		StaticComponentComposition(This this_conponent, Rest... rests) : component{ this_conponent }, others{ rests } {
		}
		~StaticComponentComposition() = default;

		template<typename S>
		auto register_to_systems(S* systems) -> void {
			component.register_to_systems(systems);
			if constexpr (sizeof...(Rest) > 0) {
				StaticComponentComposition<Rest...>::register_to_systems(systems);
			}
		}

		template<typename Component>
		requires std::same_as<Component, This> || (std::same_as<Component, Rest> || ...)
		constexpr auto get() noexcept -> Component {
			if constexpr (std::is_same_v<Component, This>) {
				return component;
			}
			else {
				return others.template get<Component>();
			}
		}

		This component;
	private:
		StaticComponentComposition<Rest...> others;
	};

	struct DynamicComponent
	{
		void* component;
		std::type_index type;

		template<typename Component> requires Registrable<Component>
		constexpr DynamicComponent(Component* component) :
			component{ reinterpret_cast<void*>(component) }, type{ typeid(Component) },
			register_function{ &DynamicComponent::register_component<Component> } {
		}
		DynamicComponent() = delete;

		auto register_to_systems(DynamicSystemComposition* systems) -> void {
			(this->*(this->register_function))(systems);
		}

		template<typename Component> requires Registrable<Component>
		auto get_component() noexcept -> Component* {
			if (std::type_index{ typeid(Component) } == type) {
				return reinterpret_cast<Component*>(component);
			}
			return nullptr;
		}

	private:
		auto (DynamicComponent::* register_function)(DynamicSystemComposition*) -> void;

		template<typename Component>
		auto register_component(DynamicSystemComposition* systems) -> void {
			reinterpret_cast<Component*>(component)->register_to_systems(systems);
		}
	};

	struct DynamicComponentComposition
	{
		std::vector<DynamicComponent> components;

		template<typename S>
		auto register_to_systems(S* systems) -> void {
			auto dynamic_systems = DynamicSystemComposition(systems);
			for (auto& component : components) {
				component.register_to_systems(&dynamic_systems);
			}
		}
		auto add_component(DynamicComponent component) -> void {
			components.push_back(component);
		}

		template<typename Component> requires Registrable<Component>
		auto add_component(Component* component) -> void {
			components.push_back(DynamicComponent{ component });
		}

		template<typename Component> requires Registrable<Component>
		auto get() noexcept -> Component* {
			for (auto& component : components) {
				if (auto c = component.get_component<Component>(); c != nullptr) {
					return c;
				}
			}
			return nullptr;
		}
	};
}