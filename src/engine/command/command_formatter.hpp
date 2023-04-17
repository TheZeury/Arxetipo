#pragma once

namespace arx
{
	struct CommandFormatter
	{
		auto operator<<(const CommandASTStatementNode& statement) -> CommandFormatter& {
			switch (statement.type) {
			case CommandASTStatementNode::Type::Expression: {
				excute_expression(std::get<CommandASTExpressionNode>(statement.value));
				break;
			}
			case CommandASTStatementNode::Type::Assignment: {
				excute_assignment(std::get<CommandASTAssignmentNode>(statement.value));
				break;
			}
			default: {
				throw CommandException("Unexcutable statement type.");
				break;
			}
			}
			std::cout << ';' << std::endl;
			return *this;
		}

		auto excute_expression(const CommandASTExpressionNode& expression) -> void {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Empty: {
				std::cout << "()";
				break;
			}
			case CommandASTExpressionNode::Type::Number: { // Maybe `Literal` is a better name.
				std::cout << std::get<CommandASTNumberNode>(expression.value).value;
				break;
			}
			case CommandASTExpressionNode::Type::Identifier: {
				std::cout << std::get<CommandASTIdentifierNode>(expression.value).name;
				break;
			}
			case CommandASTExpressionNode::Type::MethodCall: {
				excute_expression(*std::get<CommandASTMethodCallNode>(expression.value).method_body);
				std::cout << '(';
				for (auto& argument : std::get<CommandASTMethodCallNode>(expression.value).arguments) {
					excute_expression(argument);
					std::cout << ", ";
				}
				if (std::get<CommandASTMethodCallNode>(expression.value).arguments.size()) {
					std::cout << "\b\b";
				}
				std::cout << ')';
				break;
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				std::cout << '(';
				excute_expression(std::get<CommandASTParenthesesNode>(expression.value).expressions[0]);
				std::cout << ')';
				break;
			}
			case CommandASTExpressionNode::Type::Operation: {
				const auto& operation = std::get<CommandASTOperationNode>(expression.value);
				if ((operation.type == CommandASTOperationNode::Type::Positive) || (operation.type == CommandASTOperationNode::Type::Negative)) {
					std::cout << ((operation.type == CommandASTOperationNode::Type::Positive) ? "+" : "-");
					excute_expression(operation.operands[0]);
				} 
				else {
					excute_expression(operation.operands[0]);
					std::cout << ' ' << to_string(operation.type) << ' ';
					excute_expression(operation.operands[1]);
				}
				break;
			}
			default: {
				throw CommandException("Unexcutable expression type.");
				break;
			}
			}
		}

		auto excute_assignment(const CommandASTAssignmentNode& assignment) -> void {
			std::cout << assignment.name << " = ";
			excute_expression(assignment.expression);
		}
	};
}