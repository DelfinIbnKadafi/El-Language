# print

`print` writes a value to the console, followed by a new line.

## Syntax

```el
print "Text";
print variable;
print expression;
```

## Examples

Printing a literal string:

```el
print "Hello World!";
```

Printing a variable:

```el
var int age = 21;

print age;
```

Printing an expression:

```el
print 1 + 10;
```

## Printing With a Format String

`print` also accepts a format string followed by a comma-separated list of arguments, without needing a separate `format(...)` call:

```el
var str[16] nama = "Delfin";

print "Hello, %s", nama;
```

This is shorthand for `print format("Hello, %s", nama);` — see [format()](content/strings/format.md) for the full list of specifiers (`%s`, `%d`, `%f`, `%b`) and how arguments are matched to them. This shorthand is only available directly in `print` and `input`; anywhere else, build the string with `format(...)` first.

## Output Formatting

| Type | Output |
|---|---|
| `int` | Plain integer, e.g. `9` |
| `float` | Without trailing zeros, e.g. `3.5` |
| `bool` | `true` or `false` |
| `str` | The string's text |
| any type set to `NONE` | `NONE` |

## Notes

- `print` can also print the direct result of a function call, an `input` prompt, or a `format(...)` call, e.g. `print factorial(5);`, `print input "Name: ";`, or `print format("Score: %d", score);`.
- Printing an array variable without an index is a compile-time error — index into the specific element you want, e.g. `print numbers[0];`. See [Arrays](content/arrays/arrays.md).

## Related

- [input](content/console/input.md)
- [format()](content/strings/format.md)
- [The NONE Value](content/none-value/none.md)
- [Arrays](content/arrays/arrays.md)
