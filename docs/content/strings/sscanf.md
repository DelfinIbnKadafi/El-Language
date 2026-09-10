# sscanf()

`sscanf` reads values out of a string according to a format template, the same way C's `sscanf` does. It's a statement — it doesn't return a value; it writes directly into the variables you give it.

## Syntax

```el
sscanf(source, "template", var1, var2, ...);
```

## Example

```el
var str[16] name;
var int age;

sscanf("Hello 5", "%s %d", name, age);

print name;
print age;
// prints "Hello" then 5
```

## Specifiers

Same four as [format()](content/strings/format.md): `%s` (string), `%d` (int), `%f` (float), `%b` (bool). Each specifier fills the next variable, in order.

## Literal Text Must Match

Anything in the template that isn't a specifier is literal text that must appear in the source at that exact point — this is what makes `sscanf` different from [`input`'s scan sugar](content/console/input.md) or [`split()`](content/strings/split.md), both of which just split on whitespace.

```el
var str[16] name;

sscanf("Hello World", "Hello %s", name);
print name;
// prints "World" -- "Hello " in the template is matched and skipped,
// %s then reads the next word

sscanf("Goodbye World", "Hello %s", name);
print name;
// prints NONE -- the source doesn't start with "Hello ", so the match fails
```

Whitespace in the template matches any amount of whitespace in the source (including none), the same way C's `sscanf` behaves.

## Behavior

- The template must be a literal string written directly in the call — its specifiers and the number of variables given are checked at compile time.
- The source can be any string expression: a literal, a variable, an indexed array element, or a function/builtin call.
- Every destination must be an existing, plain (non-array) variable, and its type must match its specifier.
- The moment something fails to match — literal text that doesn't line up, or a specifier with no valid value left to read — that variable and every variable after it in the call become [`NONE`](content/none-value/none.md), mirroring how C's `sscanf` stops at the first failed conversion.

```el
var int a;
var int b;

sscanf("10", "%d,%d", a, b);
// a is 10 (the first %d succeeds); b is NONE, because after reading "10"
// there's no "," left in the source to match against the template
```

## Related

- [split()](content/strings/split.md)
- [format()](content/strings/format.md)
- [input](content/console/input.md)
