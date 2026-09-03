# For Loop

## Syntax

```el
for(initial_value; condition; increment/decrement) =
  // code
```

## Example

```el
for(var int i = 0; i <= 10; i++) =
  print i;
```

## Behavior

- The loop counter declared in the initializer (e.g. `var int i = 0`) is local to the loop's block. See [Scope: Global & Local](content/variables/scope.md).
- Because the counter is local, the same name can be reused in a separate, later `for` loop without conflict:

```el
for(var int i = 0; i < 5; i++) =
  print i;

for(var int i = 0; i < 3; i++) =
  print i;
// works fine -- each "i" is its own variable
```

- The condition follows the same rules as `if`/`while` conditions — see [Comparison Operators](content/operators/comparison-operators.md) and [Logical Operators](content/operators/logical-operators.md).

## Related

- [While Loop](content/control-flow/while-loop.md)
- [Scope: Global & Local](content/variables/scope.md)
- [Assignment Operators](content/operators/assignment-operators.md)
