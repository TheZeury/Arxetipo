#pragma once

namespace arx
{
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
	};

	struct CommandASTMethodCallNode
	{
		std::string method_name;
		std::vector<CommandASTExpressionNode> arguments;

		CommandASTMethodCallNode(CommandASTMethodCallNode&&) = default;
		CommandASTMethodCallNode& operator=(CommandASTMethodCallNode&&) = default;

		CommandASTMethodCallNode(const std::string& method_name, std::vector<CommandASTExpressionNode>&& arguments) : method_name(method_name), arguments(std::move(arguments)) { }

		static auto make(const std::string& method_name, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTMethodCallNode {
			return CommandASTMethodCallNode{ method_name, std::move(arguments) };
		}
	};

	struct CommandASTMethodBodyNode
	{
		std::vector<CommandASTNode> commands;
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
			std::monostate,
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
				std::monostate,
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
		static auto make_method_call(const std::string& method_name, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTExpressionNode {
			return CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodCall,
				CommandASTMethodCallNode{ method_name, std::move(arguments) }
			};
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
	};

	struct CommandASTDeleteNode
	{
		std::string name;
	};

	struct CommandASTReturnNode
	{
		CommandASTExpressionNode expression;
	};

	struct CommandASTStatementNode
	{
		enum class Type
		{
			Empty,
			Assignment,
			Expression,
			Delete,
			Return,
		};
		Type type;
		std::variant<
			std::monostate,
			CommandASTAssignmentNode,
			CommandASTExpressionNode, // TODO: Consider a unique node type rather than using the expression node directly.
			CommandASTDeleteNode,
			CommandASTReturnNode
		> value;

		CommandASTStatementNode(CommandASTStatementNode&&) = default;
		CommandASTStatementNode& operator=(CommandASTStatementNode&&) = default;

		CommandASTStatementNode(Type type,
			std::variant<
				std::monostate,
				CommandASTAssignmentNode,
				CommandASTExpressionNode,
				CommandASTDeleteNode,
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
	};
	// StatementNode complete.

	struct CommandASTNode
	{
		enum class Type
		{
			None,
			Expression,
			Statement,
			MethodDefination,
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
		static auto make_method_call(const std::string& method_name, std::vector<CommandASTExpressionNode>&& arguments) -> CommandASTNode {
			return CommandASTNode{ CommandASTNode::Type::Expression,
				CommandASTExpressionNode{ CommandASTExpressionNode::Type::MethodCall,
					CommandASTMethodCallNode{ method_name, std::move(arguments) }
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
	};



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