# Logical Operators

EL provides three logical operators for combining or inverting conditions: `or`, `and`, and `not`.

## or / and

```el
if(condition or condition) =
  // ...

if(condition and condition) =
  // ...
```

## not

`not` reverses a condition. It can also be chained together with `and`/`or`.

```el
if(not condition) =
  // ...

while(i < 10 and not found) =
  // ...
```

Using `not` on a boolean variable is equivalent to comparing it against `false`:

```el
if(not life) =
  // equivalent to: if(life == false) =
```

## Example

```el
var int i = 0;
var bool found = false;

while(i < 10 and not found) =
  if(i == 5) =
    found = true;
  i++;
```

## Related

- [Comparison Operators](content/operators/comparison-operators.md)
- [If / Else](content/control-flow/if-else.md)
- [While Loop](content/control-flow/while-loop.md)
