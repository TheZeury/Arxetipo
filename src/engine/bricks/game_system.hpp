#pragma	once

namespace arx
{
	template<typename S>
	concept System = requires(S s) {
		s.mobilize();
		s.freeze();
		s.update();
	};

	template<typename SystemContainer, typename SystemName>
	concept Contains = requires(SystemContainer systems) {
		{ systems.template get<SystemName>() } -> std::convertible_to<SystemName*>;
	};

	template<typename... types>
	struct StaticSystemComposition;

	template<>
	struct StaticSystemComposition<>
	{

	};

	template<typename This, typename... Rest>
	requires System<This> && (System<Rest> && ...)
	struct StaticSystemComposition<This, Rest...>
	{
	public:
		StaticSystemComposition(This* this_system, Rest*... rests) : system{ this_system }, others{ rests } {
		}
		~StaticSystemComposition() = default;
		This* system;

		auto mobilize() -> void {
			system->mobilize();
			if constexpr (sizeof...(Rest) > 0) {
				others.mobilize();
			}
		}

		auto freeze() -> void {
			system->freeze();
			if constexpr (sizeof...(Rest) > 0) {
				others.freeze();
			}
		}

		auto update() -> void {
			system->update();
			if constexpr (sizeof...(Rest) > 0) {
				others.update();
			}
		}

	public:
		template<typename S>
		requires std::same_as<S, This> || (std::same_as<S, Rest> || ...)
		constexpr auto get() noexcept -> S* {
			if constexpr (std::is_same_v<S, This>) {
				return system;
			}
			else {
				return others.template get<S>();
			}
		}

	private:
		StaticSystemComposition<Rest...> others;
	};

	struct DynamicSystem
	{
	public: // concept: System.
		auto mobilize() -> void {
			(this->*mobilize_function)();
		}
		auto freeze() -> void {
			(this->*freeze_function)();
		}
		auto update() -> void {
			(this->*update_function)();
		}

	public:
		void* system;
		std::type_index type;

		template<typename S>
		requires System<S>
		DynamicSystem(S* system) : system{ system }, type{ typeid(S) } {
			mobilize_function = &DynamicSystem::mobilize_internal<S>;
			freeze_function = &DynamicSystem::freeze_internal<S>;
			update_function = &DynamicSystem::update_internal<S>;
		}
		DynamicSystem() = delete;

		template<typename S>
		auto get_system() -> S* {
			if (std::type_index{ typeid(S) } == type) {
				return reinterpret_cast<S*>(system);
			}
			return nullptr;
		}

	private:
		auto (DynamicSystem::* mobilize_function)() -> void;
		auto (DynamicSystem::* freeze_function)() -> void;
		auto (DynamicSystem::* update_function)() -> void;

		template<typename S>
		auto mobilize_internal() -> void {
			reinterpret_cast<S*>(system)->mobilize();
		}
		template<typename S>
		auto freeze_internal() -> void {
			reinterpret_cast<S*>(system)->freeze();
		}
		template<typename S>
		auto update_internal() -> void {
			reinterpret_cast<S*>(system)->update();
		}
	};

	struct DynamicSystemComposition
	{
	public:	// concept: System.
		auto mobilize() -> void {
			for (auto& system : systems) {
				system.second.mobilize();
			}
		}
		auto freeze() -> void {
			for (auto& system : systems) {
				system.second.freeze();
			}
		}
		auto update() -> void {
			for (auto& system : systems) {
				system.second.update();
			}
		}

	public:
		std::map<std::type_index, DynamicSystem> systems;

		template<typename... types>
		DynamicSystemComposition(StaticSystemComposition<types...>* static_systems) {
			(add_system(static_systems->template get<types>()), ...);
		}

		template<typename S> requires System<S>
		auto add_system(S* system) -> void {
			systems[std::type_index{ typeid(S) }] = DynamicSystem{ system };
		}

		template<typename S> requires System<S>
		auto get() noexcept -> S* {
			auto it = systems.find(std::type_index{ typeid(S) });
			if (it != systems.end()) {
				return it->second.get_system<S>();
			}
			return nullptr;
		}
	};
}