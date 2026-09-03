# Array Parameters

Functions can receive arrays as parameters. The array size must be explicitly declared in the parameter.

## Syntax

```el
function function_name(var data_type name[size]) =
  // code
```

## Example

```el
function sum(var int arr[3]) =
  return arr[0] + arr[1] + arr[2];

print sum([1, 2, 3]);
```

## Ways to Pass an Array Argument

An array parameter can receive:

A list literal with the exact same size:

```el
sum([1, 2, 3]);
```

A single value, broadcast to every index:

```el
sum(10);
// arr becomes {10, 10, 10}
```

An existing array variable:

```el
var int numbers[3] = {1, 2, 3};

sum(numbers);
```

## Behavior

- Array parameters are passed by value — modifying the parameter inside the function does not modify the original array.
- The list size must match the parameter size exactly. A mismatched list is a compile-time error.
- Sized-string arrays are also supported as parameters:

```el
function print_names(var str[16] labels[3]) =
  print labels[0];
```

## Related

- [Arrays](content/arrays/arrays.md)
- [Parameters](content/functions/parameters.md)
- [Returning Arrays](content/functions/returning-arrays.md)
