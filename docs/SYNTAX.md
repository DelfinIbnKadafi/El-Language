# Syntax

**Note:** The syntax shown here reflects the latest version of the language and will be updated as new features are added.

## Print

```el
print "Text";
```

Example:

```el
print "Hello World!";
```

You can also print variables or expressions.

```el
print variable;
```

```el
print 1 + 10;
```

## Exit

Use this statement to stop the program execution.

```el
exit;
```

## Variable

#### Declaration

```el
var data_type name;
```

Example:

```el
var int age = 10;
var bool did_you_love_me;
```

Supported Data Types:

- int
- str
- bool
- float

#### Usage

Example:

```el
age = 10;
did_you_love_me = false;
age++;
age--;
age += 10;
age -= 10;
```

## Math

```el
10 + 5
(10 + 5) * 5
```

Example:

```el
age = 10 - 1;
```

## If/Else

#### If

Use this syntax to create an `if` statement.

```el
if(condition) =
  // code here
```

The indentation width is flexible. You can use one or more spaces, as long as the block is consistently indented.

Example:

```el
if(Elfaria > 10) =
  // 2-space indentation

if(Elfaria < 10) =
 // 1-space indentation
```

#### Else

Use `else` after an `if` statement. An `else` statement cannot exist without a preceding `if`.

Example:

```el
if(condition) =
  //
else =
  //
```

#### Else if

Unlike `else`, `else if` requires a condition.

Example:

```el
if(condition) =
  //
else if(condition) =
  //
```

#### Multiple If Statement

You can nest multiple `if` statements.

Example:

```el
if(condition) =
  if(condition) =
```

You can also use a boolean variable directly as the condition.

Example:

```el
if(life) =
```

This is equivalent to:

```el
if(life == true) =
```

Use `not` to check for `false`.

```el
if(not life) =
```

This is equivalent to:

```el
if(life == false) =
```

#### Or/And/Not Operator

You can combine multiple conditions using the `or`, `and` and `not` operators.

Example:

```el
if(condition or condition) =
```

```el
if(condition and condition) =
```

`not` reverses a condition, and can be chained together with `and`/`or`.

Example:

```el
if(not condition) =
```

```el
while(i < 10 and not found) =
```

#### One-Line Statement

You can write statements on the same line as the `if`.

Example:

```el
if(condition) = code; code;
```

#### Empty Statement

If an `if` or `else` block is empty, ELVM simply skips its execution.

Example:

```el
if(condition) =
```

## Commentary

Like most programming languages, EL supports comments. Text inside comments is ignored and never executed by ELVM.

Example:

```el
// This is a single-line comment.

/*
This is
a multi-line comment.
*/
```

## Array Variable

EL supports array variables for all supported data types:

- int
- float
- bool
- str

#### Declaration

```el
var data_type name[size];
```

Example:

```el
var int numbers[5];
var str names[10];
var bool status[3];
```

Array indexing starts from 0 and ends at size - 1.

Example:

```
numbers[0] = 10;
numbers[4] = 50;
```

EL uses a zero-based indexing system, similar to Pawn.

Usage

Arrays must always be accessed using an index.

Example:

```el
print numbers[0];
```

Accessing an array variable without an index will produce an error.

Invalid example:

```el
print numbers;
```

ELVM will report a clear error instead of executing the statement.

Sized String

EL supports fixed-size strings using the str[size] syntax.
Default max string size (If you doesn't custom) is 255. You can custom string size unlimited.

Declaration

```el
var str[size] name;
```

Example:

```el
var str[5] username;
```

A sized string can only store characters up to the specified size.

If an assigned string exceeds the limit, EL automatically truncates the value.

Example:

```
var str[5] text;
text = "HelloWorld";
```

The stored value becomes:

"Hello"

Extra characters are removed automatically, and the program continues execution without crashing.

Array size and string size have no fixed upper limit — you can make them as large as your device's memory allows.

#### Copying Values

You can copy the value of one array element into another, as long as they have the same data type.

Example:

```el
var str names[5];
var str backup[5];

backup[0] = names[0];
```

## Initial Value Array

If you wanna create a variable with array index, you also put default/initial value in that variable.

Example :

```el
var int test[3] = 10;
```

The value of all index test variable is 10.

But, if you want put diferent value in array varieble, use {}.

Example :

```el
var int test[3] = {1, 2, 3};
```

## NONE Value

If you create a variable, but you doesn't put default value, the variable value is NONE. And if you print with variable with a NONE value, the output is "NONE".

Example :

```el
var int jj;
// the value of jj is NONE

print jj;

// the output is 'NONE'
```

#### Mathemathic NONE Value

If the type of var is int or float, if you put both in a mathematic program, both has a 0 value.

For Example :

```el
var float bb;

jj = bb + 1;

// bb automatlicy change to 0, so 0 + 1
```

The value NONE can have by all of data type.

#### If Operator With NONE Value

Yo also create if/elseif statement with NONE condition.

Example :

```el
if(jj == NONE) =
```

## Loop

#### For 

```el
for(initial_value; condition; increnment/decrenment) =
  // code
```

Example :

```el
for(var int i = 0; i <= 10; i++) =
  print i;
```

#### While

```el
while(condition) =
  // code
```

## Global and Local Variable

If you create a variable outside of any statement (`if`, `for`, `while`, or `function`), the variable is global, and can be used from anywhere in the file below where it was declared.

If you create a variable inside a statement, it becomes local, and can only be used inside the block where it was declared. Once the block ends, the variable is gone.

Example:

```el
if(true) =
  var int local_number = 5;
  print local_number; // works fine

print local_number; // error, local_number doesn't exist here
```

Because a local variable disappears once its block ends, you can reuse the same name in another block that comes after it (this is very handy for loop counters).

Example:

```el
for(var int i = 0; i < 5; i++) =
  print i;

for(var int i = 0; i < 3; i++) =
  print i;
// this works, the second "i" is a brand new variable
```

Unlike some other languages, EL does **not** support shadowing. If a name is already used by a variable in an outer/enclosing scope, you can't declare a new variable with that same name inside a nested block, even if you wanted to.

Invalid example:

```el
var int score = 10;

if(true) =
  var int score = 20;
  // error, "score" is already declared
```

A function's parameters and any variable declared inside it also follow local scope rules — see the Function section below.

## Function

Use `function` to create a reusable block of code that can be called by name.

```el
function function_name() =
  // code
```

To run it, call it by its name:

```el
function_name();
```

Example:

```el
function say_hello() =
  print "Hello!";

say_hello();
```

#### Parameters

A function can receive values through parameters. Every parameter must be declared with `var`, just like a normal variable, and supports every data type EL has (`int`, `float`, `str`, `bool`), including sized strings.

```el
function function_name(var int number, var float decimal, var str[16] text, var bool flag) =
  // code
```

Example:

```el
function greet(var str[16] name) =
  print name;

greet("Delfin");
```

#### Array Parameters

Functions can receive arrays as parameters. The array size must be explicitly declared in the parameter.

```el
function function_name(var data_type name[size]) =
  // code
```

Example:

```el
function sum(var int arr[3]) =
  return arr[0] + arr[1] + arr[2];

print sum([1, 2, 3]);
// prints
```

An array parameter can receive:

A list literal with the exact same size:

```el
sum([1, 2, 3]);
```

A single value, which is broadcast to every index:

```el
sum(10);
// arr becomes {10, 10, 10}
```

An existing array variable:

```el
var int numbers[3] = {1, 2, 3};

sum(numbers);
```

Array parameters are passed by value, so modifying the parameter does not modify the original array.

The list size must match the parameter size. A mismatched list produces a compile-time error.

Sized-string arrays are also supported:

```el
function print_names(var str[16] labels[3]) =
  print labels[0];
```

#### Return

Use `return` to send a value back to whoever called the function. You don't need to declare a return type anywhere — EL figures it out automatically from whatever value your `return` statements use.

```el
function add(var int a, var int b) =
  return a + b;

print add(3, 4);
// prints 7
```

Because a function call produces a value, you can use it directly anywhere a value is expected, including inside `if` conditions, math expressions, or as another function's argument.

Example:

```el
function is_adult(var int age) =
  return age >= 18;

if(is_adult(20)) =
  print "Allowed";
```

A `return` on its own (with no value) simply exits the function early, without producing anything usable.

```el
function check(var int n) =
  if(n < 0) =
    return;
  print n;
```

#### Return Arrays

A function can return an array.

```el
function function_name() =
  var int result[3] = {10, 20, 30};
  return result;
```

The returned array can be assigned to an array variable:

```el
var int numbers[3] = function_name();
```

It can also be indexed directly:

```el
print function_name()[0];
// prints 10
```

The returned array must have a consistent data type and size across every return statement.

A function returning an array must return an array on every possible execution path. A bare return; is not allowed in such a function.

Example:

```el
function create_numbers() =
  var int result[3] = {1, 2, 3};
  return result;
```

Array return values also support sized strings:

```el
function create_names() =
  var str[16] names[3] = {"Alice", "Bob", "Charlie"};
  return names;

print create_names()[0];
```

#### Recursion

A function is allowed to call itself, directly or through another function. Every call keeps its own separate copy of its parameters and local variables, so recursive calls never interfere with each other.

Example:

```el
function factorial(var int n) =
  if(n <= 1) =
    return 1;
  return n * factorial(n - 1);

print factorial(5);
// prints 120
```

## Input

Same with python, you can request input by using `input` function. Input always give string value.

If you write `input` without parameter/prompt, vm gonna request input without output.

Example :

```el
input;
```

Else if you create input with params, vm gonna print prompt first and input in one line.

Example : 

```el
input "Hello : ";
```

You can assigned input to variable too.

Example :

```el
name = input;
```

You also call input and function, likes

```el
say_hi_to(input "Name : ");
```

## String Compare

You can compare string type into if statement or another statement.

Example :

```el
if(string == "Hello")
// or
if(string == string2)
```
