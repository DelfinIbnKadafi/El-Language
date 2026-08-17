# Syntax

**Note:** The syntax shown here reflects the latest version of the language and will be updated as new features are added.

## 1. Print

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

## 2. Exit

Use this statement to stop the program execution.

```el
exit;
```

## 3. Variable

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

Use `!` to check for `false`.

```el
if(!life) =
```

This is equivalent to:

```el
if(life == false) =
```

#### Or/And Operator

You can combine multiple conditions using the `or`, `and` and `not` operators.

Example:

```el
if(condition or condition)
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