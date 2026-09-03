# bool

`bool` stores a `true` or `false` value.

## Syntax

```el
var bool name;
var bool name = true;
var bool name = false;
```

## Example

```el
var bool isOnline = true;

print isOnline;
// prints "true"
```

## Behavior

- Output is printed as the literal text `true` or `false`.
- A `bool` variable can be used directly as a condition in `if`/`while`, without writing `== true` explicitly. See [If / Else](content/control-flow/if-else.md).
- A `bool` variable declared without an initial value holds [`NONE`](content/none-value/none.md) until assigned.
- `++`, `--`, `+=`, and `-=` are **not** supported on `bool` variables.
- `bool` supports array declarations, e.g. `var bool flags[5];`. See [Arrays](content/arrays/arrays.md).

## Related

- [If / Else](content/control-flow/if-else.md)
- [Logical Operators](content/operators/logical-operators.md)
- [The NONE Value](content/none-value/none.md)
