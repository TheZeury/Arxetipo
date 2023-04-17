#pragma once

namespace arx
{
	struct CommandValue
	{
		enum class Type
		{
			Empty,
			Number,
			List,
			Method,
		};
		Type type;
		std::variant<std::monostate, float, std::vector<CommandValue>, std::function<void(const std::vector<CommandValue>&, CommandValue&)>> value;

		auto to_string() const -> std::string {
			switch (type) {
			case Type::Empty: {
				return "()";
			}
			case Type::Number: {
				return std::to_string(std::get<float>(value));
			}
			case Type::List: {
				std::string result = "[";
				for (const auto& item : std::get<std::vector<CommandValue>>(value)) {
					result += item.to_string() + ", ";
				}
				if (result.size() > 1) {
					result.pop_back();
					result.pop_back();
				}
				result += "]";
				return result;
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
			if (type == Type::List && other.type == Type::List) {
				auto result = std::get<std::vector<CommandValue>>(value);
				result.insert(result.end(), std::get<std::vector<CommandValue>>(other.value).begin(), std::get<std::vector<CommandValue>>(other.value).end());
				return CommandValue{ Type::List, result };
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
			case CommandASTStatementNode::Type::Protection: {
				scope_wide_protected.insert(std::get<CommandASTProtectionNode>(statement.value).name);
				break;
			}
			case CommandASTStatementNode::Type::Argument: {
				excute_argument(std::get<CommandASTArgumentNode>(statement.value));
				break;
			}
			case CommandASTStatementNode::Type::Delete: {
				excute_delete(std::get<CommandASTDeleteNode>(statement.value));
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
				//std::cerr << "Identifier " << std::get<CommandASTIdentifierNode>(expression.value).name << " not found." << std::endl;
				scope_stack.rbegin()->insert({ std::get<CommandASTIdentifierNode>(expression.value).name, CommandValue{ CommandValue::Type::Empty, std::monostate{ } } });
				return scope_stack.rbegin()->at(std::get<CommandASTIdentifierNode>(expression.value).name);
			}
			case CommandASTExpressionNode::Type::MethodCall: {
				const auto& method = excute_expression(*(std::get<CommandASTMethodCallNode>(expression.value).method_body));
				if (method.type != CommandValue::Type::Method) {
					throw CommandException("Trying to call a non-method.");
					// Maybe we can allow calling a non-method but return a empty value? TODO.
				}
				std::vector<CommandValue> arguments;
				for (auto& argument : std::get<CommandASTMethodCallNode>(expression.value).arguments) {
					arguments.push_back(excute_expression(argument));
				}
				CommandValue result;
				std::get<std::function<void(const std::vector<CommandValue>&, CommandValue&)>>(method.value)(arguments, result);
				return result;
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				return excute_expression(std::get<CommandASTParenthesesNode>(expression.value).expressions[0]);
				break;
			}
			case CommandASTExpressionNode::Type::Operation: {
				auto& operation = std::get<CommandASTOperationNode>(expression.value);
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
			case CommandASTExpressionNode::Type::MethodBody: {
				// shard_ptr is the best of the bests!!!!!!!!!!!!!!!!!
				// I love it!!!!!!!!!!!!!!!!!!!!!
				// It solved a problem on which I spent hours!!!!!!!!!!!!!!!!!
				std::shared_ptr<CommandASTMethodBodyNode> method_body = std::make_shared<CommandASTMethodBodyNode>(std::get<CommandASTMethodBodyNode>(expression.value).clone());
				auto method = [&, method_body](const std::vector<CommandValue>& arguments, CommandValue& result) {
					if (scope_stack.size() >= 1000) {
						throw CommandException("Stack overflow.");
					}
					scope_stack.push_back({ });
					scope_stack.back().insert({ "@", CommandValue{ CommandValue::Type::List, arguments } });
					scope_stack.back().insert({ "@i", CommandValue{ CommandValue::Type::Number, 0.f } });
					for (auto& statement : method_body->commands) {
						*this << statement;
					}
					result = CommandValue{ CommandValue::Type::Empty, std::monostate{ } };
					scope_stack.pop_back();
				};
				return CommandValue{ CommandValue::Type::Method, method };
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

			// !!!!!!!!!!!!!!!!!!!!!!!!!!!! LOOK HERE MYSELF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// scope_stack.rbegin()->insert({ assignment.name, excute_expression(assignment.expression) });
			// ^ Explain: Why this line causes leak?
			// It first takes a iterator rbegin(),
			// but in the excute_expression() function, it is possible that something will be pushed into the scope_stack,
			// scope_stack is a std::vector which may reallocate the memory,
			// which will cause the rbegin() iterator to be invalidated.
			// And the iterator will be used in the insert() function, which will cause memory leak.
			// So, we must break up this like to make the `excute_expression()` excuted prior to the `rbegin()`
			// Like below:
			auto result = excute_expression(assignment.expression);
			scope_stack.rbegin()->insert({ assignment.name, std::move(result) });
		}

		auto excute_argument(const CommandASTArgumentNode& argument) -> void {
			if (scope_wide_protected.contains(argument.name)) {
				throw CommandException("`{}` is protected, cannot assign to this name.", argument.name);
			}
			auto skip = argument.length - 1;

			if (scope_stack.rbegin()[skip].find("@") == scope_stack.rbegin()[skip].end()) {
				throw CommandException("Fetching more arguments that provided.");
			}
			if (scope_stack.rbegin()[skip].at("@").type != CommandValue::Type::List) {
				throw CommandException("Fetching more arguments that provided.");
			}
			auto& i_v = scope_stack.rbegin()[skip].at("@i");
			size_t i = std::llround(std::get<float>(i_v.value));
			auto& arguments = std::get<std::vector<CommandValue>>(scope_stack.rbegin()[skip].at("@").value);
			if (i >= arguments.size()) {
				throw CommandException("Fetching more arguments that provided.");
			}
			i_v.value = float(i + 1);
			auto& value = arguments[i];

			for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
				if (scope->find(argument.name) != scope->end()) {
					scope->at(argument.name) = value;
					return;
				}
			}
			scope_stack.rbegin()->insert({ argument.name, value });
		}

		auto excute_delete(const CommandASTDeleteNode& deletion) -> void {
			if (scope_wide_protected.contains(deletion.name)) {
				throw CommandException("`{}` is protected, cannot delete this name.", deletion.name);
			}
			for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
				if (scope->find(deletion.name) != scope->end()) {
					scope->erase(deletion.name);
					return;
				}
			}
		}

		CommandKernel() {
			scope_stack.reserve(1000);
		}
	};
}