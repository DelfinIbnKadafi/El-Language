# str

`str` stores text.

## Syntax

```el
var str name;
var str name = "value";

// with an explicit fixed size
var str[size] name;
var str[size] name = "value";
```

## Example

```el
var str[32] name = "Delfin";

print name;
```

## Behavior

- Every `str` variable has a maximum length in characters. If you don't specify one with `str[size]`, the default maximum is 255 characters. See [Sized Strings](content/strings/sized-strings.md) for the full behavior, including truncation.
- `str` values can only be compared with `==` and `!=`. See [String Comparison](content/strings/string-comparison.md).
- `str` does not support concatenation with `+`.
- A `str` variable declared without an initial value holds [`NONE`](content/none-value/none.md) until assigned.
- `++`, `--`, `+=`, and `-=` are **not** supported on `str` variables.
- `str` supports array declarations, e.g. `var str names[5];` or a sized version, `var str[16] names[5];`. See [Arrays](content/arrays/arrays.md).

## Related

- [Sized Strings](content/strings/sized-strings.md)
- [String Comparison](content/strings/string-comparison.md)
- [input](content/console/input.md)
