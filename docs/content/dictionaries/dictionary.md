# dict

`dict` stores a set of named values, like a Python dictionary or a JSON object. Each value is stored under a **key**, a text written in quotes, and can be an `int`, a `float` or a `str`. Different keys of one dictionary can hold different types.

## Syntax

```el
var dict name;
var dict name = {"key1": value1, "key2": value2};

// an array of dictionaries
var dict name[size];
```

Reading and writing an entry:

```el
name["key"] = value;
variable = name["key"];
```

## Example

```el
var dict person = {"name": "Delfin", "age": 30};

person["score"] = 9.5;

print person["name"];
print person["age"];
print person["score"];

print person;
```

Output:

```
Delfin
30
9.5
{"name": "Delfin", "age": 30, "score": 9.5}
```

## Setting entries

`dict["key"] = value;` sets the entry. If the key is new it is added, otherwise its value is replaced, and the new value can be of another type.

```el
var dict item;

item["id"] = 1;
item["label"] = "box";
item["price"] = 2.5;

item["id"] = "A-1";   // the entry now holds a str
```

The value can be a number or math expression, a string, `input`, `format()`, a function call, or an entry of another dictionary (copied with the type it has).

```el
var dict a = {"x": 1, "name": "first"};
var dict b;

b["x"] = a["x"];
b["name"] = a["name"];
b["next"] = a["x"] + 1;
```

A dictionary cannot hold `NONE` or a `bool`. Store a bool as an `int` (`0` or `1`).

`+=`, `-=`, `++` and `--` work on an entry that holds a number:

```el
var dict stats = {"hits": 0, "average": 1.5};

stats["hits"]++;
stats["hits"] += 4;
stats["average"] -= 0.5;
```

The entry has to exist already, and `+` and `-` are not possible on a `str` entry.

## Reading entries

The compiler cannot know what type an entry holds, because that can change while the program runs. So the type is decided by where the entry is used, and the program checks it when it runs:

| Where the entry is used | It is read as |
|---|---|
| `var str s = dict["key"];` or a `str` parameter | a `str`, the entry has to hold a `str` |
| `var float f = dict["key"];` | a number, an `int` or a `float` entry |
| `var int n = dict["key"];` | a whole number: an `int` entry, or a `float` entry such as `4.0` |
| `print dict["key"];` | whatever the entry holds |
| an argument of `format()` or `print "..."` | whatever the entry holds, so it has to match its specifier |
| `int(dict["key"])`, `float(...)`, `str(...)` | converted like any other value, see [Type Conversion](content/data-types/type-conversion.md) |
| math, like `dict["a"] + 1` | a number, and the result is a `float` |
| a comparison | see below |

```el
var dict p = {"name": "Delfin", "age": 30, "score": 9.5};

var str name = p["name"];
var int age = p["age"];
var float score = p["score"];

print p["age"] + 1;
print int(p["score"]);
print str(p["age"]);
print format("%s is %d", p["name"], p["age"]);
```

A number read for math is a `float`, so `p["age"] + 1` cannot be put in an `int` variable directly. Wrap it, like `var int next = int(p["age"] + 1);`.

### What goes wrong

Reading the wrong type, or a key that is not there, stops the program with an error that names the key. Each of these three lines stops the program on its own, the message is shown as a comment:

```el
var dict p = {"name": "Delfin", "score": 9.5};

var int a = p["name"];
// Key "name" of dictionary 'p' holds a str, not a number

var int b = p["score"];
// Key "score" of dictionary 'p' holds the float 9.5, which is not a whole number

print p["city"];
// Key "city" not found in dictionary 'p'
```

Use [`has_key()`](content/dictionaries/dictionary-functions.md) to check a key before reading it.

## Keys

- A key is a `str`: a literal in quotes (`p["name"]`), or any `str` variable or expression (`p[field]`).
- Keys are case-sensitive, and can contain spaces or be empty.
- A key cannot be `NONE`, and a number is not a key: `p[0]` is an error.
- Keys keep the order they were first added in, and are listed in that order by `print` and [`key_at()`](content/dictionaries/dictionary-functions.md).

```el
var dict p;
var str field = "city";

p[field] = "Jakarta";
print p["city"];
```

## Comparisons

An entry can be compared with a number, a string, or another entry:

```el
var dict p = {"name": "Delfin", "age": 30};
var dict q = {"name": "Delfin", "age": 25};

if(p["age"] >= 18) =
  print "adult";

if(p["name"] == "Delfin") =
  print "same name as the text";

if(p["name"] == q["name"]) =
  print "same name";

if(p["age"] > q["age"]) =
  print "p is older";
```

- With a number, all of `==`, `!=`, `>`, `<`, `>=` and `<=` work.
- With a string, only `==` and `!=` work, like for any `str`.
- Two entries are compared by what both hold. A `str` and a number cannot be compared, that is a runtime error.
- An entry cannot be compared with `NONE`. Use `has_key()` to ask if a key exists.

## Arrays of dictionaries

`var dict name[size];` makes `size` dictionaries. Index the array first, then the key:

```el
var dict users[3];

users[0]["name"] = "Alice";
users[0]["age"] = 30;
users[1]["name"] = "Bob";
users[1]["age"] = 25;

var int i = 0;

while(i < 2) =
  print format("%s is %d", users[i]["name"], users[i]["age"]);
  i++;

print users[0];
```

Output:

```
Alice is 30
Bob is 25
{"name": "Alice", "age": 30}
```

- `len(users)` is the size of the array, `len(users[0])` is the number of keys of one dictionary.
- Every element starts as an empty dictionary.
- An index outside the array is a runtime error, like for any array.
- An array of dictionaries cannot be given initial values. Set the entries one by one.

## Printing a dictionary

`print dict;` writes the whole dictionary on one line, like JSON: keys and `str` values in double quotes, numbers without quotes.

```el
var dict p = {"name": "Delfin", "age": 30};

print p;
// {"name": "Delfin", "age": 30}
```

## Behavior

- A dictionary declared outside a function can be used inside functions, like any global variable.
- A dictionary declared inside a function is local, and is freed when the function returns.
- A dictionary declared again (for example inside a loop) starts empty each time.
- A `dict` variable always needs its key: `var int x = p;` and `p = 5;` are errors. A dictionary cannot be copied as a whole, copy the entries instead.
- `dict` is a reserved word, it cannot be used as a variable or function name.

## Not supported yet

- A dictionary as a function parameter, or returned from a function. Use a global dictionary, or pass the entries one by one.
- A dictionary inside a dictionary, and arrays as values.
- `bool` and `NONE` values.
- Copying a whole dictionary with `=`.

## Related

- [Dictionary Functions](content/dictionaries/dictionary-functions.md)
- [Arrays](content/arrays/arrays.md)
- [Type Conversion](content/data-types/type-conversion.md)
- [NONE](content/none-value/none.md)
