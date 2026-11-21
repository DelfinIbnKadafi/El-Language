# Dictionary Functions

Functions that work on a whole [dictionary](content/dictionaries/dictionary.md). The first argument is the dictionary: a dictionary variable, an array element like `users[1]`, or an entry that holds a dictionary like `user["address"]`.

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

`len(dict["key"])` is the length of the text the entry holds, like `len()` of any `str`, or the number of keys if the entry holds a dictionary. For an array of dictionaries, `len(users)` is the size of the array and `len(users[0])` is the number of keys of one dictionary.

```el
var dict user = {"name": "Delfin", "address": {"city": "Jakarta", "zip": 12345}};

print len(user["name"]);
print len(user["address"]);
```

Output:

```
6
2
```

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

`has_key()` is also `true` for a key that holds `NONE`: the key exists, its value is `NONE`. To look inside a dictionary in a dictionary, give the inner one: `has_key(user["address"], "zip")`.

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

A position outside `0` to `len(dict) - 1` stops the program with an error. It works the same for an inner dictionary: `key_at(user["address"], 0)`.

`str(p[key])` turns an entry of any type into text, so the loop can print them all with `%s`.

## Common Errors

| Message | Cause |
|---|---|
| `has_key() argument 1 must be a dictionary` | The first argument is not a `dict` variable. |
| `Key "k" of dictionary 'd' holds an int, not a dictionary` | The first argument is an entry that does not hold a dictionary, like `has_key(p["age"], "x")`. |
| `Key position 7 is out of range (the dictionary has 3 key(s))` | `key_at()` was given a position that does not exist. |

## Related

- [dict](content/dictionaries/dictionary.md)
- [Arrays](content/arrays/arrays.md)
