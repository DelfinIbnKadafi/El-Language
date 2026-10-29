# Creating a Library

A library is a folder inside the `library` folder that sits next to the `elvm` executable. It holds `.ell` files and one `library.json` that describes it.

## Folder Layout

```
elvm
library/
  greeter/
    library.json
    hello.ell
    tools/
      shout.ell
```

The folder name (`greeter`) is the name used in the program: `library "greeter";`.

## library.json

```json
{
  "name": "greeter",
  "description": "Small helpers for greeting people.",
  "version": "1.0.0",
  "author": "Your Name",
  "files": ["hello.ell", "tools/shout.ell"]
}
```

| Field | Required | Meaning |
|---|---|---|
| `name` | Yes | The name of the library. |
| `description` | No | What the library does. |
| `version` | No | The version of the library. |
| `author` | No | Who made the library. |
| `files` | Yes, unless `plugins` is used | A list with at least one `.ell` file, written as paths inside the library folder. |
| `plugins` | No | A list of native plugin files (`.dll`, `.so`, `.dylib`), written as paths inside the library folder. See [Plugins](content/modules/plugins.md). |

`name`, `description`, `version` and `author` must be strings. Any other field is ignored, so it is safe to add more information.

A library can also be only a plugin: with a `plugins` list, `files` can be left out.

```json
{
  "name": "hashing",
  "version": "1.0.0",
  "plugins": ["hashing.dll", "hashing.so"]
}
```

## Rules for files

- Files are copied into the program in the listed order. Put a file before the files that use its functions and variables.
- Paths are relative to the library folder, and may use sub-folders written with `/` (a `\` is accepted too and read as `/`, so a library written on Windows also works on Linux).
- A path cannot be absolute and cannot contain `..`. A library can only use files from inside its own folder.
- A listed file can use [`module`](content/modules/module.md) to include more files, with the path relative to that file.

## Example

`library/greeter/hello.ell`:

```el
function greet_text(var str[32] name) =
  return format("Hello, %s!", name);
```

`library/greeter/tools/shout.ell`:

```el
function shout(var str[32] text) =
  return upper(text);
```

Using it:

```el
library "greeter";

print shout(greet_text("Delfin"));
// prints HELLO, DELFIN!
```

## Give your contribute to make some library fo El

https://github.com/DelfinIbnKadafi/EL-Library

## Related

- [Library](content/modules/library.md)
- [Plugins](content/modules/plugins.md)
- [Module](content/modules/module.md)
