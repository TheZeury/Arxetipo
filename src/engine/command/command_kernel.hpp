#pragma once

namespace arx
{
	struct CommandValue
	{
		enum class Type
		{
			Empty,
			Number,
			Method,
		};
		Type type;
		std::variant<std::monostate, float, std::function<void(const std::vector<CommandValue>&, CommandValue&)>> value;

		auto to_string() const -> std::string {
			switch (type) {
			case Type::Empty: {
				return "()";
			}
			case Type::Number: {
				return std::to_string(std::get<float>(value));
			}
			case Type::Method: {
				return "method";
			}
			default: {
				return "unknown";
			}
			}
		}

		auto operator+() const -> CommandValue {
			return *this;
		}

		auto operator-() const -> CommandValue {
			if (type == Type::Number) {
				return CommandValue{ Type::Number, -std::get<float>(value) };
			}
			return CommandValue{ Type::Empty, std::monostate{} };
		}

		auto operator+(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty) {
				return other;
			}
			if (other.type == Type::Empty) {
				return *this;
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) + std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, std::monostate{} };
		}

		auto operator-(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty) {
				return -other;
			}
			if (other.type == Type::Empty) {
				return *this;
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) - std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, std::monostate{ } };
		}

		auto operator*(const CommandValue& other) const -> CommandValue {
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) * std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, std::monostate{ } };
		}

		auto operator/(const CommandValue& other) const -> CommandValue {
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) / std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, std::monostate{ } };
		}
	};

	struct CommandKernel
	{
		std::vector<std::unordered_map<std::string, CommandValue>> scope_stack{ 1 };
		std::unordered_set<std::string> scope_wide_protected;

		auto add_method(const std::string& name, const std::function<void(const std::vector<CommandValue>&, CommandValue&)>& method, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Method, method } });
			if (protect) {
				scope_wide_protected.insert(name);
			}
			return *this;
		}

		auto add_number(const std::string& name, float value, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Number, value } });
			if (protect) {
				scope_wide_protected.insert(name);
			}
			return *this;
		}

		auto add_empty(const std::string& name, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Empty, std::monostate{ } } });
			if (protect) {
				scope_wide_protected.insert(name); 
			}
			return *this;
		}

		auto operator<<(const CommandASTStatementNode& statement) -> CommandKernel& {
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
			return *this;
		}

		auto excute_expression(const CommandASTExpressionNode& expression) -> CommandValue {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Empty: {
				return CommandValue{ CommandValue::Type::Empty, std::monostate{ } };
			}
			case CommandASTExpressionNode::Type::Number: {
				return CommandValue{ CommandValue::Type::Number, std::get<CommandASTNumberNode>(expression.value).value };
			}
			case CommandASTExpressionNode::Type::Identifier: {
				for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
					if (scope->find(std::get<CommandASTIdentifierNode>(expression.value).name) != scope->end()) {
						return scope->at(std::get<CommandASTIdentifierNode>(expression.value).name);
					}
				}
				scope_stack.rbegin()->insert({ std::get<CommandASTIdentifierNode>(expression.value).name, CommandValue{ CommandValue::Type::Empty, std::monostate{ } } });
				return scope_stack.rbegin()->at(std::get<CommandASTIdentifierNode>(expression.value).name);
			}
			case CommandASTExpressionNode::Type::MethodCall: {
				const auto& method_name = std::get<CommandASTMethodCallNode>(expression.value).method_name;
				for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
					if (scope->find(method_name) != scope->end()) {
						const auto& method = scope->at(method_name);
						if (method.type == CommandValue::Type::Method) {
							std::vector<CommandValue> arguments;
							for (const auto& argument : std::get<CommandASTMethodCallNode>(expression.value).arguments) {
								arguments.push_back(excute_expression(argument));
							}
							CommandValue result;
							std::get<std::function<void(const std::vector<CommandValue>&, CommandValue&)>>(method.value)(arguments, result);
							return result;
						}
						else {
							throw CommandException("`{}` is not a method.", method_name);
						}
					}
				}
				throw CommandException("Method `{}` not found.", method_name);
				break;
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				return excute_expression(std::get<CommandASTParenthesesNode>(expression.value).expressions[0]);
				break;
			}
			case CommandASTExpressionNode::Type::Operation: {
				const auto& operation = std::get<CommandASTOperationNode>(expression.value);
				switch (operation.type)
				{
				case CommandASTOperationNode::Type::Add: {
					return excute_expression(operation.operands[0]) + excute_expression(operation.operands[1]);
				}
				case CommandASTOperationNode::Type::Subtract: {
					return excute_expression(operation.operands[0]) - excute_expression(operation.operands[1]);
				}
				case CommandASTOperationNode::Type::Multiply: {
					return excute_expression(operation.operands[0]) * excute_expression(operation.operands[1]);
				}
				case CommandASTOperationNode::Type::Divide: {
					return excute_expression(operation.operands[0]) / excute_expression(operation.operands[1]);
				}
				case CommandASTOperationNode::Type::Positive: {
					return excute_expression(operation.operands[0]);
				}
				case CommandASTOperationNode::Type::Negative: {
					return -excute_expression(operation.operands[0]);
				}
				default:
					throw CommandException("Unknow operation.");
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
			if (scope_wide_protected.contains(assignment.name)) {
				throw CommandException("`{}` is protected, cannot assign to this name.", assignment.name);
			}
			for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
				if (scope->find(assignment.name) != scope->end()) {
					scope->at(assignment.name) = excute_expression(assignment.expression);
					return;
				}
			}
			scope_stack.rbegin()->insert({ assignment.name, excute_expression(assignment.expression) });
		}
	};
}