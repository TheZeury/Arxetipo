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

		CommandRuntime(std::istream& input, std::ostream& output, std::ostream& error = std::cerr) : input{ input }, output{ output }, error{ error }, kernel{ }, parser{ kernel }, lexer{ parser } {
		}

		auto load_library(CommandLibrary&& library) -> void {
			library.load_to(kernel);
		}

		auto run() -> int {
			while (!exit) {
				output.flush();
				std::string line;
				std::getline(input, line);
				run_code(line);
				if (input.eof()) {
					break;
				}
			}
			return 0;
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