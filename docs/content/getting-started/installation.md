# Installation

Install ELVM in your computer.

## 1. Install ELVM

Clone El Language Repository.

```bash
git clone https://github.com/DelfinIbnKadafi/El-Language.git
```

And compile the src files.

```bash
cd El-Language
make
```

Or download the compatible version in repository.

Go to https://github.com/DelfinIbnKadafi/El-Language and find release tab and download 
ELVM that compatible with your computer.

## 2. Run the files

Run using :

```bash
./path_into_elvm file.ell
```

Or

```bash
elvm.exe files.ell
```

## 3. Libraries (optional)

Libraries used with `library "name";` are searched in a folder named `library` placed in the same folder as the `elvm` executable, not in the folder where you run your program.

```
elvm
library/
  mathx/
    library.json
```

See [Library](content/modules/library.md) and [Creating a Library](content/modules/creating-a-library.md).
