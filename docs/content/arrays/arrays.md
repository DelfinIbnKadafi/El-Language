# Arrays

EL supports array variables for every data type: `int`, `float`, `bool`, and `str`.

## Declaration

```el
var data_type name[size];
```

Example:

```el
var int numbers[5];
var str names[10];
var bool status[3];
```

## Indexing

Array indexing is zero-based: it starts at `0` and ends at `size - 1`.

```el
numbers[0] = 10;
numbers[4] = 50;
```

## Usage

An array variable must always be accessed using an index — reading or writing the array as a whole (without an index) is a compile-time error.

```el
print numbers[0];
```

Invalid example:

```el
print numbers;
// error: Variable 'numbers' is an array and must be indexed
```

## Array Size

Array size has no fixed upper limit beyond available memory — declare it as large as you need.

## Related

- [Initial Values](content/arrays/initial-values.md)
- [Copying Values](content/arrays/copying-values.md)
- [Sized Strings](content/strings/sized-strings.md)
- [Array Parameters](content/functions/array-parameters.md)
