# Comparison Operators

| Operator | Meaning |
|---|---|
| `==` | Equal to |
| `!=` | Not equal to |
| `>` | Greater than |
| `<` | Less than |
| `>=` | Greater than or equal to |
| `<=` | Less than or equal to |

## Examples

```el
if(age >= 18) =
  print "Allowed";

if(name == "Delfin") =
  print "Hello, Delfin!";
```

## Behavior

- `int`, `float`, and `bool` values support all six comparison operators.
- `str` values only support `==` and `!=`. Using `<`, `>`, `<=`, or `>=` on a string is a compile-time error. See [String Comparison](content/strings/string-comparison.md).
- [`NONE`](content/none-value/none.md) can only be compared using `==` or `!=`, and only directly against a variable.
- A `bool` variable can be used directly as a condition, without writing `== true`:

```el
if(life) =
  // equivalent to: if(life == true) =
```

## Related

- [Logical Operators](content/operators/logical-operators.md)
- [If / Else](content/control-flow/if-else.md)
- [The NONE Value](content/none-value/none.md)
