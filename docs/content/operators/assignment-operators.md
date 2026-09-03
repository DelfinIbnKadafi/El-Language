# Assignment Operators

| Operator | Meaning | Supported types |
|---|---|---|
| `=` | Assign a value | `int`, `float`, `bool`, `str` |
| `+=` | Add and assign | `int`, `float` |
| `-=` | Subtract and assign | `int`, `float` |
| `++` | Increment by 1 | `int` |
| `--` | Decrement by 1 | `int` |

## Examples

```el
var int age = 10;

age = 15;
age++;
age--;
age += 10;
age -= 10;
```

## Behavior

- `++` and `--` only work on `int` variables.
- `+=` and `-=` work on `int` and `float` variables, but not on `bool` or `str`.
- Assigning a `float` value directly to an `int` variable with `=` or `+=`/`-=` is a compile-time error.
- These operators also work on individual array elements, e.g. `numbers[0]++;`, as long as the element is indexed.

## Related

- [Assignment & Updates](content/variables/assignment-and-updates.md)
- [Arithmetic](content/math/arithmetic.md)
- [Arrays](content/arrays/arrays.md)
