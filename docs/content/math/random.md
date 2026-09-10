# random()

`random` returns a random whole number.

## Syntax

```el
random(min, max)
```

## Example

```el
var int roll = random(1, 6);
print roll;
// prints a random int from 1 to 6
```

## Behavior

- Returns a random `int` in the inclusive range `[min, max]` — both `min` and `max` are possible results.
- Both arguments must be `int`; a `float` argument is a compile-time error.
- If `min` is greater than `max`, it's a runtime error.

```el
print random(10, 1);
// error: random(): min is greater than max
```

## Related

- [int](content/data-types/int.md)
- [Math Functions](content/math/math-functions.md)
