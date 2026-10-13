# Library

`library` loads a ready-made library by its name. A library is a folder of `.ell` files that is installed next to the `elvm` executable, so any program can use it without knowing where it is stored.

## Syntax

```el
library "library_name";
```

## Example

```el
library "mathx";

print square(4);
print cube(3);
// prints 16 and 27
```

## Where Libraries Are Stored

ELVM looks for a folder named `library` in the same folder where the `elvm` executable is located. This is **not** the folder where your program is run from, so the same libraries work from anywhere.

```
elvm
library/
  mathx/
    library.json
    core.ell
```

For `library "mathx";`, ELVM reads `library/mathx/library.json`, then copies the content of every file listed in its `files` into your program, just like [Module](content/modules/module.md) does. To make your own, see [Creating a Library](content/modules/creating-a-library.md).

## Library Manager

El Language has library manager call with ellib. Usually, ellib is including in linux/windows setup, or download the latest version in https://github.com/DelfinIbnKadafi/El-Library


### Install Library

Install from official repository https://github.com/DelfinIbnKadafi/El-Library

```bash
ellib install <lib_name>
```

Install from another github repository

```
ellib install <username>/<repos_name>
```

For another information, go to https://github.com/DelfinIbnKadafi/El-Library

## Behavior

- Files are copied in the order they are listed in `library.json`.
- `library` can only be used at the top level of a file, not inside a function or a block.
- A `library` statement must come before the code that uses the library, because EL compiles top-to-bottom.
- A library is loaded only once, repeating `library "mathx";` does nothing.
- The code of a library shares the same global scope as your program, so its names cannot clash with yours.
- A library name may only contain letters, digits, `_`, `-` and `.`, and cannot start with `.`. It is a folder name, so it can never point outside the `library` folder.
- `library` is a reserved word, it cannot be used as a variable or function name.

## Errors Point to the Right File

An error inside a library file shows the file as `library/<name>/<file>` and the line inside that file.

```
library/mathx/core.ell (4) : Division by zero
```

## Common Errors

| Message | Cause |
|---|---|
| `Library "x" not found (expected <path>)` | There is no `library/x/library.json` next to the `elvm` executable. The message shows the exact path that was searched. |
| `Invalid library name "x"` | The name has a character that is not allowed. |
| `Invalid library.json in library "x" (line N: ...)` | `library.json` is not valid JSON. |
| `In library.json of library "x": ...` | A required field is missing or has the wrong type, or a file path is not allowed. |
| `Cannot open file "library/x/file.ell"` | A file listed in `files` does not exist. |
| `Expected string after 'library'` | The name was not written as a string, e.g. `library mathx;`. |
| `'library' can only be used at the top level, not inside a function or a block` | `library` was used inside a function or a block. |

## Related

- [Creating a Library](content/modules/creating-a-library.md)
- [Module](content/modules/module.md)
- [Installation](content/getting-started/installation.md)
