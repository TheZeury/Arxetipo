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
			String,
			Separator,
			Operator,
			Special,
			MethodRelated,
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
		/// Possible nodes that wait to join to their parent nodes: indentifier(waiting to become a method name or an independent expression), expression, statement(waiting to became part of function body).
		/// </summary>
		std::stack<CommandASTNode> awaiting_nodes;

		/// <summary>
		/// Processing node are uncompleted and waiting for their child nodes to be completed.
		/// Possible node that waits for child nodes: method body(waiting for child statements), method call(waiting for arguments), operation(waiting for right operand), parentheses(waiting for expression).
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
			case CommandToken::Type::String: {
				awaiting_nodes.push(CommandASTNode::make_string(token.value));
				break;
			}
			case CommandToken::Type::Separator: {
				parse_separator(token);
				break;
			}
			case CommandToken::Type::Operator: {
				parse_operator(token);
				break;
			}
			case CommandToken::Type::Special: {
				parse_special(token);
				break;
			}
			case CommandToken::Type::MethodRelated: {
				parse_method_related(token);
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

		auto parse_separator(const CommandToken& token) -> void {
			if (token.value == "(" || token.value == ")") {
				parse_parentheses(token);
			}
			else if (token.value == "[" || token.value == "]") {
				parse_brackets(token);
			}
			else if (token.value == "{" || token.value == "}") {
				parse_braces(token);
			}
			else if (token.value == ",") {
				parse_comma(token);
			}
			else if (token.value == ";") {
				parse_semicolon(token);
			}
			else {
				throw CommandException("Unsupported separator token`{}`.", token.value);
			}
		}

		auto parse_operator(const CommandToken& token) -> void {
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
		}

		auto parse_parentheses(const CommandToken& token) -> void {
			if (token.value == "(") {
				if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
					auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
					processing_nodes.push(CommandASTNode::make_method_call(std::make_unique<CommandASTExpressionNode>(std::move(expression)), { }));
					awaiting_nodes.pop();
					awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
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
			else {
				throw CommandException("Internal Parser Error: {} is not a parenthese.", token.value);
			}
		}

		auto parse_brackets(const CommandToken& token) -> void {
			if (token.value == "[") {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Protection is a statement and must be standalone.");
				}
				if (processing_nodes.top().type == CommandASTNode::Type::Expression) {
					if (std::get<CommandASTExpressionNode>(processing_nodes.top().value).type != CommandASTExpressionNode::Type::MethodBody) {
						throw CommandException("Protection is a statement and must be standalone.");
					}
				}
				else if (processing_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Protection is a statement and must be standalone.");
				}

				processing_nodes.push(CommandASTNode::make_protection({ }));
				awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
			}
			else if (token.value == "]") {
				if (processing_nodes.top().type != CommandASTNode::Type::Statement) {
					throw CommandException("Close bracket`]` appeared, but open bracket`[` is missing.");
				}
				auto& statement = std::get<CommandASTStatementNode>(processing_nodes.top().value);
				if (statement.type != CommandASTStatementNode::Type::Protection) {
					throw CommandException("Close bracket`]` appeared, but open bracket`[` is missing.");
				}
				auto& protection = std::get<CommandASTProtectionNode>(statement.value);

				if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
					throw CommandException("Nothing provided to be protected.");
				}
				auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
				if (expression.type != CommandASTExpressionNode::Type::Identifier) {
					throw CommandException("Only identifiers can be protected.");
				}
				protection.name = std::get<CommandASTIdentifierNode>(expression.value).name;
				
				awaiting_nodes.pop();
				awaiting_nodes.pop(); // Remove barrier.
				awaiting_nodes.push(std::move(processing_nodes.top()));
				processing_nodes.pop();
			}
			else {
				throw CommandException("Internal Parser Error: {} is not a bracket.", token.value);
			}
		}

		auto parse_braces(const CommandToken& token) -> void {
			if (token.value == "{") {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Method body can not follow other elements directly like parentheses.");
				}
				processing_nodes.push(CommandASTNode::make_method_body({ }));
				awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
			}
			else if (token.value == "}") {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Closing method body but there's an incomplete statement.");
				}
				if (processing_nodes.top().type != CommandASTNode::Type::Expression) {
					throw CommandException("Close brace appeared, but open brace is missing.");
				}
				auto& expression = std::get<CommandASTExpressionNode>(processing_nodes.top().value);
				if (expression.type != CommandASTExpressionNode::Type::MethodBody) {
					throw CommandException("Close brace appeared, but open brace is missing.");
				}
				awaiting_nodes.pop(); // Remove barrier.
				awaiting_nodes.push(std::move(processing_nodes.top()));
				processing_nodes.pop();
			}
			else {
				throw CommandException("Internal Parser Error: {} is not a brace.", token.value);
			}
		}

		auto parse_comma(const CommandToken& token) -> void {
			auto submitted = submit_preceding_operations(",");

			if (awaiting_nodes.top().type == CommandASTNode::Type::None) {
				awaiting_nodes.push(CommandASTNode::make_empty());
			}
			else if (awaiting_nodes.top().type != CommandASTNode::Type::Expression) {
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

		auto parse_semicolon(const CommandToken& token) -> void {
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
						assignment.expression = CommandASTExpressionNode::make_empty();
						awaiting_nodes.pop(); // Remove barrier.
						processing_nodes.pop();
					}
					else {
						throw CommandException("Assigning a non-expression node is not allowed.");
					}
				}
				else if (statement.type == CommandASTStatementNode::Type::Delete) {
					auto& deletion = std::get<CommandASTDeleteNode>(statement.value);
					if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
						if (expression.type != CommandASTExpressionNode::Type::Identifier) {
							throw CommandException("Only identifiers can be deleted.");
						}
						deletion.name = std::get<CommandASTIdentifierNode>(expression.value).name;
						awaiting_nodes.pop();
						if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
							throw CommandException("Only one identifier can be deleted.");
						}
						awaiting_nodes.pop(); // Remove barrier.
						processing_nodes.pop();
					}
					else if (awaiting_nodes.top().type == CommandASTNode::Type::None) { // None is not a legal node in AST, but a barrier for parser.
						throw CommandException("Deletion terminated but no expression found.");
					}
					else {
						throw CommandException("Deleting a non-expression node is not allowed.");
					}
				}
				else if (statement.type == CommandASTStatementNode::Type::Argument) {
					auto& argument = std::get<CommandASTArgumentNode>(statement.value);
					if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
						if (expression.type != CommandASTExpressionNode::Type::Identifier) {
							throw CommandException("Only identifiers can fetch arguments.");
						}
						argument.name = std::get<CommandASTIdentifierNode>(expression.value).name;
						awaiting_nodes.pop();
						if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
							throw CommandException("Only one identifier can fetch an argument at once.");
						}
						awaiting_nodes.pop(); // Remove barrier.
						processing_nodes.pop();
					}
					else if (awaiting_nodes.top().type == CommandASTNode::Type::None) { 
						throw CommandException("Argument terminated but no expression found.");
					}
					else {
						throw CommandException("Fething to a non-expression node is not allowed.");
					}
				}
				else if (statement.type == CommandASTStatementNode::Type::Return) {
					auto& returning = std::get<CommandASTReturnNode>(statement.value);
					if (awaiting_nodes.top().type == CommandASTNode::Type::Expression) {
						auto& expression = std::get<CommandASTExpressionNode>(awaiting_nodes.top().value);
						returning.expression = std::move(expression);
						awaiting_nodes.pop();

						if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
							throw CommandException("Only one expression can be assigned.");
						}
						awaiting_nodes.pop(); // Remove barrier.
						processing_nodes.pop();
					}
					else if (awaiting_nodes.top().type == CommandASTNode::Type::None) { // None is not a legal node in AST, but a barrier for parser.
						returning.expression = CommandASTExpressionNode::make_empty();
						awaiting_nodes.pop(); // Remove barrier.
						processing_nodes.pop();
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
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Statement not defined.");
				}
			}
			else if (awaiting_nodes.top().type == CommandASTNode::Type::Statement) { // Any statements that are waiting to be submitted.
				statement = std::move(std::get<CommandASTStatementNode>(awaiting_nodes.top().value));
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
				if (processing_nodes.top().type != CommandASTNode::Type::Expression) {
					throw CommandException("A Statemet must be at top level of the globe or a method body.");
				}
				auto& parent_expression = std::get<CommandASTExpressionNode>(processing_nodes.top().value);
				if (parent_expression.type != CommandASTExpressionNode::Type::MethodBody) {
					throw CommandException("A Statemet must be at top level of the globe or a method body.");
				}
				auto& method_body = std::get<CommandASTMethodBodyNode>(parent_expression.value);
				method_body.commands.push_back(std::move(statement));
			}
		}

		auto parse_special(const CommandToken& token) -> void {
			if (token.value == "#") {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Delete is a statement and must be standalone.");
				}
				if (processing_nodes.top().type == CommandASTNode::Type::Expression) {
					if (std::get<CommandASTExpressionNode>(processing_nodes.top().value).type != CommandASTExpressionNode::Type::MethodBody) {
						throw CommandException("Delete is a statement and must be standalone.");
					}
				}
				else if (processing_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Delete is a statement and must be standalone.");
				}

				processing_nodes.push(CommandASTNode::make_delete({ }));
				awaiting_nodes.push(CommandASTNode::make_none()); // Barrier.
			}
			else {
				throw CommandException("Unsupported token: {}", token.value);;
				throw CommandException("Internal Parser Error: {} is not a special.", token.value);
			}
		}

		auto parse_method_related(const CommandToken& token) -> void {
			if (token.value[0] == '<') {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Return is a statement and must be standalone.");
				}
				if (processing_nodes.top().type == CommandASTNode::Type::Expression) {
					if (std::get<CommandASTExpressionNode>(processing_nodes.top().value).type != CommandASTExpressionNode::Type::MethodBody) {
						throw CommandException("Return is a statement and must be standalone.");
					}
				}
				else if (processing_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Return is a statement and must be standalone.");
				}

				processing_nodes.push(CommandASTNode::make_return(token.value.length(), CommandASTExpressionNode::make_empty()));
				awaiting_nodes.push(CommandASTNode::make_none());
			}
			else if (token.value[0] == '>') {
				if (awaiting_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Argument is a statement and must be standalone.");
				}
				if (processing_nodes.top().type == CommandASTNode::Type::Expression) {
					if (std::get<CommandASTExpressionNode>(processing_nodes.top().value).type != CommandASTExpressionNode::Type::MethodBody) {
						throw CommandException("Argument is a statement and must be standalone.");
					}
				}
				else if (processing_nodes.top().type != CommandASTNode::Type::None) {
					throw CommandException("Argument is a statement and must be standalone.");
				}

				processing_nodes.push(CommandASTNode::make_argument(token.value.length(), { }));
				awaiting_nodes.push(CommandASTNode::make_none());
			}
			else {
				throw CommandException("Unsupported method related symbol: {}", token.value);
			}
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
				else if (c == '"') {
					std::string str;
					c = *++iter;
					while (c != '"') {
						if (c == '\\') {
							if (++iter == input.end()) {
								break;
							}
							c = *iter;
							switch (c)
							{
							case 'n':
								str += '\n';
								break;
							case 'r':
								str += '\r';
								break;
							case 't':
								str += '\t';
								break;
							case 'b':
								str += '\b';
								break;
							case 'f':
								str += '\f';
								break;
							case 'v':
								str += '\v';
								break;
							case 'a':
								str += '\a';
								break;
							case '\\':
								str += '\\';
								break;
							case '\'':
								str += '\'';
								break;
							case '"':
								str += '"';
								break;
							case '?':
								str += '?';
								break;
							case '0':
								str += '\0';
								break;
							case 'x': {
								if (++iter == input.end()) {
									break;
								}
								c = *iter;
								if (!is_hex(c)) {
									throw CommandException("Invalid hex character: {}", c);
								}
								uint8_t hex = 0;
								if (is_digit(c)) {
									hex = c - '0';
								}
								else if (is_lower(c)) {
									hex = c - 'a' + 10;
								}
								else {
									hex = c - 'A' + 10;
								}

								if (++iter == input.end()) {
									break;
								}
								c = *iter;
								if (!is_hex(c)) {
									throw CommandException("Invalid hex character: {}", c);
								}
								hex <<= 4;
								if (is_digit(c)) {
									hex |= c - '0';
								}
								else if (is_lower(c)) {
									hex |= c - 'a' + 10;
								}
								else {
									hex |= c - 'A' + 10;
								}
								str.push_back(hex);
								break;
							}
							case 'u': {
								if (++iter == input.end()) {
									break;
								}
								c = *iter;
								if (!is_hex(c)) {
									throw CommandException("Invalid hex character: {}", c);
								}
								uint16_t hex = 0;
								if (is_digit(c)) {
									hex = c - '0';
								}
								else if (is_lower(c)) {
									hex = c - 'a' + 10;
								}
								else {
									hex = c - 'A' + 10;
								}
								for (int i = 0; i < 3; ++i) {
									if (++iter == input.end()) {
										break;
									}
									c = *iter;
									if (!is_hex(c)) {
										throw CommandException("Invalid hex character: {}", c);
									}
									hex <<= 4;
									if (is_digit(c)) {
										hex |= c - '0';
									}
									else if (is_lower(c)) {
										hex |= c - 'a' + 10;
									}
									else {
										hex |= c - 'A' + 10;
									}
								}
								str.push_back(hex);
								break;
							}
							default:
								throw CommandException("Invalid escape character: {}", c);
								break;
							}
						}
						else {
							str += c;
						}
						if (++iter == input.end()) {
							break;
						}
						c = *iter;
					}
					if (iter == input.end()) {
						throw CommandException("String is not closed.");
					}
					parser << CommandToken{ CommandToken::Type::String, str };
				}
				else if (is_separator(c)) {
					parser << CommandToken{ CommandToken::Type::Separator, std::string(1, c) };
				}
				else if (is_operator(c)) {
					parser << CommandToken{ CommandToken::Type::Operator, std::string(1, c) };
				}
				else if (is_special(c)) {
					parser << CommandToken{ CommandToken::Type::Special, std::string(1, c) };
				}
				else if (is_method_related(c)) {
					std::string full;
					while (is_method_related(c)) {
						full += c;
						if (++iter == input.end()) {
							break;
						}
						c = *iter;
					}
					--iter;
					parser << CommandToken{ CommandToken::Type::MethodRelated, full };
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
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
		}
		auto is_digit(char c) -> bool {
			return (c >= '0' && c <= '9') || (c == '.');
		}
		auto is_separator(char c) -> bool {
			return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ';';
		}
		auto is_operator(char c) -> bool {
			return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '=';
		}
		auto is_special(char c) -> bool {
			return c == '!' || c == '#' || c == '&' || c == '|' || c == '~' || c == '`' || c == '?' || c == ':' ;
		}
		auto is_method_related(char c) -> bool {
			return c == '@' || c == '$' || c == '<' || c == '>'; // Cenvrons are not used as brackets in ACL.
		}
		auto is_eof(char c) -> bool {
			return c == '\0';
		}
		auto is_hex(char c) -> bool {
			return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
		}
		auto is_capitol(char c) -> bool {
			return c >= 'A' && c <= 'Z';
		}
		auto is_lower(char c) -> bool {
			return c >= 'a' && c <= 'z';
		}
	};
}