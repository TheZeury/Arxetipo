#include "../engine/command/command.hpp"

#include <fstream>

auto main(int argument_count, char* arguments[]) -> int {
	bool ast_mode = false;
	std::unordered_set<std::string> libraries = { "basic" };

	std::ifstream file;

	for (int i = 1; i < argument_count; ++i) {
		std::string_view argument{ arguments[i] };

		if (argument[0] != '-') {
			file.open(argument.data());
			break;
		}
		else if (argument == "-h" || argument == "--help") {
			std::cout << "Usage: arxemand [options] [file] [script arguments]" << std::endl;
			std::cout << "Options:" << std::endl;
			std::cout << "  -h, --help\t\t\tShow this help message and exit" << std::endl;
			std::cout << "  -a, --ast\t\t\tPrint the AST of the program and exit" << std::endl;
			std::cout << "  -n, --no-basic\t\tDo not load the basic library" << std::endl;
			std::cout << "  --lib=<name>[,<name>...]\tLoad the specified libraries" << std::endl;
			return 0;
		}
		else if (argument == "-a" || argument == "--ast") {
			ast_mode = true;
		}
		else if (argument == "-n" || argument == "--no-basic") {
			libraries.erase("basic");
		}
		else if (argument.rfind("--lib=", 0) == 0) {
			std::string name;
			for (const char c : argument.substr(6)) {
				if (c == ',') {
					libraries.insert(name);
					name.clear();
				}
				else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
					name += c;
				}
				else {
					std::cerr << "Invalid library name: " << (name += c) << std::endl;
					return 1;
				}
			}
			if (!name.empty()) {
				libraries.insert(name);
			}
		}
	}

	if (ast_mode) {
		arx::CommandASTPrinterRuntime runtime{ file.is_open() ? file : std::cin, std::cout };
		runtime.run();
	} else {
		arx::CommandRuntime runtime{ file.is_open() ? file : std::cin, std::cout };
		for (auto& name : libraries) {
			if (name == "basic") {
				runtime.load_library(arx::CommandLibrary::basic_library(std::cin, std::cout, runtime.kernel, runtime.exit));
			}
			else if (name == "math") {
				runtime.load_library(arx::CommandLibrary::math_library());
			}
			else if (name == "string") {
				runtime.load_library(arx::CommandLibrary::string_library());
			}
			else {
				std::cerr << "Unknown library: " << name << std::endl;
				return 1;
			}
		}
		return runtime.run();
	}
	return 0;
}