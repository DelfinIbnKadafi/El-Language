# dict

`dict` stores a set of named values, like a Python dictionary or a JSON object. Each value is stored under a **key**, a text written in quotes. A value can be an `int`, a `float`, a `str`, a `bool`, `NONE`, or another dictionary, and the keys of one dictionary can hold different types.

## Syntax

```el
var dict name;
var dict name = {"key1": value1, "key2": value2};
var dict name = other;

// an array of dictionaries
var dict name[size];
var dict name[2] = { {"key": 1}, {"key": 2} };
```

Reading and writing an entry:

```el
name["key"] = value;
variable = name["key"];
name["inner"]["key"] = value;   // a dictionary inside a dictionary
```

## Example

```el
var dict person = {"name": "Delfin", "age": 30, "active": true};

person["score"] = 9.5;
person["address"] = {"city": "Jakarta"};

print person["name"];
print person["address"]["city"];

print person;
```

Output:

```
Delfin
Jakarta
{"name": "Delfin", "age": 30, "active": true, "score": 9.5, "address": {"city": "Jakarta"}}
```

## Setting entries

`dict["key"] = value;` sets the entry. If the key is new it is added, otherwise its value is replaced, and the new value can be of another type.

```el
var dict item;

item["id"] = 1;
item["label"] = "box";
item["price"] = 2.5;
item["in_stock"] = true;
item["note"] = NONE;

item["id"] = "A-1";   // the entry now holds a str
```

The value can be a number or math expression, a string, `input`, `format()`, a function call, `true` or `false` (or a `bool` variable, or a function that gives a `bool`), `NONE`, a dictionary, or an entry of another dictionary (copied with the type it has).

```el
var dict a = {"x": 1, "name": "first"};
var dict b;

b["x"] = a["x"];
b["name"] = a["name"];
b["next"] = a["x"] + 1;
```

If a variable that holds `NONE` is stored, the entry becomes `NONE` too.

`+=`, `-=`, `++` and `--` work on an entry that holds a number:

```el
var dict stats = {"hits": 0, "average": 1.5};

stats["hits"]++;
stats["hits"] += 4;
stats["average"] -= 0.5;
```

The entry has to exist already, and `+` and `-` are not possible on an entry that holds text, a `bool`, `NONE` or a dictionary.

## Reading entries

The compiler cannot know what type an entry holds, because that can change while the program runs. So the type is decided by where the entry is used, and the program checks it when it runs:

| Where the entry is used | It is read as |
|---|---|
| `var str s = dict["key"];` or a `str` parameter | a `str`, the entry has to hold a `str` |
| `var float f = dict["key"];` | a number, an `int` or a `float` entry |
| `var int n = dict["key"];` | a whole number: an `int` entry, or a `float` entry such as `4.0` |
| `var bool b = dict["key"];` | a `bool`: a `bool` entry is its value, a number counts as true if it is not 0, text is an error |
| `print dict["key"];` | whatever the entry holds |
| an argument of `format()` or `print "..."` | whatever the entry holds, so it has to match its specifier |
| `int(dict["key"])`, `float(...)`, `str(...)` | converted like any other value, see [Type Conversion](content/data-types/type-conversion.md) |
| math, like `dict["a"] + 1` | a number, see below |
| an `if` or `while` condition | see [Comparisons](#comparisons) |

An entry that holds `NONE` can be read as any type, and the variable it is put in becomes `NONE`:

```el
var dict p = {"name": "Delfin", "nick": NONE};

var str nick = p["nick"];

if(nick == NONE) =
  print "no nickname";
```

### Math on entries

A number read for math is a `float`, because the compiler does not know if the entry is an `int` or a `float`. You do not have to care about that most of the time:

- the result can go in an `int` variable, as a function argument or as an array index when it is a whole number (the program stops with an error if it is not): `var int next = p["age"] + 1;`
- the result can be stored in a dictionary: a whole number is stored as an `int`;
- `print`, `format()` and `str()` write a whole result as an integer, so `print p["population"] + 1;` gives `1234568`, not `1.23457e+06`.

```el
var dict p = {"age": 30, "price": 2.5, "population": 1234567};

var int next = p["age"] + 1;
p["age"] = p["age"] + 1;

print p["population"] + 1;
print p["price"] * 2;
print format("%s is %d", "Delfin", p["age"] + 1);
```

Output:

```
1234568
5
Delfin is 32
```

### What goes wrong

Reading the wrong type, or a key that is not there, stops the program with an error that names the key. Each of these lines stops the program on its own, the message is shown as a comment:

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

## Dictionaries inside dictionaries

An entry can hold a dictionary. Put more keys after the first one to go inside:

```el
var dict user = {"name": "Delfin", "address": {"city": "Jakarta", "geo": {"lat": -6.2}}};

print user["address"]["city"];
print user["address"]["geo"]["lat"];

user["address"]["city"] = "Bandung";
user["address"]["zip"] = 40111;

user["job"] = {};
user["job"]["title"] = "developer";

print user["address"];
```

Output:

```
Jakarta
-6.2
{"city": "Bandung", "geo": {"lat": -6.2}, "zip": 40111}
```

- The inner dictionary has to exist before you set a key in it: `user["job"] = {};` first. Reading or writing a key inside something that is missing, or is not a dictionary, is a runtime error.
- `print user["address"];` writes the inner dictionary. `len()`, `has_key()`, `remove_key()` and `key_at()` also take one: `has_key(user["address"], "zip")`.
- Dictionaries can be nested 100 levels deep.

## Copying dictionaries

A dictionary is copied when it is assigned, and the copy is independent. Changing one does not change the other, and that includes the dictionaries inside it.

```el
var dict a = {"x": 1, "inner": {"deep": "v"}};
var dict b = a;
var dict c;

c = a;
b["inner"]["deep"] = "changed";

print a;
print b;
```

Output:

```
{"x": 1, "inner": {"deep": "v"}}
{"x": 1, "inner": {"deep": "changed"}}
```

All of these copy:

```el
var dict b = a;               // a new dictionary
b = a;                        // replaces what b held
b = {"y": 2};                 // from a literal, b = {} makes it empty
b["copy"] = a;                // an entry that holds a copy
users[1] = users[0];          // elements of an array of dictionaries
var dict part = user["address"];   // a dictionary from inside another one
```

A literal is evaluated before `b` is changed, so `s = {"n": s["n"] + 1};` works.

## Dictionaries and functions

A dictionary can be a parameter and can be returned. Like an array, it is passed **by value**: the function works on its own copy, and the caller's dictionary is not changed.

```el
function make_user(var str name, var int age) =
  var dict u;

  u["name"] = name;
  u["age"] = age;

  return u;

function birthday(var dict d) =
  d["age"] += 1;

  return int(d["age"]);

var dict bob = make_user("Bob", 25);

print birthday(bob);
print bob["age"];
```

Output:

```
26
25
```

- `return dict;` and `return {...};` return a copy of the whole dictionary. To return an entry write `return int(dict["key"]);` or `return str(dict["key"]);`. A dictionary that is an element of an array, or inside another dictionary, is copied into a variable first.
- A function that returns a dictionary can be used where a dictionary is wanted: `var dict x = f();`, `x = f();`, `x["key"] = f();`, as an argument `g(f())`, or `print f();`. As a statement, `f();`, the result is thrown away.
- A function that returns an entry, like `return d["n"];`, gives a `float`. Write `return int(d["n"]);` for an `int`.
- A dictionary declared outside a function can also be used inside it, like any global variable. Then the function changes the real dictionary.
- A parameter cannot be an array of dictionaries.

## Comparisons

An entry can be compared with a number, a string, a bool, `NONE`, or another entry:

```el
var dict p = {"name": "Delfin", "age": 30, "active": true, "nick": NONE};
var dict q = {"name": "Delfin", "age": 25};

if(p["age"] >= 18) =
  print "adult";

if(p["name"] == "Delfin") =
  print "same name as the text";

if(p["name"] == q["name"]) =
  print "same name";

if(p["age"] > q["age"]) =
  print "p is older";

if(p["active"]) =
  print "active";

if(p["nick"] == NONE) =
  print "no nickname";
```

- With a number, all of `==`, `!=`, `>`, `<`, `>=` and `<=` work.
- With a string, only `==` and `!=` work, like for any `str`. The same goes for a `bool`.
- Two entries are compared by what both hold. A `str` and a number, or a `bool` and a number, cannot be compared, that is a runtime error. Two dictionaries cannot be compared.
- `entry == NONE` and `entry != NONE` ask if the entry holds `NONE`. The key has to exist.
- An entry on its own as a condition (`if(p["active"])`) is read the same way: its value if it is a `bool`, true if it is a number that is not 0. Text is an error.
- To store the result of a comparison, put it in a `bool` variable first: `var bool ok = a > b;` then `p["ok"] = ok;`.

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

Values can be given when it is declared, one `{...}` for each element, starting from the first. Elements that are left out start empty.

```el
var dict team[3] = { {"name": "A", "age": 1}, {"name": "B", "age": 2} };

print team[1];
print team[2];
```

Output:

```
{"name": "B", "age": 2}
{}
```

- `len(users)` is the size of the array, `len(users[0])` is the number of keys of one dictionary.
- An index outside the array is a runtime error, like for any array.

## Printing a dictionary

`print dict;` writes the whole dictionary on one line, like JSON: keys and `str` values in double quotes, numbers without quotes, `true` and `false` for bools, `NONE` for `NONE`, and inner dictionaries in `{ }`.

```el
var dict p = {"name": "Delfin", "age": 30, "nick": NONE, "address": {"city": "Jakarta"}};

print p;
// {"name": "Delfin", "age": 30, "nick": NONE, "address": {"city": "Jakarta"}}
```

## Behavior

- A dictionary declared outside a function can be used inside functions, like any global variable.
- A dictionary declared inside a function is local, and is freed when the function returns.
- A dictionary declared again (for example inside a loop) starts empty each time.
- A `dict` variable on its own can only be used where a whole dictionary is wanted (copying, a function parameter, `print`, `len()` and the other dictionary functions). `var int x = p;` is an error.
- `dict` is a reserved word, it cannot be used as a variable or function name.

## Not supported yet

- Lists (arrays) as values. A dictionary entry holds one value, use keys like `"item1"` or an array of dictionaries.
- Comparing two whole dictionaries.
- An array of dictionaries as a function parameter, and returning an array of dictionaries.

## Related

- [Dictionary Functions](content/dictionaries/dictionary-functions.md)
- [Arrays](content/arrays/arrays.md)
- [Type Conversion](content/data-types/type-conversion.md)
- [NONE](content/none-value/none.md)
