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
			kernel.add_function("print", [&](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				for (const auto& argument : arguments) {
					output << argument.to_string() << std::endl;
				}
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, std::monostate{} };
				}
				return 0;
			}, true);
			kernel.add_function("exit", [&](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				exit = true;
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, std::monostate{} };
				}
				return 0;
			}, true);
		}

		auto run() -> void {
			while (!exit) {
				output.flush();
				std::string line;
				std::getline(input, line);
				if (input.eof()) {
					break;
				}
				run_code(line);
			}
		}

		auto run_code(const std::string& code) -> void {
			try {
				lexer << code;
			}
			catch (const CommandException& exception) {
				error << exception.what() << std::endl;
				parser.awaiting_expression = CommandASTExpressionNode::make_empty();
				while (parser.processing_nodes.size() > 1) {
					parser.processing_nodes.pop();
				}
				while (kernel.scope_stack.size() > 1) {
					kernel.scope_stack.pop_back();
				}
			}
		}
	};
}