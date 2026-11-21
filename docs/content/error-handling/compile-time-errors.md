# Compile-Time Errors

A compile-time error means ELVM found a problem in your code before running it. Compilation stops immediately and the program never runs.

## Error Format

```
filename (line) : message
```

`line` is the line number in your `.ell` file where the problem was found. If the problem is in code that came from [`module`](content/modules/module.md) or [`library`](content/modules/library.md), `filename` is the file that code is written in, and `line` is the line inside that file.

## Common Compile-Time Errors

| Message | Cause |
|---|---|
| `Variable 'x' already declared` | A variable name is already visible in the current or an enclosing scope. See [Scope: Global & Local](content/variables/scope.md). |
| `Function 'x' already declared` | Two functions in the same file share the same name. |
| `Functions cannot be nested` | A `function` was declared inside another function's body. |
| `Undefined symbol "x"` | A variable name was used that hasn't been declared. |
| `Undefined function "x"` | A function name was called that hasn't been declared yet. |
| `Too many arguments for function 'x'` / equivalent "too few" case | The number of arguments in a call doesn't match the function's parameter count. |
| `Cannot assign float value to int variable 'x'` | A `float` value was assigned directly to an `int` variable. |
| `String variable 'x' can only be compared with == or !=` | `<`, `>`, `<=`, or `>=` was used on a `str` value. |
| `Variable 'x' is an array and must be indexed` | An array variable was used without `[index]`. |
| `Expected ;` | A statement is missing its terminating semicolon. |
| `Cannot open file "x.ell"` | The file given to `module` (or listed in a library) does not exist. See [Module](content/modules/module.md). |
| `Library "x" not found (expected <path>)` | `library "x";` found no library with that name next to the `elvm` executable. See [Library](content/modules/library.md). |
| `Cannot load plugin "x" (reason)` | The operating system could not load the plugin file (wrong system or CPU type, or a file it needs is missing). See [Plugins](content/modules/plugins.md). |
| `Plugin "x" not found (expected <path>)` | A file listed in `plugins` does not exist. |
| `Library "x" has no plugin for this system` | `plugins` has no file for the system the program runs on. |
| `"x" is not an EL plugin` | The file loaded, but it does not export `elp_Supports` and `elp_Load`. |
| `Plugin "x" cannot add function 'f': ...` | A plugin function has the same name as a function already declared, or a name EL reserves. |
| `Dictionary 'd' needs a key here, like d["key"]` | A dictionary was used as a single value. Read or write one of its entries instead. |
| `A dictionary key must be a string, like d["name"]` | A dictionary key was written as a number, like `d[0]`. |
| `'users' is an array of dictionaries: write the element number first, like users[0]["key"]` | An array of dictionaries was used with a key but no element number, like `users["name"]` instead of `users[0]["name"]`. |
| `Variable 'users' is an array of dictionaries and must be indexed` | An array of dictionaries was used with nothing after it, like `print users;`. |
| `To store the result of a comparison, put it in a bool variable first` | A comparison (`dict["key"] = a > b;`) was written as the value of an entry. Put it in a `bool` variable first. |
| `Expected a dictionary (a dictionary variable, or a literal like {"key": value})` | Something that is not a dictionary was copied into a dictionary or passed as a dictionary parameter. |
| `Expected : after the key` / `Expected , or }` | A dictionary literal is not written correctly, like `{"a" 1}` or `{"a": 1 "b": 2}`. |
| `An array of dictionaries starts with {` / `Every element of an array of dictionaries is a dictionary written with { }` | Initial values of an array of dictionaries are not one `{...}` for each element. |
| `Too many initial values for array 'users' of size 2` | More `{...}` were given than the array has elements. |
| `A dictionary parameter cannot be an array` | A function parameter was written like `var dict d[3]`. |
| `An element of an array of dictionaries cannot be returned directly` | `return users[0];`. Copy it into a dictionary variable first and return that. |
| `Function 'f' returns a dictionary and cannot use a bare 'return;'` / `Function 'f' does not return a dictionary` | A function that returns a dictionary has a `return;` without a value, or a function that does not return one was used where a dictionary is wanted. |
| `Function 'f' does not return a numeric/bool value` | A function that returns a dictionary was used in math or as a number. Use it like `var dict x = f();`. |
| `'module' can only be used at the top level, not inside a function or a block` | `module` (or `library`) was used inside a function or a block. |

## Example

```el
var int score = 10;

if(true) =
  var int score = 20;
```

```
file.ell (3) : Variable 'score' already declared
```

## Related

- [Runtime Errors](content/error-handling/runtime-errors.md)
- [Scope: Global & Local](content/variables/scope.md)
- [Module](content/modules/module.md)
- [Library](content/modules/library.md)
