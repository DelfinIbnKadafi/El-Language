# Plugins

A plugin is a native program, written in C or in another language, that adds new functions to El. A plugin can do what El alone cannot: talk to a database such as MySQL, compute hashes, use the network, or call another system. A plugin belongs to a [library](content/modules/library.md), and a program uses it just by loading that library.

## Example

```el
library "hashing";

print sha256("hello");
print hash_crc32("hello");
```

`sha256` and `hash_crc32` are not written in El. They come from the plugin of the `hashing` library, and are called like any other function.

## How a Library Lists Its Plugins

The `plugins` field of `library.json` is a list of plugin files, written as paths inside the library folder.

```json
{
  "name": "hashing",
  "version": "1.0.0",
  "plugins": ["hashing.dll", "hashing.so"],
  "files": ["helpers.ell"]
}
```

| Extension | System |
|---|---|
| `.dll` | Windows |
| `.so` | Linux |
| `.dylib` | macOS |

- An entry is only loaded on the system that uses its extension. The others are skipped, so one library can list the plugin of every system it supports.
- An entry with no extension (`"hashing"`) gets the extension of the current system added.
- If the list has no file for the current system, loading the library is an error.
- `files` is optional when there is a `plugins` list: a library can be only a plugin.
- Like `files`, a plugin path cannot be absolute and cannot contain `..`.

## Behavior

- Plugins are loaded when the `library` statement is compiled, before the `.ell` files of the library. A file of the library can call the functions of its own plugin.
- A plugin function is a normal global function. It has fixed parameter types and a return type, so the compiler checks every call: the number of arguments, and that a `float` is not given where an `int` is expected.
- A plugin function can return `NONE`, and can receive `NONE` (see [NONE](content/none-value/none.md)).
- A plugin file is loaded only once, repeating `library` does nothing.
- A plugin cannot take a name that is already used by a function, a keyword, or a built-in function. Plugin authors give their functions a prefix (`mysql_connect`, `hash_crc32`) to avoid clashes between libraries.
- An `int` is a 32 bit number. A plugin that returns an `int` outside that range stops the program with a runtime error.
- A plugin function runs to the end before the program continues. Plugin functions cannot call El functions, and take no arrays.
- When the program ends, in any way (end of the file, `exit`, or an error), the plugins are told so they can close what they opened.

## Installing a Library With Plugins

`ellib` copies only the plugin file for the current system. Because a plugin is native code that runs with the same permissions as the program, `ellib` lists the plugins and asks for confirmation when the library comes from an unofficial repository (`ellib install -g user/repo`). Add `-y` to skip the question. Only install plugins from authors you trust.

## Common Errors

| Message | Cause |
|---|---|
| `Plugin "library/x/p.so" not found (expected <path>)` | The file listed in `plugins` does not exist. |
| `Library "x" has no plugin for this system ("plugins" needs a ".so" file)` | None of the listed files is for the current system. |
| `Cannot load plugin "library/x/p.so" (reason)` | The system refused to load it. The reason is the system's own text, for example a missing file the plugin needs, or a plugin built for another CPU type. |
| `"library/x/p.so" is not an EL plugin (it must export elp_Supports and elp_Load)` | The file is a normal shared library, not an EL plugin. |
| `Plugin "library/x/p.so" is made for plugin API version N, this ELVM uses version M` | The plugin was built for another version of the plugin API. |
| `Plugin "library/x/p.so" cannot add function 'f': ...` | The function name is reserved or already declared. |
| `In library.json of library "x": "plugins" must be a list` | `plugins` is not a list of strings. |

## Related

- [Writing a Plugin](content/modules/writing-a-plugin.md)
- [Creating a Library](content/modules/creating-a-library.md)
- [Library](content/modules/library.md)
