# format()

`format` builds a string by substituting values into a template, and returns the result as a string.

## Syntax

```el
format(template, arg1, arg2, ...)
```

## Example

```el
var str[16] nama = "Delfin";
var str[64] hasil = format("Hello, %s", nama);

print hasil;
// prints "Hello, Delfin"
```

`format(...)` can be used anywhere a string value is expected — assigned to a variable, passed as a function argument, used directly in `print`, or returned from a function — the same way a string-returning function call can be.

```el
print format("Hello, %s", nama);

function greet(var str[32] name) =
  return format("Hi, %s!", name);
```

## Specifiers

| Specifier | Expects |
|---|---|
| `%s` | A `str` value |
| `%d` | An `int` or `float` value, formatted as a whole number |
| `%f` | An `int` or `float` value, formatted with its decimal part |
| `%b` | A `bool` value, formatted as `true`/`false` |
| `%%` | A literal `%` character (not a substitution) |

Each specifier consumes the next argument, in the order both appear — the first specifier in the template pairs with the first argument, the second specifier with the second argument, and so on.

```el
var int age = 25;
var float score = 3.14159;
var bool active = true;

print format("Age: %d, Score: %f, Active: %b", age, score, active);
// prints "Age: 25, Score: 3.14159, Active: true"
```

## Behavior

- The template argument can be a string literal, a `str` variable, another `format(...)` call, or any other string-producing expression.
- When the template is written as a literal directly in the `format(...)` call, EL checks the number and types of arguments against its specifiers at compile time, the same way a function call's arguments are checked.
- When the template comes from a variable (its text isn't known until the program runs), that same checking happens at runtime instead.
- An `int`/`float`/`bool` argument that's [`NONE`](content/none-value/none.md) formats as the text `NONE`, regardless of specifier.
- If the template itself is `NONE`, the whole `format(...)` call returns `"NONE"`.
- `format(...)` calls can be nested: `format("%s", format("%d", x))`.

## Common Mistakes

A mismatched argument type is a compile-time error when the template is a literal:

```el
print format("Value: %d", "hello");
// error: format argument 1 does not match its %d specifier
```

A mismatched argument count is also a compile-time error:

```el
print format("Hello, %s", "a", "b");
// error: format string uses 1 argument(s) but 2 were given
```

A bare comparison or logical expression (e.g. `a == b`, `not flag`) isn't accepted directly as an argument — assign it to a `bool` variable first:

```el
var bool ok = a == b;
print format("Equal: %b", ok);
```

## Related

- [print](content/console/print.md)
- [input](content/console/input.md)
- [Sized Strings](content/strings/sized-strings.md)
