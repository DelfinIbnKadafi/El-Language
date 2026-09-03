# Copying Values

The value of one array element can be copied into another element, as long as both share the same data type.

## Example

```el
var str names[5];
var str backup[5];

backup[0] = names[0];
```

```el
var int scores[3] = {10, 20, 30};
var int scoresCopy[3];

scoresCopy[0] = scores[0];
scoresCopy[1] = scores[1];
scoresCopy[2] = scores[2];
```

## Behavior

- Copying is done per-element, using a normal indexed assignment. There's no single statement that copies an entire array at once outside of a function call or return — see [Array Parameters](content/functions/array-parameters.md) and [Returning Arrays](content/functions/returning-arrays.md) for the cases where a whole array can be passed or returned in one step.
- Copying requires both elements to be the same data type; mismatched types are a compile-time error.

## Related

- [Arrays](content/arrays/arrays.md)
- [Array Parameters](content/functions/array-parameters.md)
- [Returning Arrays](content/functions/returning-arrays.md)
