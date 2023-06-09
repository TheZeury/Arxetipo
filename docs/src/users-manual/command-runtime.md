# Arxemand

Arxemand is a simple scripting language for Arxetipo Engine. 
It can serve several purposes, such as:

1. In game command support for players.
2. Hot-loading temporary logics from servers without updating the game.
3. Writing some quick scripts for testing during development.

It's designed to be dynamic, 
concise and "just enough" to use. 
It's not designed to be a full featured high performance programming language. 
To achieve high performance and functionality, 
developers still need to write real codes in C++.

A major philosophy of it's design is to not reserve any keywords, 
which may make it's syntax a little bit confusing at first.

`(*)` in this document means the feature is not implemented yet.

## How to excute

### Standalone Analyzer

To have a quick start,
there's a complete CLI analyzer : src/lab_game/arxemand.cpp.

```
.\arxemand [options] [file path] [script arguments(*)]
```

If the file path is not provided, it will run in interactive mode.

Currently there's no way to add external libraries to the analyzer,
so for now, this analyzer is only for demostration.

#### Options:

```
-h, --help                    Show this help message and exit
-a, --ast                     Print the AST of the program and exit
-n, --no-basic                Do not load the basic library
--lib=<name>[,<name>...]      Load the specified libraries
```

#### Built-in libraries:

##### Basic

Basic library is loaded by default.

```
print,
read,
exit,
math, // a marco to load the math library
string, // a marco to load the string library
```

##### Math

```
abs,
round,
floor,
ceil,
sign,
sin,
cos,
tan,
asin,
acos,
atan,
log,
log2,
log10,
ln,
```

##### String

```
split, // split(string, separator)
join, // join(array, separator)
parse, // parse to number
```

### Integrating

To integrate the analyzer into your programs, choices are:

#### Choice 1. Using a provided runtime

Simply provide where the code is read from and where the output is written to 
(and optionally where the error is printed to) to the runtime
and you are good to go.

`arx::CommandRuntime` provides some predefined identifiers 
like `print` and `exit` which can be used in command.

```cpp
#include "../engine/command/command.hpp"

auto main() -> int {
    arx::CommandRuntime runtime{ std::cin, std::cout };
    return runtime.run();
}
```

#### Choice 2. Manually

Chain `kernel`, `parser`, and `lexer` together, 
pass the command to the lexer using `<<` operator.

Tokens generated by the lexer will be passed to the parser, 
and the parser will generate AST which will be executed by the kernel.

```cpp
#include "../engine/command/command.hpp"

auto main() -> int {
    arx::CommandKernel kernel;
    arx::CommandParser parser{ kernel };
    arx::CommandLexer lexer{ parser };

    while (true) {
        try {
            std::string line;
            std::getline(std::cin, line);
            lexer << line;
        }
        catch (const arx::CommandError& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
```

There's no ostream needed because this is not a kernel feature, 
but an implementation.
You can implement your own `print` function like this:

```cpp
kernel.add_function("print", [&](const std::vector<CommandValue>& arguments, CommandValue* result) {
    for (const auto& argument : arguments) {
        std::cout << argument.to_string() << std::endl;
    }
    if (result != nullptr) {
        *result = CommandValue{ CommandValue::Type::Empty, std::monostate{ } };
    }
}, true);
```

It is also possible to pass AST to the kernel directly without a parser, 
or pass tokens to the parser directly without a lexer. 
But most of the time you don't need to do that.

```cpp
using TokenType = arx::CommandToken::Type;

std::vector<arx::CommandToken> tokens {
    { TokenType::Name, "print" },
    { TokenType::Separator, "(" },
    { TokenType::Number, "5" },
    { TokenType::Operator, "+" },
    { TokenType::Number, "10" },
    { TokenType::Separator, ")" },
    { TokenType::Separator, ";" },
};

for (const auto& token : tokens) {
    parser << token;
}

// Output: 15

auto twenty = arx::CommandASTExpressionNode::make_number(20.f);
auto three = arx::CommandASTExpressionNode::make_number(3.f);
auto multiply = arx::CommandASTExpressionNode::make_operation(
    arx::CommandASTOperationNode::Type::Multiply, 
    { std::move(twenty), std::move(three) }
);
auto print = arx::CommandASTExpressionNode::make_identifier("print");
auto function = arx::CommandASTExpressionNode::make_function(
    std::make_unique<CommandASTExpressionNode>(std::move(print)), 
    std::make_unique<CommandASTExpressionNode>(std::move(multiply))
);
auto statement = arx::CommandASTStatementNode::make_expression(std::move(function));

kernel << statement;

// Output: 60
```

A practical usage of the full control is to get AST from the parser for other purposes.

You can pass your own interpreters to the parser 
as long as they implemented `operator<<(const CommandASTStatementNode&)`:

```cpp
struct MyInterpreter {
    auto operator<<(const arx::CommandASTStatementNode& statement) -> MyInterpreter& {
        // ...
    }
};

MyInterpreter interpreter;
arx::CommandParser parser{ interpreter };
arx::CommandLexer lexer{ parser };

// ...
```

## Semantics

### Types

#### Empty

Empty has two states: positive empty `()/(+)` and negative empty `(-)`.

#### Number

Stored in 32-bits `float`.

#### String

Stored in `std::string`.

#### List

A collection of values.

#### Accessor

A special type of which the exact behavior is defined by the C++ code. The standalone analyzer doesn't provide any accessor.

#### Function

A defined behavior that takes **one** argument and returns **one** value.

A called function will create a new scope, 
all identifiers and protections defined in the scope will be destroyed after the function returns.

#### Macro

A defined behavior that takes **one** argument and returns **one** value.

A called macro will **not** create a new scope,
any identifiers and protections defined in the scope will **not** be destroyed after the macro returns.

### Static Expression(*)

A static expression of a function or a macro is an expression which is evaluated at the time the function is generated.

All static expressions in a function body share the same scope 
which is added on top of the scope where the function is generated when generating,
and then stored in the function.

Static expressions don't have their own body(where arguments and return values are stored when calling a function/macro),
but they are using the body where the function is generated.
So fetching arguments will fetch the arguments of the generator.

```
generate_print = {
    < { print \>; }; // The `\>` fetches the argument of `generate_print`, instead of the defind function.
};

p = generate_print "Hello World!";
p(); // Output: Hello World!
```

And you can specify the identifiers defined in a function/macro's static expressions using `.`:

```
s = {
    \ name = "S";
    \ level = 19;
    print("name: " + name + ", level: " + level);
};

s(); // Output: name: S, level: 19

s.name = "Sakura";
s.level = 3;

s(); // Output: name: Sakura, level: 3
```

### Operations

All operation rules(`value` means any value; if a situation satisfies multiple rules below, the first one applies):

#### Positive

```
+ value = value
```

#### Negative

```
- (+) = (-)
- (-) = (+)
- number = -number
```

#### Addition

```
(+)/(-) + value = value
value + (+)/(-) = value
1 + 2 = 3
(a, b, c, ...) + (x, y, z, ...) = (a, b, c, ..., x, y, z, ...)
(a, b, c, ...) + value = (a, b, c, ..., value)
"..." + value = "...[to_string(value)]" // "Greating " + 2 + " You!" = "Greating 2 You!"
value + "..." = "[to_string(value)]..."
```

#### Subtraction

```
(+)/(-) - value = -value
value - (+)/(-) = value
1 - 2 = -1
```

#### Multiplication

```
(+) * (-) = (-)
(-) * (+) = (-)
(+) * (+) = (+)
(-) * (-) = (+)
2 * 3 = 6
(+)/(-) * number = +/- number;
number * (+)/(-) = +/- number;
```

#### Division

```
7 / 2 = 3.5
```

#### Modulo

```
number_1 % number_2 = fmod(number_1, number_2)
7 % 2 = 1
-2 % 3 = -2
```

#### Power

```
2 ^ 3 = 8
```

#### Comparison

```
(+) == (+) = (+)
(-) == (-) = (+)
(+) == (-) = (-)
(-) == (+) = (-)
(+) > (+) = (-)
(-) > (-) = (-)
(+) > (-) = (+)
(-) > (+) = (-)
(+) < (+) = (-)
(-) < (-) = (-)
(+) < (-) = (-)
(-) < (+) = (+)
1 == 1 = (+)
2 == 3 = (-)
3 > 2 = (+)
2 > 3 = (-)
2 < 3 = (+)
3 < 2 = (-)
"abc" == "abc" = (+) // lexicographical order
"abc" == "def" = (-)
"abc" > "def" = (-)
"abc" < "def" = (+)
print == print = (+)
print == exit = (-)

\``` (*)
get_f = { < { ... }; };
a = get_f();
b = get_f();
print(a == b); // Output: (-) // !!!!!

f = { ... };
a = f;
b = f;
print(a == b) // Output: (+)

// A function without static expresssions equals to itself, but not to another function generated by the same code.

f = { \ value = 0; };
a = f;
b = f;
print(a == b); // Output: (+)
b.value = 1;
print(a == b); // Output: (-) 
a.value = 1;
print(a == b); // Output: (+)

// Functions with static expressions are equal 
// if they branch from the same source
// and their static expressions are all equal.

f = { ... };
m = @f;
print(f == m); // Output: (-) // A function is not equal to its macro.

f = { ... };
g = f;
m = @f;
n = @g;
print(m == n); // Output: (+) // Two macros are equal if the functions they are generated from are equal.
\```


!(-) = (+)
!value = (-)

value_1 != value_2 = !(value_1 == value_2)
value_1 >= value_2 = (value_1 > value_2) or (value_1 == value_2)
value_1 <= value_2 = (value_1 < value_2) or (value_1 == value_2)
```

### Undefined Operation

```
undefined_operation = (-)

-"Hello" = (-)
print + exit = (-)
...
```

### Calling

A `calling` consists of a `callable` and an `argument`.

#### Function Calling

If the callable is of type `function`, then the calling is a `function calling`.

By calling a function, a new scope is created, 
any identifiers and protections defined in the scope
will be destroyed after the function returns.

#### Macro Calling

If the callable is of type `macro`, then the calling is a `macro calling`.

Macro calling is similar to function calling, 
except that it doesn't create a new scope.

```
f = { show := { print "Hello!"; }; };
f();
show(); // Error: Empty is not callable..
g = @f;
g();
// You can also directly write: @f();
show(); // Hello!
```

#### List Calling

List calling returns the element of the list at the given index.

```
l = 1, 2, 3;
print(l 0); // Output: 1
print(l 1); // Output: 2
print(l 2); // Output: 3
print(l 3); // Error: Index out of range.
print(l(-1)); // Output: 3
print(l(-2)); // Output: 2
print(l(-3)); // Output: 1
print(l(-4)); // Error: Index out of range.
l(2) = 4;
print l; 
// Output: 
// 1
// 2
// 4
```

### Accessing

Accessing is a special semantics which gives values additional meanings.

#### Accessing an Accessor

The exact behavior of accessing an accessor is defined by C++ code.

#### Accessing a String

Accessing the string means the identifier with the name that is the same as the string.

Accessing a string can break the rules of identifiers.

This feature can be used for passing references and meta programming.

```
@"a" = 20;
print a; // Output: 20
@"&Illegal*Identifier%" = 30;
print &Illegal*Identifier%; // Error.
print @"&Illegal*Identifier%"; // Output: 30

make_print = @{ x = >; @("print_" + x) = print; };
make_print "er";
print_er "Hello!"; // Output: Hello!
```

#### Accessing a Function

Accessing a function returns a marco.

### Expression Evaluation

Based on the position of an expression, it's evaluated to:

1. Result Value
2. Assignable
3. Identifier

#### Result Value

All expressions can be evaluated to a result value.

#### Assignable

The following expressions can be evaluated to an assignable:


1. identifier, always.
2. parentheses, if the inner expression can be evaluated to an assignable.
3. list, if all it's elements can be evaluated to an assignable. (*) ##### Unimplemented Feature: unpacking ##### 
4. calling, if the callable can be evaluated to an assignable list.
5. condition, if the selected branch can be evaluated to an assignable.
6. protection, if the protected expression can be evaluated to an assignable.
7. accessing, if the accessed expression's result value is a string.


#### Identifier

The following expressions can be evaluated to an identifier:


1. identifier, always.
2. parentheses, if the inner expression can be evaluated to an identifier.
3. condition, if the selected branch can be evaluated to an identifier.
4. accessing, if the accessed expression's result value is a string.


Note that a protection **cannot** be evaluated to an identifier
despite that the expression it protects must be evaluated to an identifier(see [Syntex: Expressions: Protect](#protect)).
The reason is, when evaluating an identifier,
we only care about the "name",
yet the exact value is still uncertain.
But a protection needs to know what exactly it's protecting.

## Syntax

### Comment

A comment starts with `//`, ends with the end of line.

```
// This is a comment.
```

### Expressions

An expression is a piece of code that 
1. does something **or not** and 
2. returns a value **for sure**.

#### Precedence

```
left_precedence :
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

right_precedence :
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

'% is `Loop`
'< is `Return`
'+ is `Positive`
'- is `Negative`
f is `Calling` 
// `f` for function, but it's not necessarily a function. 
// I'm not using `c` because `c`'s meaning is more ambiguous than `f`.
```

#### Assignment

`target = expression`

`target` must be evaluated to an `assignable`. See [Semantices: Expression Evaluation](#expression-evaluation).

```
a = 5;
b = a;
c = a + b + 5;
myprint = print;
d = ();
```

#### Local Assignment

`target := expression`

`target` must be evaluated to an `identifier`. See [Semantices: Expression Evaluation](#expression-evaluation).

Different from common assignment, 
local assignment will not affect the outer scopes.

It doesn't care if the identifier is already defined in the outer scopes.
An identifier defined by local assignment is always a different one 
if there's already one with the same name in the outer scopes.

It's suggested to always use local assignments in the defination of function bodies
that can be called somewhere where you know little about the callers.

```
a = 5;
b = 2;
{
    a = 6;
    b := 3;
    print a; // 6
    print b; // 3
    {
        print a; // 6
        print b; // 3
    }()
}();
print a; // 6
print b; // 2
```

#### Delete

`# target`

`target` must be evaluated to an `identifier`. See [Semantices: Expression Evaluation](#expression-evaluation).

```
#a;
#myprint;
#@"b";
```

#### Protect

`[target]`

`target` must be evaluated to an `identifier`. See [Semantices: Expression Evaluation](#expression-evaluation).

```
a = 5;
a = 10; // Reassign is ok without protection.
[a]; // Protect `a`.
a = 20; // Error. Cannot be reassigned once it's protected.
#a; // Error. Cannot be deleted once it's protected.
```

Waiting for the protection going out of scope is the only way to remove a protection.
So don't protect identifiers in the global scope unless you really know what you are doing.

Some Identifiers defined in built-in libraries are protected by default 
like `print` and `exit` in the global scope. 
Those identifier are impossible to be reassigned or deleted in command. 
But the application can do anything through command runtime :).

Local assignment will not be affected by protection in outer scopes, 
but you still can't locally assign a protected identifier if it's protected in the current scope,
even though the identifier is defined in the outer scopes.

```
a = 5;
{
	[a];
	a = 10; // Error. Cannot be reassigned once it's protected.
	{
        a = 15; // Error. `a` is protected in the outer scope.
		a := 15; // Ok. `a` as a new identifier is not protected in the current scope.
	}();
    a := 20; // Error. Cannot be locally assigned once it's protected.
}();
a = 25; // Ok. Protection is already removed.
```

#### Number Literal

```
3;
50;
-5;
3.14;
2.718281828459045;
```

#### String Literal

A string literal is a sequence of characters surrounded by double quotes.

```
"Hello, world!";
"Hello, \"world\"!";
```

##### Escape Sequences

To bo done.

#### Identifier

A legal identifier starts with a letter or `_`, followed by letters, digits or `_`.

```
a;
b;
print;
exit;
a_0;
a_1;
```

#### List

`expression_0, expression_1, ..., expression_n,`

A list expression is a sequence of expressions separated by comma.
The comma behind the last expression is optional.

```
a = 1, 2, 3;
b = "Greeting", 2, "the world",;
print (1, 2, 3, "one", "two", "three",);
```

It's recommended to use parentheses to wrap a list expression to avoid confusion.

#### Function Body

`{ statement_0; statement_1; ... }`

A function body returns a **function** which is a value.

A function body itself is **not** excuted, 
a **function call** is excuted.

```
add = { // Although assign it to a identifier is more common.
    a = >;
    b = >;
    < a + b;
};
print(add(3, 5));

{ < > + >; }(2, 3); // return 5.
// Since a function body returns a function, it can be called directly.
```

#### Static Expression(*)

`\ expression`

See: [Semantics: Static Expression](#static-expression)]

#### Operation

```
a + b;
a + b * (c - d);
() + 20; // 20
() - 20; // -20
5 * (); // 5
() + (); // ()
2 ^ 3; // 8
```

#### Calling

`callable_experssion argument_expression`

See also: [Semantics:Calling](#calling)

Whenever two expressions come together, 
the first one will be treated as a `callable`, 
and the second one will be treated as an `argument`.

```
print "Hello, world!";
```

Callable in Arxemand must have exact one argument.

For callables intended to have no argument, 
they must be followed by an empty,
otherwise they will be treated as just experssions which return values of those callables.

```
exit; // Nothing happens.
quit = exit; // `exit` as an expression returns a function value, which is assigned to `quit`.
quit(); // a `()`, or any expressions that returns an empty, can't be omitted.
```

Also Arxemand doesn't support multiple arguments by it's syntax,
but you can pass a list as an argument.

```
print (1, "Hello", 2, "World")
```

When passing a list, the parentheses around it can't be omitted:

```
print 1, "Hello", 2, "World";
// Which is equivalent to:
(print 1), "Hello", 2, "World";
// Instead of:
print (1, "Hello", 2, "World");
```

But there's a trick to pass multiple arguments concisely:
for example, 
a function taking the first argument returns a function taking the second argument, 
and so on,
until the last function, which performs the actual operation.

```
teleport x y z;
// Which is equivalent to:
a = teleport x;
b = a y;
b z; // actual function call.
```

Such functions are called **curried functions**.

`print` doesn't support this trick as it can take any number of arguments, 
it's impossible to know when the last argument is passed.


#### Return

`< expression`

`<< experssion`

`<<< expression`

`...`

```
< a;
< a + 5;
<; // Implicitly return ().
<(); // Explicitly return ().
<<; // Double return. Return two bodies with value ().
<<<<< 20; // Return five bodies with the value 20.
```

#### Argument Iteractor

`>`

`>>`

`>>>`

`...`

Each time a `>` appears without any experssion on it's left side, 
it has the value of the next element if the argument is a list,
or has the value of the argument itself if the argument is not a list.

When there's no more element, 
it will end the body of the arguments and return an empty, 
as if it's a `<()`;

```
>; // By doing this, an argument is ignored.
>(); // If an argument is a function, you can do this.
a = >;
```

Note that a `>` is only treated as an argument iterator 
when there's no expression on it's left side.
In case there is an expression on it's left side, 
it will be treated as a *greater than operator*.

So `print >;` will probably not work as you expected 
because it's equivalent to `(print) > ();` which results to `(-)`.
You should use `print ();` instead.

You can also use `>>` to represent 
the argument iterator of the caller of the current function,
and `>>>` and so on.

Multiple `>`s are slightly different from the single `>` by their syntax, 
as they never turn into *greater than operator*s,
which means they don't care if there's an expression on the left side.

#### Empty Deducing

Despite that the `()` is commonly used, 
Arxetipo doesn't have a particular symbol for empty expression.
Instead, it deduces empty from the context.

The rule is: 
whenever an expression is expected, 
but there's no expression, 
it will be assumed as an empty.

So, the truth behind `()` is, 
a pair of parentheses should contains one and only one expression.
But when there's nothing there, 
it's deduced that the expression is an empty.

Therefore, an empty can also be written as `(+)`. 
This time, the pair of parentheses does contain an explicit expression, 
which is a "positive operator".
But the positive operation expects an expression on its right side,
which is not provided, 
so it's deduced as an empty.
And based on the operation rules, a positive empty is an empty.

`(+)` explicitly written as this form is called **positive empty**.
It's often used to keep consistency with the **negative empty** `(-)` when doing boolean operations.

Even though we can also write them as `+()` and `-()`, 
in my opinion `(+)` and `(-)` are more elegant.

Other examples of empty deducing:

```
a = ; // Assigning an empty to `a`.
list = 1, , "hello"; // The second element is an empty.
<; // Returning an empty.
```

Note that an empty is only deduced when a must exist expression is omitted.
In case that having an expression or not makes syntax difference, 
no empty will be deduced.

For example, when a `>` has an expression on it's left side, it's a *greater than* operator,
when there's nothing, it's an argument iterator. 
In this case, Arxetipo will not deduce an empty on it's left side.

Another example is function call. 
When an expression doesn't have another expression on its right side,
it's just the expression itself,
and you must explicitly write an empty if you want to pass an empty as an argument.

#### Condition

`condition_expression ? true_branch_expression`

`condition_expression ? true_branch_expression : false_branch_expression`

if `condition_expression` is not `(-)`, 
`true_branch_expression` will be evaluated,
otherwise `false_branch_expression` will be chosen.

```
b == a ? a + 1 : 0;
```

We can combine multiple conditions to achieve `if else` effect:

```
a ? { // `if`
    print "true";
    print a;
}();

a ? { // `if`
    print "true";
    print a;
}() 
: { // `else`
    print "false";
    print 0;
}();

a ? { // `if`
    print "true";
    print a;
}() 
: b ? { // `else if`
    print "false";
    print 0;
}() 
: { // `else`
    print "false again";
    print 1;
}();
```

Again, be careful that the *empties* following the function bodies is necessary, 
otherwise it will return a function as a value, 
and codes in the function body won't be excuted.


```
a = (+);

m = a ? {
    print 5;
} : {
    print 0;
}; // Nothing printed, m is assigned with a function.

m(); // Output: 5
```

But since `,`'s precedence is greater than `:` and `;`,
we can write expressions into lists:

```
a ?
    print "true",
    print a,
;


a ? 
    print "true",
    print a,
:
    print "false",
    print 0,
;


a ?
    print "true",
    print a,
: b ?
    print "false",
    print 0,
:
    print "false again",
    print 1,
;
```

#### Self

`$ argument`

`$$ argument`

`$$$ argument`

`...`

`$` means the function itself.

```
a = 0;
{
    print(a);
    a = a + 1;
    10 - a ? $();
}();
// 0 1 2 3 4 5 6 7 8 9
```

When writing recrusive functions, make sure to use `$` instead of the function name,
as the function can be assigned to different identifiers 
and the orginal name is not always available.

#### Loop

`%` / `% argument`

`%%` / `%% argument`


`%%%` / `%%% argument`

`...`

different from `$`, 
`%` is more like `continue` in many other languages.
`%` makes the current body go back to the beginning, 
and discard the code remain, 
without creating a new scope.
`$` will create a new scope, 
and the codes remain will still be excuted after self call returns.
`@$` won't create a new scope, either. 
But, it will still excute the codes remain.

By not providing any expression, 
the current body retains the current argument 
as well as the position of the argument iterator.

By providing an expression,
argument and argument iterator are reset.

`%` won't delete any identifiers in the current scope.

#### Accessing

`@ experssion`

See [Semantics: Accessing](#accessing).

### Examples

to be done.