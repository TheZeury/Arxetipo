#pragma once

namespace arx
{
	template<typename T>
	auto clone_vector(const std::vector<T>& v) -> std::vector<T>;
	template<typename T>
	auto clone_pointer(const std::unique_ptr<T>& v) -> std::unique_ptr<T>;

	// Node begin.
	struct CommandASTNode;

	struct CommandASTNoneNode 
	{
		CommandASTNoneNode(CommandASTNoneNode&&) = default;
		CommandASTNoneNode& operator=(CommandASTNoneNode&&) = default;

		CommandASTNoneNode() = default;

		static auto make() -> CommandASTNoneNode {
			return CommandASTNoneNode{};
		}

		auto clone() const -> CommandASTNoneNode {
			return CommandASTNoneNode{};
		}
	};

	// ExpressionNode begin.
	struct CommandASTExpressionNode; 

	struct CommandASTNumberNode
	{
		float value;

		CommandASTNumberNode(CommandASTNumberNode&&) = default;
		CommandASTNumberNode& operator=(CommandASTNumberNode&&) = default;

		CommandASTNumberNode(float value) : value(value) { }

		static auto make(float value) -> CommandASTNumberNode {
			return CommandASTNumberNode{ value };
		}

		auto clone() const -> CommandASTNumberNode {
			return CommandASTNumberNode{ value };
		}
	};

	struct CommandASTStringNode
	{
		std::string value;

		CommandASTStringNode(CommandASTStringNode&&) = default;
		CommandASTStringNode& operator=(CommandASTStringNode&&) = default;

		CommandASTStringNode(const std::string& value) : value(value) { }

		static auto make(const std::string& value) -> CommandASTStringNode {
			return CommandASTStringNode{ value };
		}

		auto clone() const -> CommandASTStringNode {
			return CommandASTStringNode{ value };
		}
	};

	struct CommandASTIdentifierNode
	{
		std::string name;

		CommandASTIdentifierNode(CommandASTIdentifierNode&&) = default;
		CommandASTIdentifierNode& operator=(CommandASTIdentifierNode&&) = default;

		CommandASTIdentifierNode(const std::string& name) : name(name) { }

		static auto make(const std::string& name) -> CommandASTIdentifierNode {
			return CommandASTIdentifierNode{ name };
		}

		auto clone() const -> CommandASTIdentifierNode {
			return CommandASTIdentifierNode{ name };
		}
	};

	struct CommandASTOperationNode
	{
		enum class Type
		{
			Add,
			Subtract,
			Multiply,
			Divide,
			Positive,
			Negative,
			Modulo,
			Exponent,
			Parentheses,
		};
		Type type;
		uint32_t operand_count;
		std::vector<CommandASTExpressionNode> operands;

		CommandASTOperationNode(CommandASTOperationNode&&) = default;
		CommandASTOperationNode& operator=(CommandASTOperationNode&&) = default;

		CommandASTOperationNode(Type type, uint32_t operand_count, std::vector<CommandASTExpressionNode>&& operands) : type(type), operand_count(operand_count), operands(std::move(operands)) { }

		static auto make(Type type, uint32_t operand_count, std::vector<CommandASTExpressionNode>&& operands) -> CommandASTOperationNode {
			return CommandASTOperationNode{ type, operand_count, std::move(operands) };
		}

		auto clone() const -> CommandASTOperationNode {
			return CommandASTOperationNode{ type, operand_count, clone_vector(operands) };
		}
	};

	struct CommandASTListNode {
		std::vector<CommandASTExpressionNode> expressions;

		CommandASTListNode(CommandASTListNode&&) = default;
		CommandASTListNode& operator=(CommandASTListNode&&) = default;

		CommandASTListNode(std::vector<CommandASTExpressionNode>&& expressions) : expressions(std::move(expressions)) { }

		static auto make(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTListNode {
			return CommandASTListNode{ std::move(expressions) };
		}

		auto clone() const -> CommandASTListNode {
			return CommandASTListNode{ clone_vector(expressions) };
		}
	};

	struct CommandASTParenthesesNode
	{
		std::unique_ptr<CommandASTExpressionNode> expression;

		CommandASTParenthesesNode(CommandASTParenthesesNode&&) = default;
		CommandASTParenthesesNode& operator=(CommandASTParenthesesNode&&) = default;

		CommandASTParenthesesNode(std::unique_ptr<CommandASTExpressionNode>&& expression) : expression(std::move(expression)) { }

		static auto make(std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTParenthesesNode {
			return CommandASTParenthesesNode{ std::move(expression) };
		}

		auto clone() const -> CommandASTParenthesesNode {
			return CommandASTParenthesesNode{ clone_pointer(expression) };
		}
	};

	struct CommandASTStatementNode;
	struct CommandASTFunctionCallNode
	{
		std::unique_ptr<CommandASTExpressionNode> function_body;
		std::unique_ptr<CommandASTExpressionNode> argument;

		CommandASTFunctionCallNode(CommandASTFunctionCallNode&&) = default;
		CommandASTFunctionCallNode& operator=(CommandASTFunctionCallNode&&) = default;

		CommandASTFunctionCallNode(std::unique_ptr<CommandASTExpressionNode>&& function_body, std::unique_ptr<CommandASTExpressionNode>&& argument) : function_body(std::move(function_body)), argument(std::move(argument)) { }

		static auto make(std::unique_ptr<CommandASTExpressionNode>&& function_body, std::unique_ptr<CommandASTExpressionNode>&& argument) -> CommandASTFunctionCallNode {
			return CommandASTFunctionCallNode{ std::move(function_body), std::move(argument) };
		}

		auto clone() const -> CommandASTFunctionCallNode {
			return CommandASTFunctionCallNode{ clone_pointer(function_body), clone_pointer(argument)};
		}
	};

	struct CommandASTFunctionBodyNode
	{
		std::vector<CommandASTStatementNode> commands;

		CommandASTFunctionBodyNode(CommandASTFunctionBodyNode&&) = default;
		CommandASTFunctionBodyNode& operator=(CommandASTFunctionBodyNode&&) = default;

		CommandASTFunctionBodyNode(std::vector<CommandASTStatementNode>&& commands) : commands(std::move(commands)) { }

		static auto make(std::vector<CommandASTStatementNode>&& commands) -> CommandASTFunctionBodyNode {
			return CommandASTFunctionBodyNode{ std::move(commands) };
		}

		auto clone() const -> CommandASTFunctionBodyNode {
			return CommandASTFunctionBodyNode{ clone_vector(commands) };
		}
	};

	struct CommandASTAssignmentNode
	{
		std::string name;
		std::unique_ptr<CommandASTExpressionNode> expression;

		CommandASTAssignmentNode(CommandASTAssignmentNode&&) = default;
		CommandASTAssignmentNode& operator=(CommandASTAssignmentNode&&) = default;

		CommandASTAssignmentNode(const std::string& name, std::unique_ptr<CommandASTExpressionNode>&& expression) : name(name), expression(std::move(expression)) { }

		static auto make(const std::string& name, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTAssignmentNode {
			return CommandASTAssignmentNode{ name, std::move(expression) };
		}

		auto clone() const -> CommandASTAssignmentNode {
			return CommandASTAssignmentNode{ name, clone_pointer(expression) };
		}
	};

	struct CommandASTProtectionNode {
		std::string name;

		CommandASTProtectionNode(CommandASTProtectionNode&&) = default;
		CommandASTProtectionNode& operator=(CommandASTProtectionNode&&) = default;

		CommandASTProtectionNode(const std::string& name) : name(name) { }

		static auto make(const std::string& name) -> CommandASTProtectionNode {
			return CommandASTProtectionNode{ name };
		}

		auto clone() const -> CommandASTProtectionNode {
			return CommandASTProtectionNode{ name };
		}
	};

	struct CommandASTDeleteNode
	{
		std::string name;

		CommandASTDeleteNode(CommandASTDeleteNode&&) = default;
		CommandASTDeleteNode& operator=(CommandASTDeleteNode&&) = default;

		CommandASTDeleteNode(const std::string& name) : name(name) { }

		static auto make(const std::string& name) -> CommandASTDeleteNode {
			return CommandASTDeleteNode{ name };
		}

		auto clone() const -> CommandASTDeleteNode {
			return CommandASTDeleteNode{ name };
		}
	};

	struct CommandASTArgumentNode
	{
		uint32_t length;

		CommandASTArgumentNode(CommandASTArgumentNode&&) = default;
		CommandASTArgumentNode& operator=(CommandASTArgumentNode&&) = default;

		CommandASTArgumentNode(uint32_t length) : length(length) { }

		static auto make(uint32_t length) -> CommandASTArgumentNode {
			return CommandASTArgumentNode{ length };
		}

		auto clone() const -> CommandASTArgumentNode {
			return CommandASTArgumentNode{ length };
		}
	};

	struct CommandASTReturnNode
	{
		uint32_t length;
		std::unique_ptr<CommandASTExpressionNode> expression;

		CommandASTReturnNode(CommandASTReturnNode&&) = default;
		CommandASTReturnNode& operator=(CommandASTReturnNode&&) = default;

		CommandASTReturnNode(uint32_t length, std::unique_ptr<CommandASTExpressionNode>&& expression) : length(length), expression(std::move(expression)) { }

		static auto make(uint32_t length, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTReturnNode {
			return CommandASTReturnNode{ length, std::move(expression) };
		}

		auto clone() const -> CommandASTReturnNode {
			return CommandASTReturnNode{ length, clone_pointer(expression) };
		}
	};

	struct CommandASTExpressionNode
	{
		enum class Type
		{
			Empty,
			Number,
			String,
			Identifier,
			Operation,
			List,
			Parentheses,
			FunctionCall,
			FunctionBody,

			Assignment,
			Protection,
			Delete,
			Argument,
			Return,
		};
		Type type;
		std::variant<
			CommandASTNoneNode,
			CommandASTNumberNode, 
			CommandASTStringNode,
			CommandASTIdentifierNode, 
			CommandASTOperationNode,
			CommandASTListNode,
			CommandASTParenthesesNode,
			CommandASTFunctionCallNode, 
			CommandASTFunctionBodyNode,

			CommandASTAssignmentNode,
			CommandASTProtectionNode,
			CommandASTDeleteNode,
			CommandASTArgumentNode,
			CommandASTReturnNode
		> value;

		CommandASTExpressionNode(CommandASTExpressionNode&&) = default;
		CommandASTExpressionNode& operator=(CommandASTExpressionNode&&) = default;

		CommandASTExpressionNode(Type type, 
			std::variant<
				CommandASTNoneNode,
				CommandASTNumberNode,
				CommandASTStringNode,
				CommandASTIdentifierNode,
				CommandASTOperationNode,
				CommandASTListNode,
				CommandASTParenthesesNode,
				CommandASTFunctionCallNode,
				CommandASTFunctionBodyNode,

				CommandASTAssignmentNode,
				CommandASTProtectionNode,
				CommandASTDeleteNode,
				CommandASTArgumentNode,
				CommandASTReturnNode
			>&& value) : type(type), value(std::move(value)) { }

		static auto make_empty() -> CommandASTExpressionNode {
			return CommandASTExpressionNode { CommandASTExpressionNode::Type::Empty,
				CommandASTNoneNode{ }
			};
		}
		static auto make_number(float value) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Number,
				CommandASTNumberNode{ value }
			};
		}
		static auto make_string(const std::string& value) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::String,
				CommandASTStringNode{ value }
			};
		}
		static auto make_identifier(const std::string& name) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Identifier,
				CommandASTIdentifierNode{ name }
			};
		}
		static auto make_operation(CommandASTOperationNode::Type type, uint32_t operand_count, std::vector<CommandASTExpressionNode>&& operands) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Operation,
				CommandASTOperationNode{ type, operand_count, std::move(operands) }
			};
		}
		static auto make_list(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::List,
				CommandASTListNode{ std::move(expressions) }
			};
		}
		static auto make_parentheses(std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Parentheses,
				CommandASTParenthesesNode{ std::move(expression) }
			};
		}
		static auto make_function_call(std::unique_ptr<CommandASTExpressionNode>&& function_body, std::unique_ptr<CommandASTExpressionNode>&& argument) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::FunctionCall,
				CommandASTFunctionCallNode{ std::move(function_body), std::move(argument) }
			};
		}
		static auto make_function_body(std::vector<CommandASTStatementNode>&& commands) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::FunctionBody,
				CommandASTFunctionBodyNode{ std::move(commands) }
			};
		}
		static auto make_assignment(const std::string& name, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Assignment,
				CommandASTAssignmentNode{ name, std::move(expression) }
			};
		}
		static auto make_protection(const std::string& name) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Protection,
				CommandASTProtectionNode{ name }
			};
		}
		static auto make_delete(const std::string& name) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Delete,
				CommandASTDeleteNode{ name }
			};
		}
		static auto make_argument(uint32_t length) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Argument,
				CommandASTArgumentNode{ length }
			};
		}
		static auto make_return(uint32_t length, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Return,
				CommandASTReturnNode{ length, std::move(expression) }
			};
		}

		auto clone() const -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ type, std::visit( [](auto& v) -> decltype(value) { return v.clone(); }, value) };
		}
	}; 
	// ExpressionNode complete.

	// StatementNode begin.
	struct CommandASTStatementNode;

	struct CommandASTStatementNode
	{
		enum class Type
		{
			Empty,
			Expression,
		};
		Type type;
		std::variant<
			CommandASTNoneNode,
			CommandASTExpressionNode // TODO: Consider a unique node type rather than using the expression node directly.
		> value;

		CommandASTStatementNode(CommandASTStatementNode&&) = default;
		CommandASTStatementNode& operator=(CommandASTStatementNode&&) = default;

		CommandASTStatementNode(Type type,
			std::variant<
				CommandASTNoneNode,
				CommandASTExpressionNode
			>&& value) : type(type), value(std::move(value)) { }

		static auto make_expression(CommandASTExpressionNode&& expression) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Expression,
				std::move(expression)
			};
		}

		auto clone() const -> CommandASTStatementNode {
			return CommandASTStatementNode{ type, std::visit([](auto& v) -> decltype(value) { return v.clone(); }, value) };
		}
	};
	// StatementNode complete.

	struct CommandASTNode
	{
		enum class Type
		{
			None,
			Expression,
			Statement,
		};
		Type type;
		std::variant<
			CommandASTNoneNode,
			CommandASTExpressionNode,
			CommandASTStatementNode
		> value;

		CommandASTNode(CommandASTNode&&) = default;
		CommandASTNode& operator=(CommandASTNode&&) = default;

		CommandASTNode(Type type, std::variant<CommandASTNoneNode, CommandASTExpressionNode, CommandASTStatementNode>&& value) : type(type), value(std::move(value)) { }

		static auto make_none() -> CommandASTNode {
			return CommandASTNode{ Type::None, CommandASTNoneNode{ } };
		}
		static auto make_empty() -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode { CommandASTExpressionNode::Type::Empty, { } }
			};
		}
		static auto make_number(float value) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Number,
					CommandASTNumberNode{ value }
				}
			};
		}
		static auto make_string(const std::string& value) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::String,
					CommandASTStringNode{ value }
				}
			};
		}
		static auto make_identifier(const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Identifier,
					CommandASTIdentifierNode{ name }
				}
			};
		}
		static auto make_operation(CommandASTOperationNode::Type type, uint32_t operand_count, std::vector<CommandASTExpressionNode>&& operands) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Operation,
					CommandASTOperationNode{ type, operand_count, std::move(operands) }
				}
			};
		}
		static auto make_list(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::List,
					CommandASTListNode{ std::move(expressions) }
				}
			};
		}
		static auto make_parentheses(std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Parentheses,
					CommandASTParenthesesNode{ std::move(expression) }
				}
			};
		}
		static auto make_function_call(std::unique_ptr<CommandASTExpressionNode>&& function_body, std::unique_ptr<CommandASTExpressionNode>&& argument) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::FunctionCall,
					CommandASTFunctionCallNode{ std::move(function_body), std::move(argument) }
				}
			};
		}
		static auto make_function_body(std::vector<CommandASTStatementNode>&& commands) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::FunctionBody,
					CommandASTFunctionBodyNode{ std::move(commands) }
				}
			};
		}
		static auto make_assignment(const std::string& name, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Assignment,
					CommandASTAssignmentNode{ name, std::move(expression) }
				}
			};
		}
		static auto make_protection(const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Protection,
					CommandASTProtectionNode{ name }
				}
			};
		}
		static auto make_delete(const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Delete,
					CommandASTDeleteNode{ name }
				}
			};
		}
		static auto make_argument(uint32_t length) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Argument,
					CommandASTArgumentNode{ length }
				}
			};
		}
		static auto make_return(uint32_t length, std::unique_ptr<CommandASTExpressionNode>&& expression) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Return,
					CommandASTReturnNode{ length, std::move(expression) }
				}
			};
		}
		static auto make_expression_statement(CommandASTExpressionNode&& expression) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTStatementNode{ CommandASTStatementNode::Type::Expression,
					std::move(expression)
				}
			};
		}

		auto clone() const -> CommandASTNode {
			return CommandASTNode{ type, std::visit([](auto& v) -> decltype(value) { return v.clone(); }, value) };
		}
	};

	template<typename T>
	auto clone_vector(const std::vector<T>& v) -> std::vector<T> {
		std::vector<T> result;
		result.reserve(v.size());
		for (const auto& e : v) {
			result.push_back(e.clone());
		}
		return result;
	}

	template<typename T>
	auto clone_pointer(const std::unique_ptr<T>& v) -> std::unique_ptr<T> {
		return std::make_unique<T>(v->clone());
	}

	inline auto to_string(CommandASTOperationNode::Type operation) -> std::string {
		switch (operation) {
		case CommandASTOperationNode::Type::Add: {
			return "+";
		}
		case CommandASTOperationNode::Type::Subtract: {
			return "-";
		}
		case CommandASTOperationNode::Type::Multiply: {
			return "*";
		}
		case CommandASTOperationNode::Type::Divide: {
			return "/";
		}
		case CommandASTOperationNode::Type::Positive: {
			return "'+";
		}
		case CommandASTOperationNode::Type::Negative: {
			return "'-";
		}
		case CommandASTOperationNode::Type::Modulo: {
			return "%";
		}
		case CommandASTOperationNode::Type::Exponent: {
			return "^";
		}
		case CommandASTOperationNode::Type::Parentheses: {
			return "(";
		}
		default: {
			throw CommandException("Unknown operation.");
		}
		}
	}

	inline auto to_string(CommandASTExpressionNode::Type type) -> std::string {
		switch (type)
		{
		case arx::CommandASTExpressionNode::Type::Empty:
			return "Empty";
			break;
		case arx::CommandASTExpressionNode::Type::Number:
			return "Number";
			break;
		case arx::CommandASTExpressionNode::Type::String:
			return "String";
			break;
		case arx::CommandASTExpressionNode::Type::Identifier:
			return "Identifier";
			break;
		case arx::CommandASTExpressionNode::Type::Operation:
			return "Operation";
			break;
		case arx::CommandASTExpressionNode::Type::List:
			return "List";
			break;
		case arx::CommandASTExpressionNode::Type::Parentheses:
			return "Parentheses";
			break;
		case arx::CommandASTExpressionNode::Type::FunctionCall:
			return "FunctionCall";
			break;
		case arx::CommandASTExpressionNode::Type::FunctionBody:
			return "FunctionBody";
			break;
		case arx::CommandASTExpressionNode::Type::Assignment:
			return "Assignment";
			break;
		case arx::CommandASTExpressionNode::Type::Protection:
			return "Protection";
			break;
		case arx::CommandASTExpressionNode::Type::Delete:
			return "Delete";
			break;
		case arx::CommandASTExpressionNode::Type::Argument:
			return "Argument";
			break;
		case arx::CommandASTExpressionNode::Type::Return:
			return "Return";
			break;
		default:
			return "Unknown";
			break;
		}
	}

	inline auto to_operation_type(const std::string& operation) -> CommandASTOperationNode::Type {
		if (operation == "+") {
			return CommandASTOperationNode::Type::Add;
		}
		if (operation == "-") {
			return CommandASTOperationNode::Type::Subtract;
		}
		if (operation == "*") {
			return CommandASTOperationNode::Type::Multiply;
		}
		if (operation == "/") {
			return CommandASTOperationNode::Type::Divide;
		}
		if (operation == "'+") {
			return CommandASTOperationNode::Type::Positive;
		}
		if (operation == "'-") {
			return CommandASTOperationNode::Type::Negative;
		}
		if (operation == "'%") {
			return CommandASTOperationNode::Type::Modulo;
		}
		throw CommandException("Unknown operation.");
	}

	struct CommandAST
	{

		CommandASTNode root;
	};
}