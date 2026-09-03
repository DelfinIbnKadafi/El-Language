# Sized Strings

Every `str` variable has a maximum number of characters it can hold. EL calls this a sized string.

## Declaration

```el
var str[size] name;
```

Example:

```el
var str[5] username;
```

## Default Size

If you don't specify a size, the default maximum is 255 characters.

```el
var str name;
// same as: var str[255] name;
```

## Truncation

If an assigned string exceeds the declared size, EL automatically truncates the value — it does not crash or raise an error.

```el
var str[5] text;

text = "HelloWorld";
print text;
// prints "Hello"
```

Extra characters are simply removed, and the program continues running normally.

## Related

- [str](content/data-types/str.md)
- [String Comparison](content/strings/string-comparison.md)
- [input](content/console/input.md)
