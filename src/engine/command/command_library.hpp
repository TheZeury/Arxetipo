#pragma once

namespace arx
{
	struct CommandLibrary
	{
		std::unordered_map<std::string, CommandValue> variables;

		auto add_function(const std::string& name, const std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>& function) -> void {
			variables[name] = CommandValue(CommandValue::Type::Function, function);
		}
		auto add_macro(const std::string& name, const std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>& function) -> void {
			variables[name] = CommandValue(CommandValue::Type::Macro , function);
		}

		auto load_to(CommandKernel& kernel) -> std::vector<CommandValue> {
			std::vector<CommandValue> failed_identifiers;
			for (auto& [name, value] : variables) {
				auto success = kernel.add_identifier(name, std::move(value), true);
				if (!success) {
					failed_identifiers.push_back(CommandValue{ CommandValue::Type::String, name });
				}
			}
			return failed_identifiers;
		}

		static auto basic_library(std::istream& input, std::ostream& output, CommandKernel& kernel, bool& exit) -> CommandLibrary {
			CommandLibrary library;
			library.add_function("print", [&output](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				for (const auto& argument : arguments) {
					output << argument.to_string() << std::endl;
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, true };
				}
				return 0;
			});
			library.add_function("read", [&input](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				std::string line;
				std::getline(input, line);
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::String, line };
				}
				return 0;
			});
			library.add_function("exit", [&exit](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				exit = true;
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, true };
				}
				return 0;
			});
			library.add_macro("math", [&kernel](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				auto math = math_library();
				auto failed_identifiers = math.load_to(kernel);
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::List, std::move(failed_identifiers) };
				}
				return 0;
			});
			library.add_macro("string", [&kernel](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				auto string = string_library();
				auto failed_identifiers = string.load_to(kernel);
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::List, std::move(failed_identifiers) };
				}
				return 0;
			});
			return library;
		}

		static auto math_library() -> CommandLibrary {
			CommandLibrary library;
			library.add_function("abs", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`abs` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`abs` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::abs(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("round", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`round` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`round` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::round(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("floor", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`floor` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`floor` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::floor(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("ceil", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`ceil` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`ceil` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::ceil(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("sign", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`sign` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`sign` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, !std::signbit(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("sin", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`sin` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`sin` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::sin(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("cos", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`cos` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`cos` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::cos(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("tan", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`tan` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`tan` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::tan(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("asin", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`asin` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`asin` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::asin(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("acos", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`acos` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`acos` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::acos(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("atan", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() == 1) {
					if (arguments[0].type != CommandValue::Type::Number) {
						throw CommandException("`atan` takes a number as argument");
					}
					if (result != nullptr) {
						*result = CommandValue{ CommandValue::Type::Number, std::atan(std::get<float>(arguments[0].value)) };
					}
				}
				else if (arguments.size() == 2) {
					if (arguments[0].type != CommandValue::Type::Number) {
						throw CommandException("`atan` takes a number as argument");
					}
					if (arguments[1].type != CommandValue::Type::Number) {
						throw CommandException("`atan` takes a number as argument");
					}
					if (result != nullptr) {
						*result = CommandValue{ CommandValue::Type::Number, std::atan2(std::get<float>(arguments[0].value), std::get<float>(arguments[1].value)) };
					}
				}
				else {
					throw CommandException("`atan` takes one or two arguments");
				}
				return 0;
			});
			library.add_function("log", [](const std::vector<CommandValue>& arguments, CommandValue* result)->uint32_t {
				if (arguments.size() == 1) {
					if (arguments[0].type != CommandValue::Type::Number) {
						throw CommandException("`log` takes a number as argument");
					}
					if (result != nullptr) {
						*result = CommandValue{ CommandValue::Type::Number, std::log(std::get<float>(arguments[0].value)) };
					}
				}
				else if (arguments.size() == 2) {
					if (arguments[0].type != CommandValue::Type::Number) {
						throw CommandException("`log` takes a number as argument");
					}
					if (arguments[1].type != CommandValue::Type::Number) {
						throw CommandException("`log` takes a number as argument");
					}
					if (result != nullptr) {
						*result = CommandValue{ CommandValue::Type::Number, std::log(std::get<float>(arguments[0].value)) / std::log(std::get<float>(arguments[1].value)) };
					}
				}
				else {
					throw CommandException("`log` takes one or two arguments");
				}
				return 0;
			});
			library.add_function("log2", [](const std::vector<CommandValue>& arguments, CommandValue* result)->uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`log2` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`log2` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::log2(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("log10", [](const std::vector<CommandValue>& arguments, CommandValue* result)->uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`log10` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`log10` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::log10(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			library.add_function("ln", [](const std::vector<CommandValue>& arguments, CommandValue* result)->uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`ln` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::Number) {
					throw CommandException("`ln` takes a number as argument");
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::log(std::get<float>(arguments[0].value)) };
				}
				return 0;
			});
			return library;
		}

		static auto string_library() -> CommandLibrary {
			CommandLibrary library;
			library.add_function("split", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 2) {
					throw CommandException("`split` takes exactly two arguments");
				}
				if (arguments[0].type != CommandValue::Type::String) {
					throw CommandException("`split` takes a string as first argument");
				}
				if (arguments[1].type != CommandValue::Type::String) {
					throw CommandException("`split` takes a string as second argument");
				}
				if (result != nullptr) {
					std::string string = std::get<std::string>(arguments[0].value);
					std::string delimiter = std::get<std::string>(arguments[1].value);
					std::vector<CommandValue> result_vector;
					size_t pos = 0;
					std::string token;
					while ((pos = string.find(delimiter)) != std::string::npos) {
						token = string.substr(0, pos);
						result_vector.push_back(CommandValue{ CommandValue::Type::String, token });
						string.erase(0, pos + delimiter.length());
					}
					result_vector.push_back(CommandValue{ CommandValue::Type::String, string });
					*result = CommandValue{ CommandValue::Type::List, std::move(result_vector) };
				}
				return 0;
			});
			library.add_function("join", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 2) {
					throw CommandException("`join` takes exactly two arguments");
				}
				if (arguments[0].type != CommandValue::Type::List) {
					throw CommandException("`join` takes a list as first argument");
				}
				if (arguments[1].type != CommandValue::Type::String) {
					throw CommandException("`join` takes a string as second argument");
				}
				if (result != nullptr) {
					std::string delimiter = std::get<std::string>(arguments[1].value);
					std::string result_string;
					for (const auto& value : std::get<std::vector<CommandValue>>(arguments[0].value)) {
						result_string += value.to_string() + delimiter;
					}
					result_string.erase(result_string.length() - delimiter.length());
					*result = CommandValue{ CommandValue::Type::String, std::move(result_string) };
				}
				return 0;
			});
			library.add_function("parse", [](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (arguments.size() != 1) {
					throw CommandException("`parse` takes exactly one argument");
				}
				if (arguments[0].type != CommandValue::Type::String) {
					throw CommandException("`parse` takes a string as argument");
				}
				if (result != nullptr) {
					try {
						*result = CommandValue{ CommandValue::Type::Number, std::stof(std::get<std::string>(arguments[0].value)) };
					}
					catch (std::invalid_argument&) {
						throw CommandException("`parse` could not parse string \"{}\" to a number.", std::get<std::string>(arguments[0].value));
					}
				}
				return 0;
			});
			return library;
		}
	};
}