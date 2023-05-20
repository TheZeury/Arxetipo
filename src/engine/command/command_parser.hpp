#pragma once

namespace arx
{
	struct CommandToken
	{
		enum class Type
		{
			None,
			Comment,
			Name,
			Number,
			String,
			Separator,
			Operator,
			Condition,
			Special,
			FunctionRelated,
			Eof,
		};
		Type type;
		std::string value;
	};

	const std::unordered_map<std::string, int32_t> operator_left_precedence = {
		{ ";", 0 },
		{ ":", 0 },
		{ ",", 2 },
		{ "?", 2 },
		{ "==", 3 },
		{ "!=", 3 },
		{ "<", 4 },
		{ ">", 4 },
		{ "<=", 4 },
		{ ">=", 4 },
		{ "+", 5 },
		{ "-", 5 },
		{ "*", 6 },
		{ "/", 6 },
		{ "%", 6 },
		{ "^", 8 },
		{ "=", 9 },
		{ "f", 9 },
	};

	const std::unordered_map<std::string, int32_t> operator_right_precedence = {
		{ "=", 1 },
		{ "'%", 1 },
		{ "'<", 1 },
		{ ",", 1 },
		{ "?", 1 },
		{ ":", 1 },
		{ "==", 3 },
		{ "!=", 3 },
		{ "<", 4 },
		{ ">", 4 },
		{ "<=", 4 },
		{ ">=", 4 },
		{ "+", 5 },
		{ "-", 5 },
		{ "*", 6 },
		{ "/", 6 },
		{ "%", 6 },
		{ "'+", 7 },
		{ "'-", 7 },
		{ "!", 7 },
		{ "^", 8 },
		{ "#", 9 },
		{ "@", 9 },
		{ "f", 9 },
	};

	template<typename I>
	struct CommandParser
	{
		/// <summary>
		/// Awaiting nodes are completed by themselves, but waiting to join to their parent nodes.
		/// </summary>
		CommandASTExpressionNode awaiting_expression;

		/// <summary>
		/// Processing node are uncompleted and waiting for their child nodes to be completed.
		/// </summary>
		std::stack<CommandASTExpressionNode> processing_nodes;

		auto operator<<(const CommandToken& token) -> CommandParser& {
			switch (token.type)
			{
			case CommandToken::Type::None: {
				// Placeholder.
				break;
			}
			case CommandToken::Type::Name: {
				parse_name(token);
				break;
			}
			case CommandToken::Type::Number: {
				parse_number(token);
				break;
			}
			case CommandToken::Type::String: {
				parse_string(token);
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
			case CommandToken::Type::Condition: {
				parse_condition(token);
				break;
			}
			case CommandToken::Type::Special: {
				parse_special(token);
				break;
			}
			case CommandToken::Type::FunctionRelated: {
				parse_function_related(token);
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

		auto parse_name(const CommandToken& token) -> void {
			submit_expression(CommandASTExpressionNode::make_identifier(token.value));
		}

		auto parse_number(const CommandToken& token) -> void {
			float value = std::stof(token.value);
			submit_expression(CommandASTExpressionNode::make_number(std::stof(token.value)));
		}

		auto parse_string(const CommandToken& token) -> void {
			submit_expression(CommandASTExpressionNode::make_string(token.value));
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

		auto parse_parentheses(const CommandToken& token) -> void {
			if (token.value == "(") {
				open_incomplete_expression(CommandASTExpressionNode::make_parentheses({ }));
			}
			else if (token.value == ")") {
				submit_preceding_expressions(")");

				if (processing_nodes.top().type == CommandASTExpressionNode::Type::Parentheses) {
					auto& parentheses = std::get<CommandASTParenthesesNode>(processing_nodes.top().value);

					parentheses.expression = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
				}
				else {
					throw CommandException("Unexpected close parentheses`)`. Close parentheses`)` appeared but parentheses`(` is missing.");
				}
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not a parenthese.", token.value);
			}
		}

		auto parse_brackets(const CommandToken& token) -> void {
			if (token.value == "[") {
				open_incomplete_expression(CommandASTExpressionNode::make_protection(nullptr));
			}
			else if (token.value == "]") {
				submit_preceding_expressions("]");

				if (processing_nodes.top().type == CommandASTExpressionNode::Type::Protection) {
					auto& protection = std::get<CommandASTProtectionNode>(processing_nodes.top().value);

					protection.target = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
				}
				else {
					throw CommandException("Unexpected close bracket`]`. Close bracket`]` appeared, but open bracket`[` is missing.");
				}
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not a bracket.", token.value);
			}
		}

		auto parse_braces(const CommandToken& token) -> void {
			if (token.value == "{") {
				open_incomplete_expression(CommandASTExpressionNode::make_function_body({ }));
			}
			else if (token.value == "}") {
				submit_preceding_expressions("}");

				if (processing_nodes.top().type == CommandASTExpressionNode::Type::FunctionBody) {
					if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
						throw CommandException("Closing function body but there's an incomplete statement.");
					}
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
				}
				else {
					throw CommandException("Unexpected close brace. Close brace appeared, but open brace is missing.");
				}
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not a brace.", token.value);
			}
		}

		auto parse_comma(const CommandToken& token) -> void {
			submit_preceding_expressions(",");

			if (processing_nodes.top().type == CommandASTExpressionNode::Type::List) {
				auto& list = std::get<CommandASTListNode>(processing_nodes.top().value);
				list.expressions.push_back(std::move(awaiting_expression));
				awaiting_expression = CommandASTExpressionNode::make_empty();
			}
			else {
				auto expression = CommandASTExpressionNode::make_list({ });
				auto& list = std::get<CommandASTListNode>(expression.value);
				list.expressions.push_back(std::move(awaiting_expression));
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(expression));
			}
		}

		auto parse_semicolon(const CommandToken& token) -> void {
			submit_preceding_expressions(";");;

			CommandASTStatementNode statement{ CommandASTStatementNode::Type::Expression, std::move(awaiting_expression) };
			awaiting_expression = CommandASTExpressionNode::make_empty();

			if (processing_nodes.size() == 1) {
				interpreter << statement;
			}
			else {
				if (processing_nodes.top().type == CommandASTExpressionNode::Type::FunctionBody) {
					auto& function_body = std::get<CommandASTFunctionBodyNode>(processing_nodes.top().value);
					function_body.commands.push_back(std::move(statement));
				}
				else {
					throw CommandException("A Statemet must be at top level of the globe or a function body.");
				}
			}
		}

		auto parse_operator(const CommandToken& token) -> void {
			auto op = token.value;
			if (op == "=") {
				submit_preceding_expressions("=");
				auto assignment = CommandASTExpressionNode::make_assignment(std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression)), nullptr);
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(assignment));
			}
			else if (op == ":=") {
				submit_preceding_expressions("=");
				auto assignment = CommandASTExpressionNode::make_assignment(std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression)), nullptr, true);
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(assignment));
			}
			else if (op == "!") {
				open_incomplete_expression(CommandASTExpressionNode::make_operation(to_operation_type(op), 1, { }));
			}
			else if (op == ">" || op == ">=" || op == "<" || op == "<=" || op == "==" || op == "!= ") {
				submit_preceding_expressions(op);
				auto expression = CommandASTExpressionNode::make_operation(to_operation_type(op), 2, {  });
				auto& operation = std::get<CommandASTOperationNode>(expression.value);
				operation.operands.push_back(std::move(awaiting_expression));
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(expression));
			}
			else if (op == "^" || op == "%") {
				submit_preceding_expressions(op);
				auto expression = CommandASTExpressionNode::make_operation(to_operation_type(op), 2, {  });
				auto& operation = std::get<CommandASTOperationNode>(expression.value);
				operation.operands.push_back(std::move(awaiting_expression));
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(expression));
			}
			else if (op == "+" || op == "-") {
				if (awaiting_expression.type == CommandASTExpressionNode::Type::Empty) {
					op = "'" + op; // '+ and '-
					open_incomplete_expression(CommandASTExpressionNode::make_operation(to_operation_type(op), 1, { }));
				}
				else { // + -
					submit_preceding_expressions(op);
					if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
						auto expression = CommandASTExpressionNode::make_operation(to_operation_type(op), 2, {  });
						auto& operation = std::get<CommandASTOperationNode>(expression.value);
						operation.operands.push_back(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
						open_incomplete_expression(std::move(expression));
					}
					else {
						throw CommandException("Unexpected Parser Error. No expression awaiting after submitting preceding expressions");
					}
				}
			}
			else if (op == "*" || op == "/") { // * /
				submit_preceding_expressions(op);
				auto expression = CommandASTExpressionNode::make_operation(to_operation_type(op), 2, {  });
				auto& operation = std::get<CommandASTOperationNode>(expression.value);
				operation.operands.push_back(std::move(awaiting_expression));
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(expression));
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not an operator.", token.value);
			}
		}

		auto parse_condition(const CommandToken& token) -> void {
			auto op = token.value;
			if (op == "?") {
				submit_preceding_expressions("?");
				auto condition = CommandASTExpressionNode::make_condition(std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression)), nullptr, nullptr);
				awaiting_expression = CommandASTExpressionNode::make_empty();
				open_incomplete_expression(std::move(condition));
			}
			else if (op == ":") {
				submit_preceding_expressions(":");
				if (processing_nodes.top().type != CommandASTExpressionNode::Type::Condition) {
					throw CommandException("Unexpected Parser Error. No condition awaiting after submitting preceding expressions");
				}
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not a condition symbol.", token.value);
			}
		}

		auto parse_special(const CommandToken& token) -> void {
			if (token.value == "#") {
				open_incomplete_expression(CommandASTExpressionNode::make_delete(nullptr));
			}
			else if (token.value == "@") {
				open_incomplete_expression(CommandASTExpressionNode::make_accessing(nullptr));
			}
			else {
				throw CommandException("Unexpected Parser Error: {} is not a special.", token.value);
			}
		}

		auto parse_function_related(const CommandToken& token) -> void {
			if (token.value[0] == '<') {
				if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty && token.value.size() == 1) {
					parse_operator({ CommandToken::Type::Operator, "<" });
				}
				else {
					open_incomplete_expression(CommandASTExpressionNode::make_return(
						token.value.length(),
						std::make_unique<CommandASTExpressionNode>(CommandASTExpressionNode::make_empty())
					));
				}
			}
			else if (token.value[0] == '>') {
				if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty && token.value.size() == 1) {
					parse_operator({ CommandToken::Type::Operator, ">" });
				}
				else {
					submit_expression(CommandASTExpressionNode::make_argument(token.value.length()));
				}
			}
			else if (token.value[0] == '$') {
				submit_expression(CommandASTExpressionNode::make_self(token.value.length()));
			}
			else if (token.value[0] == '%') {
				if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty && token.value.size() == 1) {
					parse_operator({ CommandToken::Type::Operator, "%" });
				}
				else {
					open_incomplete_expression(CommandASTExpressionNode::make_loop(token.value.size(), nullptr));
				}
			}
			else {
				throw CommandException("Unsupported function related symbol: {}", token.value);
			}
		}

		auto submit_preceding_expressions(const std::string& operation) -> uint32_t {
			uint32_t count = 0;
			while (!processing_nodes.empty()) {
				auto& expression = processing_nodes.top();

				if (expression.type == CommandASTExpressionNode::Type::Operation) {
					auto& last_operation = std::get<CommandASTOperationNode>(expression.value);
					auto str_op = to_string(last_operation.type);
					if (prior_to(operation, str_op)) {
						break;
					}

					if (last_operation.operands.size() >= last_operation.operand_count) {
						throw CommandException("Unexpect Parser Error: Operator {} can only operate on {} operands.", str_op, last_operation.operand_count);
					}

					last_operation.operands.push_back(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					if (last_operation.operands.size() == last_operation.operand_count) {
						submit_top_preceding();
						++count;
					}
					else {
						break;
					}
				}
				else if (expression.type == CommandASTExpressionNode::Type::Condition) {
					auto& condition = std::get<CommandASTConditionNode>(expression.value);
					if (condition.true_branch == nullptr && condition.false_branch == nullptr) {
						auto str_op = "?";
						if (prior_to(operation, str_op)) {
							break;
						}

						condition.true_branch = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
						if (operation != ":") {
							submit_top_preceding();
							++count;
						}
						else {
							break;
						}
					}
					else if (condition.true_branch != nullptr && condition.false_branch == nullptr) {
						auto str_op = ":";
						if (prior_to(operation, str_op)) {
							break;
						}
						condition.false_branch = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
						submit_top_preceding();
						++count;
					}
					else {
						throw CommandException("Unexpected Parser Error. Illegal processing condition expression state: false_branch is already full.");
					}
				}
				else if (expression.type == CommandASTExpressionNode::Type::Assignment) {
					auto& assignment = std::get<CommandASTAssignmentNode>(expression.value);
					auto str_op = "=";
					if (prior_to(operation, str_op)) {
						break;
					}

					assignment.expression = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
					++count;
				}
				else if (expression.type == CommandASTExpressionNode::Type::Delete) {
					auto& del = std::get<CommandASTDeleteNode>(expression.value);
					auto str_op = "#";
					if (prior_to(operation, str_op)) {
						break;
					}

					if (awaiting_expression.type == CommandASTExpressionNode::Type::Identifier) {
						del.target = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
						submit_top_preceding();
						++count;
					}
					else {
						throw CommandException("Only identifiers can be deleted.");
					}
				}
				else if (expression.type == CommandASTExpressionNode::Type::Accessing) {
					auto& accessing = std::get<CommandASTAccessingNode>(expression.value);
					auto str_op = "@";
					if (prior_to(operation, str_op)) {
						break;
					}

					accessing.expression = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
					++count;
				}
				else if (expression.type == CommandASTExpressionNode::Type::Return) {
					auto& ret = std::get<CommandASTReturnNode>(expression.value);
					auto str_op = "'<";
					if (prior_to(operation, str_op)) {
						break;
					}

					ret.expression = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
					++count;
				}
				else if (expression.type == CommandASTExpressionNode::Type::List) {
					auto& list = std::get<CommandASTListNode>(expression.value);
					auto str_op = ",";
					if (prior_to(operation, str_op)) {
						break;
					}

					if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
						list.expressions.push_back(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
					}
					submit_top_preceding();
					++count;
				}
				else if (expression.type == CommandASTExpressionNode::Type::Calling) {
					auto& calling = std::get<CommandASTCallingNode>(expression.value);
					auto str_op = "f";
					if (prior_to(operation, str_op)) {
						break;
					}

					calling.argument = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
					awaiting_expression = CommandASTExpressionNode::make_empty();
					submit_top_preceding();
					++count;
				}
				else if (expression.type == CommandASTExpressionNode::Type::Loop) {
					auto& loop = std::get<CommandASTLoopNode>(expression.value);
					auto str_op = "'%";
					if (prior_to(operation, str_op)) {
						break;
					}
					if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
						loop.argument = std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression));
						awaiting_expression = CommandASTExpressionNode::make_empty();
					}
					submit_top_preceding();
					++count;
				}
				else {
					break;
				}
			}
			return count;
		}

		auto prior_to(const std::string& right_op, const std::string& left_op) -> bool {
			if (!operator_left_precedence.contains(right_op)) {
				// throw CommandException("Operator`{}` is not supported to accept operants on it's left side.", a);
				return false;
			}
			if (!operator_right_precedence.contains(left_op)) {
				// throw CommandException("Operator`{}` is not supported to accept operants on it's right side.", b);
				return true;
			}
			return operator_left_precedence.at(right_op) > operator_right_precedence.at(left_op);
		}

		auto open_incomplete_expression(CommandASTExpressionNode&& expression) -> void {
			if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
				submit_preceding_expressions("f");
				if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
					processing_nodes.push(CommandASTExpressionNode::make_calling(
						std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression)),
						nullptr
					));
					awaiting_expression = CommandASTExpressionNode::make_empty();
				}
				else {
					throw CommandException("Unexpected Parser Error. No expression awaiting after submitting preceding expressions");
				}
			}
			processing_nodes.push(std::move(expression));
		}

		auto submit_expression(CommandASTExpressionNode&& expression) -> void {
			if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) { // Turn the previous expression into a function call.
				submit_preceding_expressions("f");
				if (awaiting_expression.type != CommandASTExpressionNode::Type::Empty) {
					processing_nodes.push(CommandASTExpressionNode::make_calling(
						std::make_unique<CommandASTExpressionNode>(std::move(awaiting_expression)),
						{ }
					));
				}
				else {
					throw CommandException("Unexpected Parser Error. No expression awaiting after submitting preceding expressions");
				}
			}
			awaiting_expression = std::move(expression);
		}

		auto submit_top_preceding() -> void {
			if (!processing_nodes.empty()) {
				if (awaiting_expression.type == CommandASTExpressionNode::Type::Empty) {
					awaiting_expression = std::move(processing_nodes.top());
					processing_nodes.pop();
				}
				else {
					throw CommandException("Awaiting expression must be empty when submitting top preceding expression.");
				}
			}
		}

		CommandParser(I& interpreter) : interpreter(interpreter), awaiting_expression(CommandASTExpressionNode::make_empty()) {
			processing_nodes.push(CommandASTExpressionNode::make_empty());
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
				if (c == '/') {
					if (++iter != input.end()) {
						c = *iter;
						if (c == '/') {
							while (c != '\n') {
								if (++iter == input.end()) {
									break;
								}
								c = *iter;
							}
							if (iter == input.end()) {
								break;
							}
						}
						else {
							--iter;
							c = '/';
						}
					}
					else {
						--iter;
						c = '/';
					}
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
				else if (is_function_related(c)) {
					std::string full;
					char h = c;
					while (h == c) {
						full += c;
						if (++iter == input.end()) {
							break;
						}
						c = *iter;
					}

					if (c == '=' && full.size() == 1 && (h == '<' || h == '>')) {
						full += '=';
						parser << CommandToken{ CommandToken::Type::Operator, full };
					}
					else {
						--iter;
						parser << CommandToken{ CommandToken::Type::FunctionRelated, full };
					}
				}
				else if (is_comparator(c)) {
					++iter;
					auto s = *iter;
					if (s == '=') {
						parser << CommandToken{ CommandToken::Type::Operator, std::string({ c, s }) };
					}
					else {
						--iter;
						parser << CommandToken{ CommandToken::Type::Operator, std::string(1, c) };
					}
				}
				else if (is_operator(c)) {
					parser << CommandToken{ CommandToken::Type::Operator, std::string(1, c) };
				}
				else if (c == ':') {
					++iter;
					auto s = *iter;
					if (s == '=') {
						parser << CommandToken{ CommandToken::Type::Operator, std::string({ c, s }) };
					}
					else {
						--iter;
						parser << CommandToken{ CommandToken::Type::Condition, std::string(1, c) };
					}
				}
				else if (is_condition(c)) {
					parser << CommandToken{ CommandToken::Type::Condition, std::string(1, c) };
				}
				else if (is_special(c)) {
					parser << CommandToken{ CommandToken::Type::Special, std::string(1, c) };
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
		auto is_comparator(char c) -> bool {
			return c == '<' || c == '>' || c == '=' || c == '!';
		}
		auto is_function_related(char c) -> bool {
			return c == '$' || c == '%' || c == '<' || c == '>'; // Cenvrons are not used as brackets in ACL.
		}
		auto is_operator(char c) -> bool {
			return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '=';
		}
		auto is_condition(char c) -> bool {
			return c == '?' || c == ':';
		}
		auto is_special(char c) -> bool {
			return c == '!' || c == '#' || c == '@' || c == '&' || c == '|' || c == '~' || c == '`';
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