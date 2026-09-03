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

## Related

- [print](content/console/print.md)
- [str](content/data-types/str.md)
- [String Comparison](content/strings/string-comparison.md)
