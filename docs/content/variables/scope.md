# Scope: Global & Local

Where a variable is declared determines where it can be used.

## Global Variables

A variable declared outside of any `if`, `for`, `while`, or `function` block is global. It can be used anywhere in the file below the line where it was declared.

```el
var int score = 0;

function addPoint() =
  score += 1;
```

## Local Variables

A variable declared inside a block is local to that block. Once the block ends, the variable no longer exists.

```el
if(true) =
  var int local_number = 5;
  print local_number; // works fine

print local_number; // error, local_number doesn't exist here
```

Because a local variable disappears once its block ends, the same name can be reused in a later, unrelated block:

```el
for(var int i = 0; i < 5; i++) =
  print i;

for(var int i = 0; i < 3; i++) =
  print i;
// works -- the second "i" is a brand new variable
```

## No Shadowing

EL does not support shadowing. If a name is already visible from an enclosing scope, it cannot be reused for a new variable in a nested block, even if that would otherwise be convenient:

```el
var int score = 10;

if(true) =
  var int score = 20;
  // error: Variable 'score' already declared
```

This also applies to function parameters and any variable declared inside a function body — a parameter or local variable inside a function cannot reuse a name that's already declared as a global variable.

## Related

- [Declaring Variables](content/variables/declaring-variables.md)
- [Functions](content/functions/functions.md)
- [Parameters](content/functions/parameters.md)
