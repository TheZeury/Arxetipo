#pragma once

namespace arx
{
	struct CommandRuntime {
		std::istream& input;
		std::ostream& output;
		std::ostream& error;

		CommandKernel kernel;
		CommandParser<CommandKernel> parser;
		CommandLexer<CommandKernel> lexer;

		bool exit = false;

		CommandRuntime(std::istream& input, std::ostream& output, std::ostream& error = std::cerr) : input{ input }, output{ output }, error{ error }, kernel{}, parser{ kernel }, lexer{ parser } {
			kernel.add_method("print", [&](const std::vector<CommandValue>& arguments, CommandValue& result) {
				for (const auto& argument : arguments) {
					output << argument.to_string() << std::endl;
				}
				result = CommandValue{ CommandValue::Type::Empty, std::monostate{} };
			}, true);
			kernel.add_method("exit", [&](const std::vector<CommandValue>& arguments, CommandValue& result) {
				exit = true;
				result = CommandValue{ CommandValue::Type::Empty, std::monostate{} };
			}, true);
		}

		auto run() -> void {
			while (!exit) {
				try {
					output.flush();
					std::string line;
					std::getline(input, line);
					if (input.eof()) {
						break;
					}
					lexer << line;
				}
				catch (const CommandException& exception) {
					error << exception.what() << std::endl;
					while (parser.awaiting_nodes.size() > 1) {
						parser.awaiting_nodes.pop();
					}
					while (parser.processing_nodes.size() > 1) {
						parser.processing_nodes.pop();
					}
					while (kernel.scope_stack.size() > 1) {
						kernel.scope_stack.pop_back();
					}
				}
			}
		}
	};
}