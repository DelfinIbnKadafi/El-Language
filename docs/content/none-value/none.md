# The NONE Value

`NONE` represents "no value yet." It's a single concept that applies across every data type — `int`, `float`, `bool`, and `str`.

## When a Variable is NONE

A variable is `NONE` if it was declared without an initial value, or was explicitly assigned `NONE`.

```el
var int jj;
// the value of jj is NONE

print jj;
// prints "NONE"
```

```el
var int kk = 5;

kk = NONE;
print kk;
// prints "NONE"
```

## Printing a NONE Value

Printing a variable whose value is `NONE` prints the text `NONE`.

## NONE in Math Expressions

If a variable of type `int` or `float` is `NONE` and it's used in a math expression, it's treated as `0`.

```el
var float bb;

var float result = bb + 1;
// bb is treated as 0, so result is 1
```

## Comparing Against NONE

A variable can be compared against `NONE` using `==` or `!=`:

```el
if(jj == NONE) =
  print "jj has no value yet";
```

`NONE` comparisons only work directly against a variable (`var == NONE`), and only with `==` or `!=` — not `<`, `>`, `<=`, or `>=`.

## Related

- [Declaring Variables](content/variables/declaring-variables.md)
- [Comparison Operators](content/operators/comparison-operators.md)
- [Arithmetic](content/math/arithmetic.md)
