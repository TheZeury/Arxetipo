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

	struct CommandASTParenthesesNode
	{
		std::vector<CommandASTExpressionNode> expressions;

		CommandASTParenthesesNode(CommandASTParenthesesNode&&) = default;
		CommandASTParenthesesNode& operator=(CommandASTParenthesesNode&&) = default;

		CommandASTParenthesesNode(std::vector<CommandASTExpressionNode>&& expressions) : expressions(std::move(expressions)) { }

		static auto make(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTParenthesesNode {
			return CommandASTParenthesesNode{ std::move(expressions) };
		}

		auto clone() const -> CommandASTParenthesesNode {
			return CommandASTParenthesesNode{ clone_vector(expressions) };
		}
	};

	struct CommandASTStatementNode;
	struct CommandASTMethodCallNode
	{
		std::unique_ptr<CommandASTExpressionNode> method_body;
		std::vector<CommandASTExpressionNode> arguments;

		CommandASTMethodCallNode(CommandASTMethodCallNode&&) = default;
		CommandASTMethodCallNode& operator=(CommandASTMethodCallNode&&) = default;

		CommandASTMethodCallNode(std::unique_ptr<CommandASTExpressionNode>&& method_body, std::vector<CommandASTExpressionNode>&& arguments) : method_body(std::move(method_body)), arguments(std::move(arguments)) { }

		static auto make(std::unique_ptr<CommandASTExpressionNode>&& method_body, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTMethodCallNode {
			return CommandASTMethodCallNode{ std::move(method_body), std::move(arguments) };
		}

		auto clone() const -> CommandASTMethodCallNode {
			return CommandASTMethodCallNode{ clone_pointer(method_body), clone_vector(arguments)};
		}
	};

	struct CommandASTMethodBodyNode
	{
		std::vector<CommandASTStatementNode> commands;

		CommandASTMethodBodyNode(CommandASTMethodBodyNode&&) = default;
		CommandASTMethodBodyNode& operator=(CommandASTMethodBodyNode&&) = default;

		CommandASTMethodBodyNode(std::vector<CommandASTStatementNode>&& commands) : commands(std::move(commands)) { }

		static auto make(std::vector<CommandASTStatementNode>&& commands) -> CommandASTMethodBodyNode {
			return CommandASTMethodBodyNode{ std::move(commands) };
		}

		auto clone() const -> CommandASTMethodBodyNode {
			return CommandASTMethodBodyNode{ clone_vector(commands) };
		}
	};

	struct CommandASTExpressionNode
	{
		enum class Type
		{
			Empty,
			Number,
			Identifier,
			Operation,
			Parentheses,
			MethodCall,
			MethodBody,
		};
		Type type;
		std::variant<
			CommandASTNoneNode,
			CommandASTNumberNode, 
			CommandASTIdentifierNode, 
			CommandASTOperationNode,
			CommandASTParenthesesNode,
			CommandASTMethodCallNode, 
			CommandASTMethodBodyNode
		> value;

		CommandASTExpressionNode(CommandASTExpressionNode&&) = default;
		CommandASTExpressionNode& operator=(CommandASTExpressionNode&&) = default;

		CommandASTExpressionNode(Type type, 
			std::variant<
				CommandASTNoneNode,
				CommandASTNumberNode,
				CommandASTIdentifierNode,
				CommandASTOperationNode,
				CommandASTParenthesesNode,
				CommandASTMethodCallNode,
				CommandASTMethodBodyNode
			>&& value) : type(type), value(std::move(value)) { }

		static auto make_number(float value) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Number,
				CommandASTNumberNode{ value }
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
		static auto make_parentheses(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::Parentheses,
				CommandASTParenthesesNode{ std::move(expressions) }
			};
		}
		static auto make_method_call(std::unique_ptr<CommandASTExpressionNode>&& method_body, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodCall,
				CommandASTMethodCallNode{ std::move(method_body), std::move(arguments) }
			};
		}
		static auto make_method_body(std::vector<CommandASTStatementNode>&& commands) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodBody,
				CommandASTMethodBodyNode{ std::move(commands) }
			};
		}

		auto clone() const -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ type, std::visit( [](auto& v) -> decltype(value) { return v.clone(); }, value) };
		}
	}; 
	// ExpressionNode complete.

	// StatementNode begin.
	struct CommandASTStatementNode;

	struct CommandASTAssignmentNode
	{
		std::string name;
		CommandASTExpressionNode expression;

		CommandASTAssignmentNode(CommandASTAssignmentNode&&) = default;
		CommandASTAssignmentNode& operator=(CommandASTAssignmentNode&&) = default;

		CommandASTAssignmentNode(const std::string& name, CommandASTExpressionNode&& expression) : name(name), expression(std::move(expression)) { }

		static auto make(const std::string& name, CommandASTExpressionNode&& expression) -> CommandASTAssignmentNode {
			return CommandASTAssignmentNode{ name, std::move(expression) };
		}

		auto clone() const -> CommandASTAssignmentNode {
			return CommandASTAssignmentNode{ name, expression.clone() };
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
		size_t length;
		std::string name;

		CommandASTArgumentNode(CommandASTArgumentNode&&) = default;
		CommandASTArgumentNode& operator=(CommandASTArgumentNode&&) = default;
		
		CommandASTArgumentNode(size_t length, const std::string& name) : length(length), name(name) { }

		static auto make(size_t length, const std::string& name) -> CommandASTArgumentNode {
			return CommandASTArgumentNode{ length, name };
		}

		auto clone() const -> CommandASTArgumentNode {
			return CommandASTArgumentNode{ length, name };
		}
	};

	struct CommandASTReturnNode
	{
		CommandASTExpressionNode expression;

		CommandASTReturnNode(CommandASTReturnNode&&) = default;
		CommandASTReturnNode& operator=(CommandASTReturnNode&&) = default;

		CommandASTReturnNode(CommandASTExpressionNode&& expression) : expression(std::move(expression)) { }

		static auto make(CommandASTExpressionNode&& expression) -> CommandASTReturnNode {
			return CommandASTReturnNode{ std::move(expression) };
		}

		auto clone() const -> CommandASTReturnNode {
			return CommandASTReturnNode{ expression.clone() };
		}
	};

	struct CommandASTStatementNode
	{
		enum class Type
		{
			Empty,
			Assignment,
			Expression,
			Protection,
			Delete,
			Argument,
			Return,
		};
		Type type;
		std::variant<
			CommandASTNoneNode,
			CommandASTAssignmentNode,
			CommandASTExpressionNode, // TODO: Consider a unique node type rather than using the expression node directly.
			CommandASTProtectionNode,
			CommandASTDeleteNode,
			CommandASTArgumentNode,
			CommandASTReturnNode
		> value;

		CommandASTStatementNode(CommandASTStatementNode&&) = default;
		CommandASTStatementNode& operator=(CommandASTStatementNode&&) = default;

		CommandASTStatementNode(Type type,
			std::variant<
				CommandASTNoneNode,
				CommandASTAssignmentNode,
				CommandASTExpressionNode,
				CommandASTProtectionNode,
				CommandASTDeleteNode,
				CommandASTArgumentNode,
				CommandASTReturnNode
			>&& value) : type(type), value(std::move(value)) { }

		static auto make_assignment(const std::string& name, CommandASTExpressionNode&& expression) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Assignment,
				CommandASTAssignmentNode{ name, std::move(expression) }
			};
		}
		static auto make_expression(CommandASTExpressionNode&& expression) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Expression,
				std::move(expression)
			};
		}
		static auto make_protection(const std::string& name) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Protection,
				CommandASTProtectionNode{ name }
			};
		}
		static auto make_delete(const std::string& name) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Delete,
				CommandASTDeleteNode{ name }
			};
		}
		static auto make_argument(size_t length, const std::string& name) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Argument,
				CommandASTArgumentNode{ length, name }
			};
		}
		static auto make_return(CommandASTExpressionNode&& expression) -> CommandASTStatementNode {
			return CommandASTStatementNode{ CommandASTStatementNode::Type::Return,
				CommandASTReturnNode{ std::move(expression) }
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
		static auto make_number(float value) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Number,
					CommandASTNumberNode{ value }
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
		static auto make_parentheses(std::vector<CommandASTExpressionNode>&& expressions) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::Parentheses,
					CommandASTParenthesesNode{ std::move(expressions) }
				}
			};
		}
		static auto make_method_call(std::unique_ptr<CommandASTExpressionNode>&& method_body, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodCall,
					CommandASTMethodCallNode{ std::move(method_body), std::move(arguments) }
				}
			};
		}
		static auto make_method_body(std::vector<CommandASTStatementNode>&& commands) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodBody,
					CommandASTMethodBodyNode{ std::move(commands) }
				}
			};
		}
		static auto make_assignment(const std::string& name, CommandASTExpressionNode&& expression) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTStatementNode{ CommandASTStatementNode::Type::Assignment,
					CommandASTAssignmentNode{ name, std::move(expression) }
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
		static auto make_protection(const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTStatementNode{ CommandASTStatementNode::Type::Protection,
					CommandASTProtectionNode{ name }
				}
			};
		}
		static auto make_delete(const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTStatementNode{ CommandASTStatementNode::Type::Delete,
					CommandASTDeleteNode{ name }
				}
			};
		}
		static auto make_argument(size_t length, const std::string& name) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Statement,
				CommandASTStatementNode{ CommandASTStatementNode::Type::Argument,
					CommandASTArgumentNode{ length, name }
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

	auto to_string(CommandASTOperationNode::Type operation) -> std::string {
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

	auto to_operation_type(const std::string& operation) -> CommandASTOperationNode::Type {
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