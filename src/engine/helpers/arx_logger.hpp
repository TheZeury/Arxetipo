#if !defined(ARX_MODULE)
#pragma once
#include <iostream>
#endif

namespace arx
{
	auto inline log_step(const std::string& instance, const std::string& step) -> void {
		std::cout << "\033[0m[\033[1;36mSTEP\033[0m] " << "\033[1;34m" + instance + "\t\033[0m" + step + "...\t";
	}

	auto inline log_success() -> void {
		std::cout << "\033[1;32mSuccess\033[0m\n";
	}

	auto inline log_failure() -> void {
		std::cout << "\033[1;31mFailure\033[0m\n";
	}

	auto inline log_info(const std::string& instance, const std::string& information, size_t table) -> void {
		std::cout << "\033[0m[\033[1;33mINFO\033[0m] " << "\033[1;34m" + instance;
		while (table--) {
			std::cout << '\t';
		}
		std::cout << "\t\033[0m" + information << "\n";
	}

	auto inline log_error(const std::string& errorMessage) -> void {
		std::cout << "\033[0m[\033[1;31mERRO\033[0m] " << errorMessage << std::endl;
	}
}