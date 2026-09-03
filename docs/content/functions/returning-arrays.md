# Returning Arrays

A function can return an array, as long as the array's size and element type are declared consistently.

## Syntax

```el
function function_name() =
  var data_type name[size] = ...;
  return name;
```

## Example

```el
function make_numbers() =
  var int arr[3] = {1, 2, 3};
  return arr;

var int result[3] = make_numbers();

print result[0];
print result[1];
print result[2];
```

## Behavior

- The variable receiving the returned array must be declared with the exact same size as the array returned by the function.
- Every code path in a function that returns an array must return an array — a function can't sometimes return an array and sometimes return a bare `return;` or a non-array value.
- Sized-string arrays can also be returned this way.

## Related

- [Arrays](content/arrays/arrays.md)
- [Array Parameters](content/functions/array-parameters.md)
- [Return](content/functions/return.md)
