# While Loop

## Syntax

```el
while(condition) =
  // code
```

## Example

```el
var int i = 0;

while(i < 8) =
  print i;
  i++;
```

## Behavior

- The condition follows the same rules as `if` conditions, including boolean shorthand and the `or`/`and`/`not` operators. See [If / Else](content/control-flow/if-else.md) and [Logical Operators](content/operators/logical-operators.md).
- A variable declared inside the loop's body is local to that block, and is re-created on every iteration. See [Scope: Global & Local](content/variables/scope.md).

## Related

- [For Loop](content/control-flow/for-loop.md)
- [Comparison Operators](content/operators/comparison-operators.md)
- [Logical Operators](content/operators/logical-operators.md)
