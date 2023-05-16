#pragma once

namespace arx
{
	struct CommandASTPrinter
	{
		auto operator<<(const CommandASTStatementNode& statement) -> CommandASTPrinter& {
			print_indent(0);
			print_statement(statement, 0);
			return *this;

		}

		auto print_statement(const CommandASTStatementNode& statement, uint32_t indent) -> void {
			std::cout << "[statement]:" << std::endl;
			if (statement.type == CommandASTStatementNode::Type::Empty) {
				// do nothing.
			}
			else if (statement.type == CommandASTStatementNode::Type::Expression) {
				print_indent(indent + 1);
				std::cout << "expression = ";
				print_expression(std::get<CommandASTExpressionNode>(statement.value), indent + 1);
			}
		}

		auto print_expression(const CommandASTExpressionNode& expression, uint32_t indent) -> void {
			std::cout << std::format("[expression, type = {}]:", to_string(expression.type)) << std::endl;
			switch (expression.type)
			{
			case arx::CommandASTExpressionNode::Type::Empty:
				break;
			case arx::CommandASTExpressionNode::Type::Number:
			{
				print_indent(indent + 1);
				std::cout << "value = " << std::get<CommandASTNumberNode>(expression.value).value << std::endl;
				break;
			}
			case arx::CommandASTExpressionNode::Type::String:
			{
				print_indent(indent + 1);
				std::cout << "value = \"" << std::get<CommandASTStringNode>(expression.value).value << "\"" << std::endl;
				break;
			}
			case arx::CommandASTExpressionNode::Type::Identifier:
			{
				print_indent(indent + 1);
				std::cout << "name = " << std::get<CommandASTIdentifierNode>(expression.value).name << std::endl;
				break;
			}
			case arx::CommandASTExpressionNode::Type::Operation:
			{
				auto& operation = std::get<CommandASTOperationNode>(expression.value);
				print_indent(indent + 1);
				std::cout << "operator = " << to_string(operation.type) << std::endl;
				print_indent(indent + 1);
				std::cout << "operand_count = " << operation.operand_count << std::endl;
				for (size_t i = 0; i < operation.operands.size(); ++i) {
					print_indent(indent + 1);
					std::cout << std::format("operand_{} = ", i);
					print_expression(operation.operands[i], indent + 1);
				}
				break;
			}
			case arx::CommandASTExpressionNode::Type::List:	
			{
				auto& list = std::get<CommandASTListNode>(expression.value);
				for (size_t i = 0; i < list.expressions.size(); ++i) {
					print_indent(indent + 1);
					std::cout << std::format("element_{} = ", i);
					print_expression(list.expressions[i], indent + 1);
				}
				break;
			}
			case arx::CommandASTExpressionNode::Type::Parentheses:
			{
				print_indent(indent + 1);
				std::cout << "expression = ";
				print_expression(*(std::get<CommandASTParenthesesNode>(expression.value).expression), indent + 1);
				break;
			}
			case arx::CommandASTExpressionNode::Type::FunctionCall:
			{
				auto& function_call = std::get<CommandASTFunctionCallNode>(expression.value);
				print_indent(indent + 1);
				std::cout << "function = ";
				print_expression(*(function_call.function_body), indent + 1);
				print_indent(indent + 1);
				std::cout << "argument = ";
				print_expression(*(function_call.argument), indent + 1);
				break;
			}
			case arx::CommandASTExpressionNode::Type::FunctionBody:
			{
				auto& function_body = std::get<CommandASTFunctionBodyNode>(expression.value);
				for (size_t i = 0; i < function_body.commands.size(); ++i) {
					print_indent(indent + 1);
					std::cout << std::format("statement_{} = ", i);
					print_statement(function_body.commands[i], indent + 1);
				}
				break;
			}
			case arx::CommandASTExpressionNode::Type::Assignment:
			{
				auto& assignment = std::get<CommandASTAssignmentNode>(expression.value);
				print_indent(indent + 1);
				std::cout << "identifier = " << assignment.name << std::endl;
				print_indent(indent + 1);
				std::cout << "expression = ";
				print_expression(*(assignment.expression), indent + 1);
				break;
			}
			case arx::CommandASTExpressionNode::Type::Protection:
			{
				print_indent(indent + 1);
				std::cout << "identifier = " << std::get<CommandASTProtectionNode>(expression.value).name << std::endl;
				break;
			}
			case arx::CommandASTExpressionNode::Type::Delete:
			{
				print_indent(indent + 1);
				std::cout << "identifier = " << std::get<CommandASTDeleteNode>(expression.value).name << std::endl;
				break;
			}
			case arx::CommandASTExpressionNode::Type::Argument:
			{
				break;
			}
			case arx::CommandASTExpressionNode::Type::Return:
			{
				print_indent(indent + 1);
				std::cout << "expression = ";
				print_expression(*(std::get<CommandASTReturnNode>(expression.value).expression), indent + 1);
				break;
			}
			default:
			{
				break;
			}
			}
		}

		auto print_indent(uint32_t indent) -> void {
			for (uint32_t i = 0; i < indent; ++i) {
				std::cout << "| ";
			}
			std::cout << "|-";
		}
	};

	struct CommandASTPrinterRuntime
	{
		CommandASTPrinter printer;
		CommandParser<CommandASTPrinter> parser;
		CommandLexer<CommandASTPrinter> lexer;

		CommandASTPrinterRuntime() : printer{}, parser{ printer }, lexer{ parser } {

		}

		auto run() -> void {
			while (true) {
				std::string line;
				std::getline(std::cin, line);
				if (std::cin.eof()) {
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
				std::cerr << exception.what() << std::endl;
				parser.awaiting_expression = CommandASTExpressionNode::make_empty();
				while (parser.processing_nodes.size() > 1) {
					parser.processing_nodes.pop();
				}
			}
		}
	};
}