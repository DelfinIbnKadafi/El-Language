# String Functions

## len()

Returns the length of a `str`, or the size of an array.

```el
var str[16] name = "Delfin";
print len(name);
// prints 6

var int numbers[5];
print len(numbers);
// prints 5
```

### Behavior

- On an array (of any element type — `int`, `float`, `bool`, or `str`), referenced by name **without** an index, `len()` gives the array's declared size. Since that size is fixed at compile time, this doesn't cost a runtime call at all.
- On any other string value — a literal, a `str` variable, an indexed element of a `str` array, or another function/builtin call — `len()` measures the string's actual current length at runtime.
- `len()` of a [`NONE`](content/none-value/none.md) string is `NONE`.

```el
var str names[3] = {"Alice", "Bob", "Charlie"};

print len(names);
// prints 3 (array size)

print len(names[0]);
// prints 5 (length of "Alice")
```

## upper() / lower()

Return a copy of a `str` with every letter converted to uppercase or lowercase.

```el
print upper("hello");
// prints "HELLO"

print lower("WORLD");
// prints "world"
```

`upper()`/`lower()` of a `NONE` string is `NONE`.

## trim()

Returns a copy of a `str` with leading and trailing whitespace removed.

```el
var str[16] padded = "  spaced  ";
print trim(padded);
// prints "spaced"
```

`trim()` of a `NONE` string is `NONE`.

## Related

- [str](content/data-types/str.md)
- [Sized Strings](content/strings/sized-strings.md)
- [Arrays](content/arrays/arrays.md)
- [format()](content/strings/format.md)
