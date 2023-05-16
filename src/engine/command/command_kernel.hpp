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
		};
		Type type;
		std::variant<
			std::monostate, 
			float, 
			std::string, 
			std::vector<CommandValue>, 
			std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>
		> value;

		CommandValue(
			Type type = Type::Empty, 
			std::variant<
				std::monostate,
				float,
				std::string,
				std::vector<CommandValue>,
				std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>
			> value = std::monostate{ }
		) : type{ type }, value{ value } {
		}

		auto to_string() const -> std::string {
			switch (type) {
			case Type::Empty: {
				return "()";
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
			if (type == Type::String || other.type == Type::String) {
				return CommandValue{ Type::String, to_string() + other.to_string() };
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
		struct StackFrame {
			std::unordered_map<std::string, CommandValue> identifiers;
			std::unordered_set<std::string> protections;
		};
		std::vector<StackFrame> scope_stack{ 1 };

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
			scope_stack.rbegin()->insert({ name, CommandValue{ CommandValue::Type::Empty, std::monostate{ } } });
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
			scope_stack.rbegin()->identifiers.insert({ name, CommandValue{ CommandValue::Type::Empty, std::monostate{ } } });
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

		auto excute_expression(const CommandASTExpressionNode& expression, CommandValue* result) -> uint32_t {
			switch (expression.type) {
			case CommandASTExpressionNode::Type::Empty: {
				if (result != nullptr) {
					*result = CommandValue{ CommandValue::Type::Empty, std::monostate{ } };
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
			case CommandASTExpressionNode::Type::FunctionCall: {
				return excute_function_call(std::get<CommandASTFunctionCallNode>(expression.value), result);
			}
			case CommandASTExpressionNode::Type::FunctionBody: {
				return excute_function_body(std::get<CommandASTFunctionBodyNode>(expression.value), result);
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
				CommandValue{ CommandValue::Type::Empty, std::monostate{ } }
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

		auto excute_function_call(const CommandASTFunctionCallNode& function_call, CommandValue* result) -> uint32_t {
			CommandValue function;
			auto return_level = excute_expression(*(function_call.function_body), &function);
			if (return_level != 0) {
				return return_level;
			}

			if (function.type != CommandValue::Type::Function) {
				throw CommandException("Trying to call a non-function.");
			}

			CommandValue argument;
			return_level = excute_expression(*(function_call.argument), &argument);
			if (return_level != 0) {
				return return_level;
			}

			if (argument.type == CommandValue::Type::List) {
				const auto& arguments = std::get<std::vector<CommandValue>>(argument.value);
				return std::get<std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>>(function.value)(arguments, result);
			}
			else {
				return std::get<std::function<uint32_t(const std::vector<CommandValue>&, CommandValue*)>>(function.value)({ argument }, result);
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
			std::shared_ptr<CommandASTFunctionBodyNode> function_body = std::make_shared<CommandASTFunctionBodyNode>(body.clone());
			auto function = [&, function_body](const std::vector<CommandValue>& arguments, CommandValue* result) -> uint32_t {
				if (scope_stack.size() >= 1000) {
					throw CommandException("Stack overflow.");
				}
				scope_stack.push_back({ });
				scope_stack.back().identifiers.insert({ "@", CommandValue{ CommandValue::Type::List, arguments } });
				scope_stack.back().identifiers.insert({ "@i", CommandValue{ CommandValue::Type::Number, 0.f } });
				for (auto& statement : function_body->commands) {
					auto return_level = excute_statement(statement);
					if (return_level != 0) {
						if (result != nullptr && return_level == 1) {
							*result = (scope_stack.back().identifiers.find("&") != scope_stack.back().identifiers.end()) ? scope_stack.back().identifiers.at("&") : CommandValue{ CommandValue::Type::Empty, std::monostate{} };
						}
						scope_stack.pop_back();
						return return_level - 1;
					}
				}
				if (result != nullptr) {
					*result = (scope_stack.back().identifiers.find("&") != scope_stack.back().identifiers.end()) ? scope_stack.back().identifiers.at("&") : CommandValue{ CommandValue::Type::Empty, std::monostate{} };
				}
				scope_stack.pop_back();
				return 0;
			};
			*result = CommandValue{ CommandValue::Type::Function, function };
			return 0;
		}

		auto excute_assignment(const CommandASTAssignmentNode& assignment, CommandValue* result) -> uint32_t {
			auto [identifier, is_protected] = find_identifier_or_insert(assignment.name);
			if (is_protected) {
				throw CommandException("`{}` is protected, cannot assign to this name.", assignment.name);
			}
			auto return_level = excute_expression(*(assignment.expression), &identifier);
			if (return_level != 0) {
				return return_level;
			}
			if (result != nullptr) {
				*result = identifier;
			}
			return 0;
		}

		auto excute_protection(const CommandASTProtectionNode& protection, CommandValue* result) -> uint32_t {
			find_identifier_or_throw(protection.name);
			scope_stack.back().protections.insert(protection.name);
			return 0;
		}

		auto excute_delete(const CommandASTDeleteNode& deletion, CommandValue* result) -> uint32_t {
			for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
				if (it->protections.find(deletion.name) != it->protections.end()) {
					throw CommandException("`{}` is protected, cannot delete it.", deletion.name);
				}
				auto find = it->identifiers.find(deletion.name);
				if (find != it->identifiers.end()) {
					it->identifiers.erase(find);
					return 0;
				}
			}
			throw CommandException("Cannot delete a non-exist identifier {}.", deletion.name);
		}

		auto excute_argument(const CommandASTArgumentNode& argument, CommandValue* result) -> uint32_t {
			if (argument.length > scope_stack.size()) {
				throw CommandException("Scope doesn't exist");
			}
			auto skip = argument.length - 1;
			if (scope_stack.rbegin()[skip].identifiers.find("@") == scope_stack.rbegin()[skip].identifiers.end()) {
				excute_return(CommandASTReturnNode::make(
					argument.length, 
					std::make_unique<CommandASTExpressionNode>(CommandASTExpressionNode::make_empty())
				), result);
			}
			if (scope_stack.rbegin()[skip].identifiers.at("@").type != CommandValue::Type::List) {
				throw CommandException("Unexpected Kernel Error. Argument is not or is not converted to a list.");
			}

			auto& i_v = scope_stack.rbegin()[skip].identifiers.at("@i");
			size_t i = std::llround(std::get<float>(i_v.value));
			auto& arguments = std::get<std::vector<CommandValue>>(scope_stack.rbegin()[skip].identifiers.at("@").value);
			if (i >= arguments.size()) {
				excute_return(CommandASTReturnNode::make(
					argument.length,
					std::make_unique<CommandASTExpressionNode>(CommandASTExpressionNode::make_empty())
				), result);
			}
			i_v.value = float(i + 1);
			if (result != nullptr) {
				*result = arguments[i];
			}
			return 0;
		}

		auto excute_return(const CommandASTReturnNode& returning, CommandValue* result) -> uint32_t {
			if (returning.length > scope_stack.size()) {
				throw CommandException("Scope doesn't exist.");
			}
			auto skip = returning.length - 1;

			CommandValue return_value;
			auto return_level = excute_expression(*(returning.expression), &return_value);
			if (return_level != 0) {
				return return_level;
			}
			scope_stack.rbegin()[skip].identifiers.insert({ "&", std::move(return_value) });
			return returning.length;
		}

		CommandKernel() {
			scope_stack.reserve(1000);
		}
	};
}