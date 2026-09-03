# String Comparison

String values can be compared using `==` and `!=`.

## Syntax

```el
if(string == "Hello") =
  // ...

if(string == string2) =
  // ...
```

## Example

```el
var str[16] answer = input "Continue? (y/n): ";

if(answer == "y") =
  print "Continuing...";
else =
  print "Cancelled.";
```

## Behavior

- Strings can only be compared with `==` and `!=`. Using `<`, `>`, `<=`, or `>=` on a string is a compile-time error.
- A string variable can also be compared against [`NONE`](content/none-value/none.md) with `==` or `!=`.

```el
if(answer == NONE) =
  print "No input yet";
```

## Related

- [str](content/data-types/str.md)
- [Comparison Operators](content/operators/comparison-operators.md)
- [The NONE Value](content/none-value/none.md)
