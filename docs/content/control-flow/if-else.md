# If / Else

## If

```el
if(condition) =
  // code here
```

Indentation width is flexible — one or more spaces are allowed, as long as the block is indented consistently.

```el
if(Elfaria > 10) =
  // 2-space indentation

if(Elfaria < 10) =
 // 1-space indentation
```

## Else

`else` must follow an `if` statement — it cannot exist on its own.

```el
if(condition) =
  // ...
else =
  // ...
```

## Else If

Unlike `else`, `else if` requires its own condition.

```el
if(condition) =
  // ...
else if(condition) =
  // ...
```

## Nested If Statements

```el
if(condition) =
  if(condition) =
    // ...
```

## Boolean Conditions

A `bool` variable can be used directly as a condition:

```el
if(life) =
  // ...
// equivalent to: if(life == true) =
```

Use `not` to check for `false`:

```el
if(not life) =
  // ...
// equivalent to: if(life == false) =
```

See [Logical Operators](content/operators/logical-operators.md) for `or`, `and`, and `not`.

## One-Line Statements

Statements can be written on the same line as the `if`.

```el
if(condition) = code; code;
```

## Empty Blocks

If an `if` or `else` block has no statements, ELVM simply skips its execution.

```el
if(condition) =
```

## Related

- [Comparison Operators](content/operators/comparison-operators.md)
- [Logical Operators](content/operators/logical-operators.md)
- [The NONE Value](content/none-value/none.md)
