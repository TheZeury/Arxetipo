#pragma once

#include <string>
#include <vector>
#include <stack>
#include <variant>
#include <string_view>
#include <stdexcept>
#include <format>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <optional>

namespace arx 
{
	struct CommandException : public std::exception
	{
		template<typename... Args>
		CommandException(const std::format_string<Args...> message, Args&... args) : message{ std::format<Args...>(message, std::forward<Args>(args)...) } {
		}

		const char* what() const noexcept override {
			return message.c_str();
		}

	private:
		std::string message;
	};
}

#include "command_ast.hpp"
#include "command_formatter.hpp"
#include "command_kernel.hpp"
#include "command_reader.hpp"
#include "command_parser.hpp"
#include "command_runtime.hpp"