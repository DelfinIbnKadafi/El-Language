# Writing a Plugin

A plugin is a shared library (`.dll`, `.so` or `.dylib`) that exports three C functions. It is made against one header, `elplugin.h`, which is found in the `src` folder of El Language. The plugin does not link against `elvm`: everything goes through a table of functions (`ELApi`) that ELVM gives to the plugin when it starts. Any language that can export C functions with these names and types can be used.

## The Three Entry Points

```c
int  elp_Supports(void);          // required: return EL_PLUGIN_API_VERSION
int  elp_Load(const ELApi *api);  // required: register the functions, return 0
void elp_Unload(void);            // optional: called when elvm exits
```

Mark them with `EL_EXPORT`, which the header defines for each system.

`elp_Load` runs while the program is being **compiled**, when the `library` statement is read. Use it only to register functions. Open connections and other resources the first time a function needs them, not in `elp_Load`. `elp_Unload` runs when `elvm` exits, in every way it can exit, and is the place to close them.

## A Small Plugin

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elplugin.h"

static const ELApi *api;

// repeat_text(str, int):str
static void native_repeat(ELCall *call)
{
    const char *text = api->GetStr(call, 0);
    long long times = api->GetInt(call, 1);

    if (times < 0 || times > 1000) {
        api->Error(call, "repeat_text: times must be 0 to 1000, got %lld", times);
        return;
    }

    size_t length = strlen(text);
    char *result = malloc(length * (size_t)times + 1);

    result[0] = '\0';

    for (long long i = 0; i < times; i++)
        strcat(result, text);

    api->ReturnStr(call, result);
    free(result);
}

EL_EXPORT int elp_Supports(void)
{
    return EL_PLUGIN_API_VERSION;
}

EL_EXPORT int elp_Load(const ELApi *a)
{
    api = a;
    api->Register("repeat_text(str, int):str", native_repeat);
    return 0;
}
```

Build it and put it in a library folder:

```bash
# Linux
gcc -shared -fPIC -O2 -I path/to/El-Language/src repeat.c -o repeat.so

# Windows (MinGW)
gcc -shared -O2 -I path\to\El-Language\src repeat.c -o repeat.dll

# macOS
gcc -dynamiclib -O2 -I path/to/El-Language/src repeat.c -o repeat.dylib
```

`library/repeat/library.json`:

```json
{ "name": "repeat", "plugins": ["repeat.dll", "repeat.so", "repeat.dylib"] }
```

Using it:

```el
library "repeat";

print repeat_text("ab", 3);
// prints ababab
```

The plugin must be built for the same CPU type as `elvm` (a 64 bit `elvm` loads 64 bit plugins).

## Registering a Function

```c
api->Register("name(type, type, ...):returntype", function);
```

| Part | Meaning |
|---|---|
| `name` | Letters, digits and `_`, not starting with a digit. |
| `type` | `int`, `float`, `bool` or `str`. |
| `:returntype` | The same four types. Left out for a function that returns nothing, like `"log_write(str)"`. |

`Register` returns `1`, or `0` if the signature is not valid. A signature that is not valid also makes loading the library fail, with the reason in the error message.

A native function has the type `void f(ELCall *call)`.

## Reading Arguments and Giving the Result

| Function | Use |
|---|---|
| `GetInt(call, i)` | Argument `i` (counted from 0) as a `long long`. |
| `GetFloat(call, i)` | Argument `i` as a `double`. |
| `GetBool(call, i)` | Argument `i` as `0` or `1`. |
| `GetStr(call, i)` | Argument `i` as a `const char *`. Only valid until the function returns, copy it to keep it. |
| `IsNone(call, i)` | `1` if argument `i` is `NONE`. A `NONE` argument reads as `0`, `0.0`, `0` or `""`. |
| `ArgCount(call)` | The number of arguments, always the registered count. |
| `ReturnInt`, `ReturnFloat`, `ReturnBool` | The result for a number type. |
| `ReturnStr(call, text)` | The result for `str`. The text is copied, so the plugin keeps its own memory. |
| `ReturnNone(call)` | The result is `NONE` (for example a `NULL` column of a database). |
| `Error(call, format, ...)` | Stops the program with `file (line) : message`. Return right after calling it. |

Rules:

- The compiler already checked the number and the types of the arguments, so a plugin only reads them with the function that matches the type it registered. Reading an argument with the wrong kind of function (a `str` with `GetInt`) is a bug in the plugin and stops the program with an error.
- Give the result with the `Return` function that matches the registered return type. A function with a return type that never gives a result returns `NONE`.
- An `int` is a signed 32 bit number. If a function registered with `:int` gives a value outside -2147483648 to 2147483647, the program stops with a runtime error. Return a bigger number as a `float` or a `str`.
- A `str` result has no size limit.

## Handles

A plugin keeps its own resources (a database connection, a socket) and gives El a handle, a plain `int` that the plugin knows how to find.

```c
// mysql_connect(str, str, str, str, int):int   returns a handle, or NONE on failure
// mysql_query(int, str):int                      uses the handle
// mysql_close(int)
```

```el
var int db = mysql_connect("localhost", "user", "password", "shop", 3306);

if(db == NONE) =
  print "cannot connect";
```

## Limits

Version 1 of the plugin API does not have:

- Arrays as arguments or results.
- Callbacks from a plugin into El functions.
- Running a function in the background: a function must finish before the program continues.

The API table has a `size` field and new functions are only added at its end, so a plugin built today keeps working with newer versions of ELVM.

## Related

- [Plugins](content/modules/plugins.md)
- [Creating a Library](content/modules/creating-a-library.md)
