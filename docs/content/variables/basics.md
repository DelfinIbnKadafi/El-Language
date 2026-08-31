# Variable Basics

## Declaration

For creating or declared new variable in El, you must write true syntax:

```el
var data_type variable_name;
```

You can also give initiation value for the variables:

```el
var data_type variable_name = init_value;
```

### Data Type

El have 4 data type so far, that is:

1. int (Integer/Round number) [Integer Guide](content/variables/integer.md)
2. str (String/Text, including symbols) [String Guide](content/variables/string.md)
3. float (Decimal) [Float Guide](content/variables/float.md)
4. bool (Boolean true/false) [Bool Guide](content/variables/bool.md)

Example:

```el
var int number;
var str text;
var float decimal;
var bool boolean;
```

## Assignment

For assignment or fill variable with value, you just need to using equals symbols '=':

```el
variable_name = value;
```