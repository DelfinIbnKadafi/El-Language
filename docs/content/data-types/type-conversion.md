# Type Conversion

`int()`, `float()`, and `str()` convert a value from one data type to another.

## int()

Converts a `str`, `float`, or `bool` value to `int`.

```el
var str[8] age_text = "42";
var int age = int(age_text);

print age;
// prints 42
```

- From `str`: the text must be a valid number, or the result is [`NONE`](content/none-value/none.md).
- From `float`: the decimal part is truncated (dropped, not rounded).
- From `bool`: `true` becomes `1`, `false` becomes `0`.
- From `NONE`: the result is `NONE`.

```el
print int("3.99");
// prints 3 (parsed, then truncated)

print int("abc");
// prints NONE (not a valid number)
```

## float()

Converts a `str`, `int`, or `bool` value to `float`. Behaves the same as `int()`, except the decimal part is kept.

```el
var str[8] price_text = "19.99";
var float price = float(price_text);

print price;
// prints 19.99

print float("abc");
// prints NONE
```

## str()

Converts an `int`, `float`, or `bool` value to `str`, using the same formatting `print` uses (floats without trailing zeros, bools as `true`/`false`). A `str` argument is returned unchanged. [`NONE`](content/none-value/none.md) becomes the text `"NONE"`.

```el
print str(42);
// prints "42"

print str(3.14);
// prints "3.14"

print str(true);
// prints "true"
```

## Behavior

- All three accept any single value — a literal, a variable, an indexed array element, or another function/builtin call.
- `int()`/`float()`/`str()` can be used anywhere a value of their result type is expected: assigned to a variable, passed as a function argument, used in `print`, `format()`, or `return`.

```el
var str[16] combined = format("Value: %s", str(42));

print combined;
// prints "Value: 42"
```

## Related

- [int](content/data-types/int.md)
- [float](content/data-types/float.md)
- [str](content/data-types/str.md)
- [format()](content/strings/format.md)
