# Dictionary Functions

Functions that work on a whole [dictionary](content/dictionaries/dictionary.md). The first argument is always the dictionary variable itself, written without a key (or an array element, like `users[1]`).

## Syntax

```el
len(dict)                  // number of keys
has_key(dict, "key")       // bool: is the key there?
remove_key(dict, "key")    // bool: removes the key, true if it existed
key_at(dict, position)     // str: the key at a position
```

## len

`len(dict)` gives the number of keys as an `int`.

```el
var dict p = {"name": "Delfin", "age": 30};

print len(p);
// 2
```

`len(dict["key"])` is the length of the text an entry holds, like `len()` of any `str`. For an array of dictionaries, `len(users)` is the size of the array and `len(users[0])` is the number of keys of one dictionary.

## has_key

`has_key(dict, "key")` gives `true` if the dictionary has that key, `false` if not. It never stops the program, so use it before reading a key that may not exist.

```el
var dict p = {"name": "Delfin"};

if(has_key(p, "age")) =
  print p["age"];
else =
  print "no age";
```

Output:

```
no age
```

The result is a `bool`, so it can be kept in a variable: `var bool found = has_key(p, "age");`

## remove_key

`remove_key(dict, "key")` removes the key and its value. It gives `true` if the key was there and `false` if it was not.

```el
var dict p = {"a": 1, "b": 2};

print remove_key(p, "a");
print remove_key(p, "a");
print p;
```

Output:

```
true
false
{"b": 2}
```

It can also be used as a statement when the result is not needed: `remove_key(p, "b");`

## key_at

`key_at(dict, position)` gives the key at a position, counted from 0, in the order the keys were added. Use it with `len()` to go through a dictionary:

```el
var dict p = {"name": "Delfin", "age": 30, "score": 9.5};

var int i = 0;

while(i < len(p)) =
  var str key = key_at(p, i);

  print format("%s = %s", key, str(p[key]));
  i++;
```

Output:

```
name = Delfin
age = 30
score = 9.5
```

A position outside `0` to `len(dict) - 1` stops the program with an error.

`str(p[key])` turns an entry of any type into text, so the loop can print them all with `%s`.

## Common Errors

| Message | Cause |
|---|---|
| `has_key() argument 1 must be a dictionary` | The first argument is not a `dict` variable. |
| `has_key() needs the whole dictionary, not one of its entries` | The first argument has a key, like `p["a"]`. |
| `Key position 7 is out of range (the dictionary has 3 keys)` | `key_at()` was given a position that does not exist. |

## Related

- [dict](content/dictionaries/dictionary.md)
- [Arrays](content/arrays/arrays.md)
