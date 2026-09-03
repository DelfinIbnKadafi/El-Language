# Exit Statement

`exit` stops the program's execution immediately.

## Syntax

```el
exit;
```

## Example

```el
var int age = 15;

if(age < 18) =
  print "Not allowed.";
  exit;

print "Welcome!";
```

## Behavior

`exit` can be used anywhere a statement is valid, including inside `if`, `for`, `while`, or a function body. Once reached, no further statements are executed.

## Related

- [If / Else](content/control-flow/if-else.md)
- [Return](content/functions/return.md)
