# Initial Values

An array variable can be given a default value at declaration time.

## Broadcast: Same Value for Every Index

Assigning a single value fills every index of the array with that value.

```el
var int test[3] = 10;
// test is {10, 10, 10}
```

## List: Different Value Per Index

Use `{}` to give each index its own value.

```el
var int test[3] = {1, 2, 3};
```

The number of values inside `{}` must match the declared array size.

## Example

```el
var int numbers[5] = {10, 20, 30, 40, 50};
var str names[3] = {"Alice", "Bob", "Charlie"};
var bool flags[3] = {true, false, true};

print numbers[2];
// prints 30
```

## Behavior

- An array declared without any initial value has every index set to [`NONE`](content/none-value/none.md).
- Sized strings can also be used as array elements, e.g. `var str[16] names[3] = {"Alice", "Bob", "Charlie"};`. See [Sized Strings](content/strings/sized-strings.md).

## Related

- [Arrays](content/arrays/arrays.md)
- [Copying Values](content/arrays/copying-values.md)
- [Array Parameters](content/functions/array-parameters.md)
