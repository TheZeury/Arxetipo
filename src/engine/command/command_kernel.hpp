#pragma once

namespace arx
{
	struct CommandValue
	{
		enum class Type
		{
			Empty,
			Number,
			String,
			List,
			Function,
			Macro,
		};
		Type type;
		std::variant<
			bool, 
			float, 
			std::string, 
			std::vector<CommandValue>, 
			std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>
		> value;

		CommandValue(
			Type type = Type::Empty, 
			std::variant<
				bool,
				float,
				std::string,
				std::vector<CommandValue>,
				std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>
			> value = true
		) : type{ type }, value{ value } {
		}

		auto to_string() const -> std::string {
			switch (type) {
			case Type::Empty: {
				return std::get<bool>(value) ? "()" : "(-)";
			}
			case Type::Number: {
				return std::to_string(std::get<float>(value));
			}
			case Type::String: {
				return std::get<std::string>(value);
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
			case Type::Function: {
				return "function";
			}
			case Type::Macro: {
				return "marco";
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
			if (type == Type::Empty) {
				return CommandValue{ Type::Empty, !std::get<bool>(value) };
			}
			else if (type == Type::Number) {
				return CommandValue{ Type::Number, -std::get<float>(value) };
			}
			return CommandValue{ Type::Empty, false };
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
			if (type == Type::List) {
				auto result = std::get<std::vector<CommandValue>>(value);
				if (other.type == Type::List) {
					result.insert(result.end(), std::get<std::vector<CommandValue>>(other.value).begin(), std::get<std::vector<CommandValue>>(other.value).end());
				}
				else {
					result.push_back(other);
				}
				return CommandValue{ Type::List, std::move(result) };
			}
			if (type == Type::String || other.type == Type::String) {
				return CommandValue{ Type::String, to_string() + other.to_string() };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator-(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty || other.type == Type::Empty) {
				return -other;
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) - std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator*(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue(Type::Empty, std::get<bool>(value) == std::get<bool>(other.value));
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) * std::get<float>(other.value) };
			}
			if (type == Type::Empty && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<bool>(value) ? std::get<float>(other.value) : -std::get<float>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Empty) {
				return CommandValue{ Type::Number, std::get<bool>(other.value) ? std::get<float>(value) : -std::get<float>(value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator/(const CommandValue& other) const -> CommandValue {
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::get<float>(value) / std::get<float>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator%(const CommandValue& other) const -> CommandValue {
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Number, std::fmod(std::get<float>(value), std::get<float>(other.value)) };
			}
			return CommandValue{ Type::Empty, true };
		}

		auto operator==(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue{ Type::Empty, std::get<bool>(value) == std::get<bool>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Empty, std::get<float>(value) == std::get<float>(other.value) };
			}
			if (type == Type::String && other.type == Type::String) {
				return CommandValue{ Type::Empty, std::get<std::string>(value) == std::get<std::string>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator!=(const CommandValue& other) const -> CommandValue {
			auto result = *this == other;
			std::get<bool>(result.value) = !std::get<bool>(result.value);
			return result;
		}

		auto operator<(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue{ Type::Empty, std::get<bool>(value) < std::get<bool>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Empty, std::get<float>(value) < std::get<float>(other.value) };
			}
			if (type == Type::String && other.type == Type::String) {
				return CommandValue{ Type::Empty, std::get<std::string>(value) < std::get<std::string>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator<=(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue{ Type::Empty, std::get<bool>(value) <= std::get<bool>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Empty, std::get<float>(value) <= std::get<float>(other.value) };
			}
			if (type == Type::String && other.type == Type::String) {
				return CommandValue{ Type::Empty, std::get<std::string>(value) <= std::get<std::string>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator>(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue{ Type::Empty, std::get<bool>(value) > std::get<bool>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Empty, std::get<float>(value) > std::get<float>(other.value) };
			}
			if (type == Type::String && other.type == Type::String) {
				return CommandValue{ Type::Empty, std::get<std::string>(value) > std::get<std::string>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator>=(const CommandValue& other) const -> CommandValue {
			if (type == Type::Empty && other.type == Type::Empty) {
				return CommandValue{ Type::Empty, std::get<bool>(value) >= std::get<bool>(other.value) };
			}
			if (type == Type::Number && other.type == Type::Number) {
				return CommandValue{ Type::Empty, std::get<float>(value) >= std::get<float>(other.value) };
			}
			if (type == Type::String && other.type == Type::String) {
				return CommandValue{ Type::Empty, std::get<std::string>(value) >= std::get<std::string>(other.value) };
			}
			return CommandValue{ Type::Empty, false };
		}

		auto operator!() const -> CommandValue {
			if (type == Type::Empty) {
				return CommandValue{ Type::Empty, !std::get<bool>(value) };
			}
			return CommandValue{ Type::Empty, false };
		}
	};

	inline auto to_string(const CommandValue::Type& type) -> std::string {
		switch (type) {
		case CommandValue::Type::Empty: {
			return "Empty";
		}
		case CommandValue::Type::Number: {
			return "Number";
		}
		case CommandValue::Type::String: {
			return "String";
		}
		case CommandValue::Type::List: {
			return "List";
		}
		case CommandValue::Type::Function: {
			return "Function";
		}
		case CommandValue::Type::Macro: {
			return "Marco";
		}
		default: {
			return "Unknown";
		}
		}
	}

	struct CommandKernel
	{
		struct StackFrame {
			std::unordered_map<std::string, CommandValue> identifiers;
			std::unordered_set<std::string> protections;
		};
		struct BodyFrame {
			std::shared_ptr<std::vector<CommandValue>> owned_arguments;
			const std::vector<CommandValue>* arguments;
			size_t index = 0;
			CommandValue return_value;
			const std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>* self;
			BodyFrame(
				const std::vector<CommandValue>* arguments, 
				const std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>* self
			) : 
				arguments{ arguments },
				return_value{ },
				self{ self } {
			}
		};
		std::vector<StackFrame> scope_stack{ 1 };
		std::vector<BodyFrame> body_stack;
		bool requiring_loop = false;

		auto add_identifier(const std::string& name, CommandValue&& value, bool protect = false) -> bool {
			if (scope_stack.back().protections.contains(name)) {
				return false;
			}
			scope_stack.back().identifiers[name] = std::move(value);
			if (protect) {
				scope_stack.back().protections.insert(name);
			}
			return true;
		}

		auto add_function(const std::string& name, const std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>& function, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->identifiers.insert({ name, CommandValue{ CommandValue::Type::Function, function } });
			if (protect) {
				scope_stack.rbegin()->protections.insert(name);
			}
			return *this;
		}

		/*auto add_number(const std::string& name, float value, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Number, value } });
			if (protect) {
				scope_wide_protected.insert(name);
			}
			return *this;
		}

		auto add_empty(const std::string& name, bool protect = false) -> CommandKernel& {
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Empty, true } });
			if (protect) {
				scope_wide_protected.insert(name); 
			}
			return *this;
		}*/

		auto find_identifier_or_insert(const std::string& name) -> std::pair<CommandValue&, bool> {
			auto is_protected = false;
			for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
				if (it->protections.find(name) != it->protections.end()) {
					is_protected = true;
				}
				auto find = it->identifiers.find(name);
				if (find != it->identifiers.end()) {
					return { find->second, is_protected };
				}
			}
			scope_stack.rbegin()->identifiers.insert({ name, CommandValue{ CommandValue::Type::Empty, true } });
			return { scope_stack.rbegin()->identifiers.at(name), false }; // false, not `is_protected`. 
		}
		
		auto find_identifier_or_throw(const std::string& name) -> std::pair<CommandValue&, bool> {
			auto is_protected = false;
			for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
				if (it->protections.find(name) != it->protections.end()) {
					is_protected = true;
				}
				auto find = it->identifiers.find(name);
				if (find != it->identifiers.end()) {
					return { find->second, is_protected };
				}
			}
			throw CommandException("Non-exist identifier {}.", name);
		}

		auto operator<<(const CommandASTStatementNode& statement) -> CommandKernel& {
			excute_statement(statement);
			return *this;
		}

		auto excute_statement(const CommandASTStatementNode& statement) -> uint32_t {
			switch (statement.type) {
			case CommandASTStatementNode::Type::Empty: {
				return 0;
			}
			case CommandASTStatementNode::Type::Expression: {
				return excute_expression(std::get<CommandASTExpressionNode>(statement.value), nullptr);
			}
			default: {
				throw CommandException("Unknown statement type.");
			}
			}
		}

	public: // excute expressions.
		auto excute_expression(const CommandASTExpressionNode& expression, CommandValue* result) -> uint32_t {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Empty: {
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, true };
				}
				return 0;
			}
			case CommandASTExpressionNode::Type::Number: {
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Number, std::get<CommandASTNumberNode>(expression.value).value };
				}
				return 0;
			}
			case CommandASTExpressionNode::Type::String: {
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::String, std::get<CommandASTStringNode>(expression.value).value };
				}
				return 0;
			}
			case CommandASTExpressionNode::Type::Identifier: {
				return excute_identifier(std::get<CommandASTIdentifierNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Operation: {
				return excute_operation(std::get<CommandASTOperationNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::List: {
				return excute_list(std::get<CommandASTListNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				return excute_expression(*(std::get<CommandASTParenthesesNode>(expression.value).expression), result);
			}
			case CommandASTExpressionNode::Type::Calling: {
				return excute_calling(std::get<CommandASTCallingNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::FunctionBody: {
				return excute_function_body(std::get<CommandASTFunctionBodyNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Condition: {
				return excute_condition(std::get<CommandASTConditionNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Assignment: {
				return excute_assignment(std::get<CommandASTAssignmentNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Protection: {
				return excute_protection(std::get<CommandASTProtectionNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Delete: {
				return excute_delete(std::get<CommandASTDeleteNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Argument: {
				return excute_argument(std::get<CommandASTArgumentNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Return: {
				return excute_return(std::get<CommandASTReturnNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Self: {
				return excute_self(std::get<CommandASTSelfNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Loop: {
				return excute_loop(std::get<CommandASTLoopNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::Accessing: {
				return excute_accessing(std::get<CommandASTAccessingNode>(expression.value), result);
			}
			default: {
				throw CommandException("Unexcutable expression type.");
			}
			}
		}

		auto excute_identifier(const CommandASTIdentifierNode& identifier, CommandValue* result) -> uint32_t {
			for (auto scope = scope_stack.rbegin(); scope != scope_stack.rend(); ++scope) {
				if (scope->identifiers.find(identifier.name) != scope->identifiers.end()) {
					if (result != nullptr) {
						*result = scope->identifiers.at(identifier.name);
					}
					return 0;
				}
			}
			scope_stack.rbegin()->identifiers.insert({
				identifier.name,
				CommandValue{ CommandValue::Type::Empty, true }
			});
			if (result != nullptr) {
				*result = scope_stack.rbegin()->identifiers.at(identifier.name);
			}
			return 0;
		}

		auto excute_operation(const CommandASTOperationNode& operation, CommandValue* result) -> uint32_t {
			std::vector<CommandValue> operand_results(operation.operands.size());
			for (size_t i = 0; i < operation.operands.size(); ++i) {
				auto return_level = excute_expression(operation.operands[i], &operand_results[i]);
				if (return_level != 0) {
					return return_level;
				}
			}

			if (result != nullptr) {
				switch (operation.type)
				{
				case CommandASTOperationNode::Type::Add: {
					*result = operand_results[0] + operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::Subtract: {
					*result = operand_results[0] - operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::Multiply: {
					*result = operand_results[0] * operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::Divide: {
					*result = operand_results[0] / operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::Positive: {
					*result = +operand_results[0];
					break;
				}
				case CommandASTOperationNode::Type::Negative: {
					*result = -operand_results[0];
					break;
				}
				case CommandASTOperationNode::Type::Not: {
					*result = !operand_results[0];
					break;
				}
				case CommandASTOperationNode::Type::Modulo: {
					*result = operand_results[0] % operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::Exponent: {
					if (operand_results[0].type == CommandValue::Type::Number && operand_results[1].type == CommandValue::Type::Number) {
						*result = CommandValue{ 
							CommandValue::Type::Number, 
							std::pow(std::get<float>(operand_results[0].value), std::get<float>(operand_results[1].value)) 
						};
					}
					else {
						*result = CommandValue{ CommandValue::Type::Empty, true };
					}
					break;
				}
				case CommandASTOperationNode::Type::Equal: {
					*result = operand_results[0] == operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::NotEqual: {
					*result = operand_results[0] != operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::LessThan: {
					*result = operand_results[0] < operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::LessThanOrEqual: {
					*result = operand_results[0] <= operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::GreaterThan: {
					*result = operand_results[0] > operand_results[1];
					break;
				}
				case CommandASTOperationNode::Type::GreaterThanOrEqual: {
					*result = operand_results[0] >= operand_results[1];
					break;
				}
				default:
					throw CommandException("Unknow operation.");
				}
			}
			return 0;
		}

		auto excute_list(const CommandASTListNode& list, CommandValue* result) -> uint32_t {
			//std::vector<CommandValue> element_results(list.expressions.size());
			if (result != nullptr) {
				*result = CommandValue{ CommandValue::Type::List, std::vector<CommandValue>(list.expressions.size()) };
				auto& element_results = std::get<std::vector<CommandValue>>(result->value);
				for (size_t i = 0; i < list.expressions.size(); ++i) {
					auto return_level = excute_expression(list.expressions[i], &element_results[i]);
					if (return_level != 0) {
						return return_level;
					}
				}
			}
			else {
				for (size_t i = 0; i < list.expressions.size(); ++i) {
					auto return_level = excute_expression(list.expressions[i], nullptr);
					if (return_level != 0) {
						return return_level;
					}
				}
			}
			return 0;
		}

		auto excute_calling(const CommandASTCallingNode& calling, CommandValue* result) -> uint32_t {
			CommandValue callable;
			auto return_level = excute_expression(*(calling.callable), &callable);
			if (return_level != 0) {
				return return_level;
			}

			CommandValue argument;
			return_level = excute_expression(*(calling.argument), &argument);
			if (return_level != 0) {
				return return_level;
			}

			switch (callable.type)
			{
			case CommandValue::Type::Function: {
				if (scope_stack.size() >= 1000) {
					throw CommandException("Stack overflow.");
				}
				scope_stack.push_back({ });

				auto& function = std::get<std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>>(callable.value);
				function(argument.type == CommandValue::Type::List ? std::get<std::vector<CommandValue>>(argument.value) : std::vector{ argument }, result);

				scope_stack.pop_back();

				break;
			}
			case CommandValue::Type::Macro: {
				auto& function = std::get<std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>>(callable.value);
				function(argument.type == CommandValue::Type::List ? std::get<std::vector<CommandValue>>(argument.value) : std::vector{ argument }, result);
				break;
			}
			case CommandValue::Type::List: {
				auto& list = std::get<std::vector<CommandValue>>(callable.value);
				if (argument.type == CommandValue::Type::Number) {
					auto index = std::llround(std::get<float>(argument.value));
					if ((index < -static_cast<int64_t>(list.size())) || (index >= static_cast<int64_t>(list.size()))) {
						throw CommandException("Index({}) out of range(-{}..{}).", index, list.size(), list.size());
					}
					if (index < 0) {
						index += list.size();
					}
					if (result != nullptr) {
						*result = list[index];
					}
				}
				else {
					throw CommandException("Index must be a number.");
				}
				break;
			}
			default: {
				throw CommandException("{} is not callable.", to_string(callable.type));
			}
			}
			return 0;
		}

		auto excute_function_body(const CommandASTFunctionBodyNode& body, CommandValue* result) -> uint32_t {
			// shard_ptr is the best of the bests!!!!!!!!!!!!!!!!!
			// I love it!!!!!!!!!!!!!!!!!!!!!
			// It solved a problem on which I spent hours!!!!!!!!!!!!!!!!!
			if (result == nullptr) {
				return 0;
			}

			struct body_callable {
				std::shared_ptr<CommandASTFunctionBodyNode> body;
				CommandKernel& kernel;
				auto operator()(const std::vector<CommandValue>& arguments, CommandValue* result) const -> uint32_t {
					std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)> self = *this;
					kernel.body_stack.emplace_back(
						&arguments,
						&self
					);
					while (true) {
						for (auto& statement : body->commands) {
							auto return_level = kernel.excute_statement(statement);
							if (return_level == 0) {
								continue;
							}
							else if (return_level == 1) {
								if (kernel.requiring_loop) {
									break;
								}
								else if (result != nullptr) {
									*result = kernel.body_stack.back().return_value;
								}
							}
							kernel.body_stack.pop_back();
							return return_level - 1;
						}
						if (kernel.requiring_loop) {
							kernel.requiring_loop = false;
							continue;
						}
						break;
					}
					if (result != nullptr) {
						*result = kernel.body_stack.back().return_value;
					}
					kernel.body_stack.pop_back();
					return 0;
				}
				body_callable(std::shared_ptr<CommandASTFunctionBodyNode> body, CommandKernel& kernel) : body(body), kernel(kernel) { }
				body_callable(const body_callable& other) : body(other.body), kernel(other.kernel) { }
				body_callable(body_callable&& other) noexcept : body(std::move(other.body)), kernel(other.kernel) { }
				auto operator=(const body_callable& other) -> body_callable& {
					body = other.body;
					kernel = other.kernel;
					return *this;
				}
				auto operator=(body_callable&& other) noexcept  -> body_callable& {
					body = std::move(other.body);
					kernel = other.kernel;
					return *this;
				}
			};

			std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)> function = 
				body_callable(std::make_shared<CommandASTFunctionBodyNode>(body.clone()), *this);
			*result = CommandValue{ CommandValue::Type::Function, std::move(function) };
			return 0;
		}

		auto excute_condition(const CommandASTConditionNode& condition, CommandValue* result) -> uint32_t {
			CommandValue condition_result;
			if (condition.condition != nullptr) {
				auto return_level = excute_expression(*(condition.condition), &condition_result);
				if (return_level != 0) {
					return return_level;
				}
			}
			if ((condition_result.type == CommandValue::Type::Empty) && (!std::get<bool>(condition_result.value))) {
				if (condition.false_branch != nullptr) {
					return excute_expression(*(condition.false_branch), result);
				}
				else {
					return 0;
				}
			}
			else {
				if (condition.true_branch != nullptr) {
					return excute_expression(*(condition.true_branch), result);
				}
				else {
					return 0;
				}
			}
		}

		auto excute_assignment(const CommandASTAssignmentNode& assignment, CommandValue* result) -> uint32_t {
			if (assignment.local) {
				auto name = get_identifier(*(assignment.target));
				if (scope_stack.back().identifiers.find(name) == scope_stack.back().identifiers.end()) {
					scope_stack.back().identifiers.insert({ name, CommandValue{ CommandValue::Type::Empty, true } });
				}
				else {
					if (scope_stack.back().protections.contains(name)) {
						throw CommandException("cannot assign to protected identifier `{}`.", name);
					}
				}
				auto& identifier = scope_stack.back().identifiers.at(name);

				auto return_level = excute_expression(*(assignment.expression), &identifier);
				if (return_level != 0) {
					return return_level;
				}
				if (result != nullptr) {
					*result = identifier;
				}
			}
			else {
				auto [identifier, is_protected] = get_assignable(*(assignment.target));
				if (is_protected) {
					throw CommandException("cannot assign to protected identifier.");
				}
				auto return_level = excute_expression(*(assignment.expression), &identifier);
				if (return_level != 0) {
					return return_level;
				}
				if (result != nullptr) {
					*result = identifier;
				}
			}
			return 0;
		}

		auto excute_protection(const CommandASTProtectionNode& protection, CommandValue* result) -> uint32_t {
			auto name = get_identifier(*(protection.target));
			auto [identifier, _] = find_identifier_or_throw(name);
			scope_stack.back().protections.insert(name);
			if (result != nullptr) {
				*result = identifier;
			}
			return 0;
		}

		auto excute_delete(const CommandASTDeleteNode& deletion, CommandValue* result) -> uint32_t {
			auto name = get_identifier(*(deletion.target));
			for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
				if (it->protections.find(name) != it->protections.end()) {
					throw CommandException("`{}` is protected, cannot delete it.", name);
				}
				auto find = it->identifiers.find(name);
				if (find != it->identifiers.end()) {
					if (result != nullptr) {
						*result = find->second;
					}
					it->identifiers.erase(find);
					return 0;
				}
			}
			throw CommandException("Cannot delete a non-existing identifier {}.", name);
		}

		auto excute_argument(const CommandASTArgumentNode& argument, CommandValue* result) -> uint32_t {
			if (argument.length > body_stack.size()) {
				throw CommandException("Body doesn't exist");
			}
			auto skip = argument.length - 1;
			auto& target_stack = body_stack.rbegin()[skip];
			if (target_stack.index >= target_stack.arguments->size()) {
				return excute_return(CommandASTReturnNode::make(
					argument.length, 
					std::make_unique<CommandASTExpressionNode>(CommandASTExpressionNode::make_empty())
				), result);
			}

			if (result != nullptr) {
				*result = (*target_stack.arguments)[target_stack.index];
			}
			++target_stack.index;
			return 0;
		}

		auto excute_return(const CommandASTReturnNode& returning, CommandValue* result) -> uint32_t {
			if (returning.length > body_stack.size()) {
				throw CommandException("Body doesn't exist.");
			}
			auto skip = returning.length - 1;

			CommandValue return_value;
			auto return_level = excute_expression(*(returning.expression), &return_value);
			if (return_level != 0) {
				return return_level;
			}
			body_stack.rbegin()[skip].return_value = std::move(return_value);
			return returning.length;
		}

		auto excute_accessing(const CommandASTAccessingNode& accessing, CommandValue* result) -> uint32_t {
			CommandValue value;
			auto return_level = excute_expression(*(accessing.expression), &value);
			if (return_level != 0) {
				return return_level;
			}
			switch (value.type)
			{
			case CommandValue::Type::String: {
				return excute_identifier(CommandASTIdentifierNode::make(std::get<std::string>(value.value)), result);
			}
			case CommandValue::Type::Function: {
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Macro, value.value };
				}
				return 0;
			}
			default:
				throw CommandException("{} value type is not accessible.", to_string(value.type));
				break;
			}
		}

		auto excute_self(const CommandASTSelfNode& self, CommandValue* result) -> uint32_t {
			if (result != nullptr) {
				if (self.length > body_stack.size()) {
					throw CommandException("Body doesn't exist.");
				}
				auto skip = self.length - 1;
				*result = CommandValue{ CommandValue::Type::Function, *(body_stack.rbegin()[skip].self)};
			}
			return 0;
		}

		auto excute_loop(const CommandASTLoopNode& loop, CommandValue* result) -> uint32_t {
			if (loop.length > body_stack.size()) {
				throw CommandException("Body doesn't exist.");
			}
			if (loop.argument != nullptr) {
				CommandValue argument;
				auto return_level = excute_expression(*(loop.argument), &argument);
				if (return_level != 0) {
					return return_level;
				}
				auto skip = loop.length - 1;
				auto& target_stack = body_stack.rbegin()[skip];
				target_stack.index = 0;
				target_stack.owned_arguments = std::make_shared<std::vector<CommandValue>>(argument.type == CommandValue::Type::List ? std::move(std::get<std::vector<CommandValue>>(argument.value)) : std::vector{ argument });
				target_stack.arguments = target_stack.owned_arguments.get();
			}
			requiring_loop = true;
			return loop.length;
		}

	public: // get assignables.
		auto get_assignable(const CommandASTExpressionNode& expression) -> std::pair<CommandValue&, bool> {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Identifier: {
				return get_assignable_from_identifier(std::get<CommandASTIdentifierNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::List: {
				throw CommandException("List expression is not assignable.\nFuture feature: unpacking.");
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				return get_assignable_from_parentheses(std::get<CommandASTParenthesesNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Calling: {
				return get_assignable_from_calling(std::get<CommandASTCallingNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Condition: {
				return get_assignable_from_condition(std::get<CommandASTConditionNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Protection: {
				return get_assignable_from_protection(std::get<CommandASTProtectionNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Accessing: {
				return get_assignable_from_accessing(std::get<CommandASTAccessingNode>(expression.value));
			}
			default: {
				throw CommandException("{} expression is not assignable", to_string(expression.type));
			}
			}
		}

		auto get_assignable_from_identifier(const CommandASTIdentifierNode& identifier) -> std::pair<CommandValue&, bool> {
			return find_identifier_or_insert(identifier.name);
		}

		auto get_assignable_from_parentheses(const CommandASTParenthesesNode& parentheses) -> std::pair<CommandValue&, bool> {
			return get_assignable(*(parentheses.expression));
		}

		auto get_assignable_from_calling(const CommandASTCallingNode& calling) -> std::pair<CommandValue&, bool> {
			auto [callable, is_protected] = get_assignable(*(calling.callable));
			CommandValue argument;
			auto return_level = excute_expression(*(calling.argument), &argument);
			if (return_level != 0) {
				throw CommandException("Cannot return from assignable");
			}
			if (callable.type == CommandValue::Type::List) {
				auto& list = std::get<std::vector<CommandValue>>(callable.value);
				if (argument.type == CommandValue::Type::Number) {
					auto index = std::llround(std::get<float>(argument.value));
					if ((index < -static_cast<int64_t>(list.size())) || (index >= static_cast<int64_t>(list.size()))) {
						throw CommandException("Index({}) out of range(-{}..{}).", index, list.size(), list.size());
					}
					if (index < 0) {
						index += list.size();
					}
					return { list[index], is_protected };
				}
				else {
					throw CommandException("Index must be a number.");
				}
			}
			else {
				throw CommandException("{} Calling is not assignable", to_string(callable.type));
			}
		}

		auto get_assignable_from_condition(const CommandASTConditionNode& condition) -> std::pair<CommandValue&, bool> {
			CommandValue condition_result;
			if (condition.condition != nullptr) {
				auto return_level = excute_expression(*(condition.condition), &condition_result);
				if (return_level != 0) {
					throw CommandException("Cannot return from assignable");
				}
			}
			if ((condition_result.type == CommandValue::Type::Empty) && (!std::get<bool>(condition_result.value))) {
				if (condition.false_branch != nullptr) {
					return get_assignable(*(condition.false_branch));
				}
				else {
					throw CommandException("Empty expression is not assignable.");
				}
			}
			else {
				if (condition.true_branch != nullptr) {
					return get_assignable(*(condition.true_branch));
				}
				else {
					throw CommandException("Empty expression is not assignable.");
				}
			}
		}

		auto get_assignable_from_protection(const CommandASTProtectionNode& protection) -> std::pair<CommandValue&, bool> {
			auto name = get_identifier(*(protection.target));
			auto [assignable, is_protected] = find_identifier_or_insert(name);
			scope_stack.back().protections.insert(name);
			return { assignable, is_protected };
		}

		auto get_assignable_from_accessing(const CommandASTAccessingNode& accessing) -> std::pair<CommandValue&, bool> {
			CommandValue value;
			auto return_level = excute_expression(*(accessing.expression), &value);
			if (return_level != 0) {
				throw CommandException("Cannot return from assignable");
			}
			switch (value.type)
			{
			case CommandValue::Type::String: {
				return get_assignable_from_identifier(CommandASTIdentifierNode::make(std::get<std::string>(value.value)));
			}
			default:
				throw CommandException("{} value type is not accessible.", to_string(value.type));
				break;
			}
		}

	public: // get identifiers.
		auto get_identifier(const CommandASTExpressionNode& expression) -> std::string {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Identifier: {
				return get_identifier_from_identifier(std::get<CommandASTIdentifierNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Parentheses: {
				return get_identifier_from_parentheses(std::get<CommandASTParenthesesNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Condition: {
				return get_identifier_from_condition(std::get<CommandASTConditionNode>(expression.value));
			}
			case CommandASTExpressionNode::Type::Accessing: {
				throw CommandException("Unimplemented yet.");
			}
			default: {
				throw CommandException("{} expression cannot be evaluated to an identifier.", to_string(expression.type));
			}
			}
		}

		auto get_identifier_from_identifier(const CommandASTIdentifierNode& identifier) -> std::string {
			return identifier.name;
		}

		auto get_identifier_from_parentheses(const CommandASTParenthesesNode& parentheses) -> std::string {
			return get_identifier(*(parentheses.expression));
		}

		auto get_identifier_from_condition(const CommandASTConditionNode& condition) -> std::string {
			CommandValue condition_result;
			if (condition.condition != nullptr) {
				auto return_level = excute_expression(*(condition.condition), &condition_result);
				if (return_level != 0) {
					throw CommandException("Cannot return from assignable");
				}
			}
			if ((condition_result.type == CommandValue::Type::Empty) && (!std::get<bool>(condition_result.value))) {
				if (condition.false_branch != nullptr) {
					return get_identifier(*(condition.false_branch));
				}
				else {
					throw CommandException("Empty expression cannot be evaluated to an identifier.");
				}
			}
			else {
				if (condition.true_branch != nullptr) {
					return get_identifier(*(condition.true_branch));
				}
				else {
					throw CommandException("Empty expression cannot be evaluated to an identifier.");
				}
			}
		}

		auto get_identifier_from_accessing(const CommandASTAccessingNode& accessing) -> std::string {
			CommandValue value;
			auto return_level = excute_expression(*(accessing.expression), &value);
			if (return_level != 0) {
				throw CommandException("Cannot return from assignable");
			}
			switch (value.type)
			{
			case CommandValue::Type::String: {
				return std::get<std::string>(value.value);
			}
			default:
				throw CommandException("{} value type is not accessible.", to_string(value.type));
				break;
			}
		}

		CommandKernel() {
			scope_stack.reserve(1000);
			//body_stack.reserve(1000);
		}
	};
}