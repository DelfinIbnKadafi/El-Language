# split()

`split` breaks a string into pieces on whitespace, writing each piece into the variable you give it, in order — similar to Python's `str.split()`. It's a statement — it doesn't return a value.

## Syntax

```el
split(source, var1, var2, ...);
```

## Example

```el
var str a = "Budi 28 78 34";

var str b;
var str c;

split(a, b, c);

print b;
print c;
// prints "Budi" then "28"
```

Only as many pieces as there are variables get assigned — `"78"` and `"34"` are read from `a` but discarded, since no variable was given for them.

## Behavior

- Every destination must be an existing, plain (non-array) `str` variable.
- Splitting is always on whitespace — there's no format template, unlike [`sscanf()`](content/strings/sscanf.md).
- If `source` has fewer pieces than there are variables, the remaining variables are left [`NONE`](content/none-value/none.md).

```el
var str a = "one two";
var str x;
var str y;
var str z;

split(a, x, y, z);
// x is "one", y is "two", z is NONE
```

## Related

- [sscanf()](content/strings/sscanf.md)
- [str](content/data-types/str.md)
- [input](content/console/input.md)
