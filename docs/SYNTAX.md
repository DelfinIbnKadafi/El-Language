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

You can combine multiple conditions using the `or` and `and` operators.

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