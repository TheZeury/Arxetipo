#pragma once

namespace arx
{
	struct CommandSystem
	{
	public: // concept: System
		template<typename System>
		requires std::same_as<System, CommandSystem>
		auto get() noexcept -> System* {
			return this;
		}
		auto mobilize() -> void {
			mobilized = true;
		}
		auto freeze() -> void {
			mobilized = false;
		}
		auto update() -> void {
			if (mobilized && (commands != "")) {
				command_runtime->run_code(commands);
				commands = "";
			}
		}

	public:
		auto command(const std::string& command) -> void {
			commands += command + ' ';
		}

	public:
		CommandSystem(CommandRuntime* command_runtime) : command_runtime(command_runtime) {
		}

	public:
		std::string commands = "";
		bool mobilized = false;
		CommandRuntime* command_runtime;
	};

	template<typename Systems>
	concept ContainsCommandSystem = Contains<Systems, CommandSystem>;
}