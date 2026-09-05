# input

`input` reads a line of text from the console. Like Python's `input()`, it always produces a string value.

## Syntax

```el
input;
input "Prompt text";
```

## Bare Input (No Prompt)

Writing `input` without a prompt reads a line without printing anything first.

```el
input;
```

## Input With a Prompt

When `input` is given a prompt, the prompt is printed first (without a trailing new line), then a line is read.

```el
input "Hello : ";
```

## Assigning Input to a Variable

```el
var str[16] name;

name = input;
```

```el
var str[16] answer = input "Continue? (y/n): ";
```

## Using Input Directly as an Argument

```el
say_hi_to(input "Name : ");
```

## Reading Into Multiple Variables With a Format String

`input` also accepts a format string followed by a comma-separated list of destination variables. Unlike a regular prompt, the format string here isn't displayed — it's a template describing what to read.

```el
var str[16] name;
var int age;

input "%s %d", name, age;

print name;
print age;
```

This reads one line, splits it by whitespace, and assigns one piece per specifier (in order) into the matching variable, converting it according to that specifier — `%s`, `%d`, `%f`, and `%b` work the same as in [format()](content/strings/format.md). If the line has fewer pieces than there are specifiers, the remaining variables are left [`NONE`](content/none-value/none.md).

This form is a statement on its own — it can't be assigned to a variable or used as a function argument, unlike a regular `input "prompt";`. Every destination must be an existing, plain (non-array) variable, and its type must match its specifier; both the number of specifiers and each variable's type are checked at compile time.

## Related

- [print](content/console/print.md)
- [format()](content/strings/format.md)
- [str](content/data-types/str.md)
- [String Comparison](content/strings/string-comparison.md)
