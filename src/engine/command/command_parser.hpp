#pragma once

namespace arx
{
	struct CommandToken
	{
		enum class Type
		{
			None,
			Name,
			Number,
			Separator,
			Operator,
			Eof,
		};
		Type type;
		std::string value;
	};

	const std::unordered_map<std::string, int32_t> operator_precedence = {
		{ ",", 0 },
		{ ";", 0 },
		{ "+", 1 },
		{ "-", 1 },
		{ "*", 2 },
		{ "/", 2 },
		{ "'+", 3 },
		{ "'-", 3 },
	};

	template<typename I>
	struct CommandParser
	{
		/// <summary>
		/// Awaiting nodes are completed by themselves, but waiting to joint to their parent nodes.
		/// Possible nodes that wait to join to their parent nodes: indentifier(waiting to become a method name or an independent expression)
		/// </summary>
		std::stack<CommandASTNode> awaiting_nodes;

		/// <summary>
		/// Processing node are uncompleted and waiting for their child nodes to be completed.
		/// Possible node that waits for child nodes: method body(waiting for child statements), method call(waiting for arguments).
		/// </summary>
		std::stack<CommandASTNode> processing_nodes;

		auto operator<<(const CommandToken& token) -> CommandParser& {
			switch (token.type)
			{
			case CommandToken::Type::None: {
				// Placeholder.
				break;
			}
			case CommandToken::Type::Name: {
				awaiting_nodes.push(CommandASTNode::make_identifier(token.value));
				break;
			}
			case CommandToken::Type::Number: {
				float value = std::stof(token.value);
				awaiting_nodes.push(CommandASTNode::make_number(std::stof(token.value)));
				break;
			}
			case CommandToken::Type::Separator: {
				if (token.value == ",") {
					auto submitted = submit_preceding_operations(",");

					if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
						throw CommandException("Unexpected comma`,`. Comma`,` can only be used to terminate expression.");
					}

					auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
					if (processing_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& parent_expression = std::get<CommandASTExpressionNode>(processing_nodes.top().value);
						if (parent_expression.type == CommandASTExpressionNode::Type::MethodCall) {
							auto& method_call = std::get<CommandASTMethodCallNode>(parent_expression.value);
							method_call.arguments.push_back(std::move(expression)); // TODO try to not move;
							awaiting_nodes.pop();
						}
						else {
							throw CommandException("Unsupport usage of comma`,`. Comma`,` can only be used to separate method arguments at this point.\n Future feature: tuple");
						}
					}
					else {
						throw CommandException("Unsupport usage of comma`,`. Comma`,` can only be used to separate method arguments at this point.");
					}
				}
				else if (token.value == "(") {
					if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
						if (expression.type == CommandASTExpressionNode::Type::Identifier) { // Turn identifier into method call.
							std::string name = std::get<CommandASTIdentifierNode>(expression.value).name;
							awaiting_nodes.pop();
							processing_nodes.push(CommandASTNode::make_method_call(name, { }));
							awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
						}
						else {
							throw CommandException("Unsupported parentheses usage. Identifier is the only expression type that can be used as a method name at this point.");
						}
					}
					else {
						processing_nodes.push(CommandASTNode::make_parentheses({ }));
						awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
					}
				}
				else if (token.value == ")") {
					auto submitted = submit_preceding_operations(","); // As if a comma(,) is encountered.
					
					if (processing_nodes.top().type != CommandASTNode::Type::Expression) {
						throw CommandException("Unexpected close parentheses`)`. Close parentheses`)` appeared but there's nothing to be terminated.");
					}

					auto& parent_expression = std::get<CommandASTExpressionNode>(processing_nodes.top().value);
					if (parent_expression.type == CommandASTExpressionNode::Type::MethodCall) {
						auto& method_call = std::get<CommandASTMethodCallNode>(parent_expression.value);
						if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
							auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
							method_call.arguments.push_back(std::move(expression));
							awaiting_nodes.pop();
						}
						if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
							throw CommandException("Incorrect method call. Multiple arguments must be separated by comma`,`.");
						}
						awaiting_nodes.pop(); // Remove the barrier None node.
						awaiting_nodes.push(std::move(processing_nodes.top())); // TODO. try not move.
						processing_nodes.pop();
					}
					else if (parent_expression.type == CommandASTExpressionNode::Type::Parentheses) {
						auto& parentheses = std::get<CommandASTParenthesesNode>(parent_expression.value);

						if (awaiting_nodes.top().type == CommandASTNode::Type::None) {
							awaiting_nodes.pop(); // Remove barrier.
							awaiting_nodes.push(CommandASTNode{ CommandASTNode::Type::Expression,
								CommandASTExpressionNode { CommandASTExpressionNode::Type::Empty, { } }
							});
							processing_nodes.pop();
						}
						else if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
							auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
							parentheses.expressions.push_back(std::move(expression)); // TODO. try not move.
							awaiting_nodes.pop();
							if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
								throw CommandException("Incorrect expression. Only one expression can be put in a pair of parenthesis.");
							}
							awaiting_nodes.pop(); // Remove the barrier None node.
							awaiting_nodes.push(std::move(processing_nodes.top())); // TODO. try not move.
							processing_nodes.pop();
						}
					}
					else {
						throw CommandException("Unexpected close parenthesis`)`. Close parenthesis`)` appeared but there's nothing to be terminated.");
					}
				}
				else if (token.value == ";") {
					submit_preceding_operations(";");
					CommandASTStatementNode statement{ { }, { } };
					if (processing_nodes.top().type == CommandASTNode::Type::Statement) {
						statement = std::move(std::get<CommandASTStatementNode>(processing_nodes.top().value));
						if (statement.type == CommandASTStatementNode::Type::Assignment) {
							auto& assignment = std::get<CommandASTAssignmentNode>(statement.value);
							if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
								auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
								assignment.expression = std::move(expression);
								awaiting_nodes.pop();
							
								if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
									throw CommandException("Only one expression can be assigned.");
								}
								awaiting_nodes.pop(); // Remove barrier.
								processing_nodes.pop();
							}
							else if (awaiting_nodes.top().type == CommandASTNode::Type::None) { // None is not a legal node in AST, but a barrier for parser.
								throw CommandException("Assignment terminated but no expression found. \nHint: If you want to assign an empty value to a non-existing indetifier, simply use `name;`");
							}
							else {
								throw CommandException("Assigning a non-expression node is not allowed.");
							}
						}
						else {
							throw CommandException("Unsupported statement type.");
						}
					}
					else if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
						statement = CommandASTStatementNode{ CommandASTStatementNode::Type::Expression, std::move(expression) };
						awaiting_nodes.pop();
					}
					else if (awaiting_nodes.top().type == CommandASTNode::Type::None) {
						throw CommandException("Unexpected semicolon`;`. Empty statement is not allowed.");
					}
					else {
						throw CommandException("Unexpected semicolon`;`.");
					}

					if (processing_nodes.size() == 1) {
						interpreter << statement;
					}
					else {
						throw CommandException("Unsupported syntex. A Statemet must be at top level at this point.");
					}
				}
				else {
					throw CommandException("Unexpected separator token`{}`.", token.value);
				}
				break;
			}
			case CommandToken::Type::Operator: {
				auto op = token.value;
				if (op == "=") {
					if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
						throw CommandException("Assigment`=` must follow a expression.");
					}
					auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);

					if (expression.type == CommandASTExpressionNode::Type::Identifier) {
						std::string name = std::get<CommandASTIdentifierNode>(expression.value).name;
						awaiting_nodes.pop();
						processing_nodes.push(CommandASTNode::make_assignment(name, { { }, { } }));
						awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
					}
					else {
						throw CommandException("Unsupported assignment`=` usage. Assignment`=` must follow a indentifier name at this point.\n Future feature: tuple");
					}
				}
				else if (op == "^" || op == "%") {
					throw CommandException("Operators power`^` and modulo`%` are not supported yet.");
				}
				else if ((op == "+" || op == "-") && awaiting_nodes.top().type == CommandASTNode::Type::None) {
					op = "'" + op; // '+ and '-
					processing_nodes.push(CommandASTNode::make_operation(to_operation_type(op), 1, { }));
					awaiting_nodes.push(CommandASTNode::make_none());
				}
				else { // + - * /
					submit_preceding_operations(op);
					if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
						throw CommandException("Left hand operant of {} not existing.", op);
					}
					processing_nodes.push(CommandASTNode::make_operation(to_operation_type(op), 2, {  }));
					auto& operation = std::get<CommandASTOperationNode>(std::get<CommandASTExpressionNode>(processing_nodes.top().value).value);
					operation.operands.push_back(std::move(std::get<CommandASTExpressionNode>(awaiting_nodes.top().value)));
					awaiting_nodes.pop();
					awaiting_nodes.push(CommandASTNode::make_none());
				}
				break;
			}
			case CommandToken::Type::Eof: {
				break;
			}
			default:
				break;
			}
			return *this;
		}

		auto submit_preceding_operations(const std::string& operation) -> uint32_t {
			uint32_t count = 0;
			while (!processing_nodes.empty()) {
				if (processing_nodes.top().type != CommandASTNode::Type::Expression) {
					break;
				}
				auto& expression = std::get<CommandASTExpressionNode>(processing_nodes.top().value);
				if (expression.type != CommandASTExpressionNode::Type::Operation) {
					break;
				}
				auto& last_operation = std::get<CommandASTOperationNode>(expression.value);
				auto str_op = to_string(last_operation.type);
				if (prior_to(operation, str_op)) {
					break;
				}

				if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
					throw CommandException("Operator {} can only operate on expression.", str_op);
				}
				last_operation.operands.push_back(std::move(std::get<CommandASTExpressionNode>(awaiting_nodes.top().value)));
				awaiting_nodes.pop();
				
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Operants to {} are more than expected.", str_op);
				}
				awaiting_nodes.pop(); // Remove barrier
				awaiting_nodes.push(std::move(processing_nodes.top()));
				processing_nodes.pop();
				++count;
			}
			return count;
		}

		auto prior_to(const std::string& a, const std::string& b) -> bool {
			if (!operator_precedence.contains(a)) {
				throw CommandException("Operator`{}` is not supported.", a);
			}
			if (!operator_precedence.contains(b)) {
				throw CommandException("Operator`{}` is not supported.", b);
			}
			return operator_precedence.at(a) > operator_precedence.at(b);
		}

		CommandParser(I& interpreter) : interpreter(interpreter) {
			awaiting_nodes.push(CommandASTNode::make_none());
			processing_nodes.push(CommandASTNode::make_none());
		}

		I& interpreter;
	};

	template<typename I>
	struct CommandLexer
	{
		auto get_tokens(CommandParser<I>& parser, const std::string& input) {
			for (auto iter = input.begin(); iter != input.end(); ++iter) {
				char c = *iter;
				if (is_empty(c) || is_eof(c)) {
					continue;
				}
				if (is_letter(c)) {
					std::string name;
					while (is_letter(c) || is_digit(c)) {
						name += c;
						if (++iter == input.end()) {
							break;
						}
						c = *iter;
					}
					--iter;
					parser << CommandToken{ CommandToken::Type::Name, name };
				}
				else if (is_digit(c)) {
					std::string number;
					while (is_digit(c)) {
						number += c;
						if (++iter == input.end()) {
							break;
						}
						c = *iter;
					}
					--iter;
					parser << CommandToken{ CommandToken::Type::Number, number };
				}
				else if (is_separator(c)) {
					parser << CommandToken{ CommandToken::Type::Separator, std::string(1, c) };
				}
				else if (is_operator(c)) {
					parser << CommandToken{ CommandToken::Type::Operator, std::string(1, c) };
				}
				else if (is_eof(c)) {
					parser << CommandToken{ CommandToken::Type::Eof, std::string(1, c) };
				}
				else {
					throw CommandException("Unexpected character`{}`.", c);
				}
			}
		}

		friend auto operator>>(std::istream& is, CommandLexer& lexer) -> std::istream& {
			std::string line;
			while (std::getline(is, line)) {
				lexer.get_tokens(lexer.parser, line);
			}
			return is;
		}

		auto operator<<(const std::string& text) -> CommandLexer& {
			get_tokens(parser, text);
			return *this;
		}

		CommandLexer(CommandParser<I>& parser) : parser(parser) {
		}

		CommandParser<I>& parser;

	private:
		auto is_empty(char c) -> bool {
			return c == ' ' || c == '\t' || c == '\n' || c == '\r';
		}
		auto is_letter(char c) -> bool {
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		}
		auto is_digit(char c) -> bool {
			return (c >= '0' && c <= '9') || (c == '.');
		}
		auto is_separator(char c) -> bool {
			return c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == ';';
		}
		auto is_operator(char c) -> bool {
			return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '=';
		}
		auto is_eof(char c) -> bool {
			return c == '\0';
		}
	};
}