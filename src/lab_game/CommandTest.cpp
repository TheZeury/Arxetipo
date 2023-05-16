#include "../engine/command/command.hpp"

auto main() -> int {
	arx::CommandRuntime runtime{ std::cin, std::cout };
	//arx::CommandASTPrinterRuntime runtime;
	runtime.run();
}