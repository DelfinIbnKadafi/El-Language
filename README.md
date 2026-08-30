# El Language

El is a high-level programming language built on a custom Virtual Machine (VM) architecture.

It prioritizes simplicity, productivity, and portability, delivering a consistent execution environment across platforms through its own VM.

![Logo](logo.png)

## Architecture

El programs compile to bytecode and run inside the El Virtual Machine. The VM manages memory, execution flow, and runtime operations, ensuring applications work the same way everywhere.

## Code Example

```el
print "Hello World!";

// Variables
var int age = 25;
var str[20] name = "Alice";
var bool is_active = true;

// Arrays
var int scores[5] = {10, 20, 30, 40, 50};

// Function with return
function greet(var str[20] user) =
  return "Hello, " + user;

// Function with array parameter
function sum_array(var int arr[3]) =
  return arr[0] + arr[1] + arr[2];

// Recursion
function factorial(var int n) =
  if(n <= 1) =
    return 1;
  return n * factorial(n - 1);

// Control flow
if(age >= 18) =
  print greet(name);
  print "You are an adult.";
else =
  print "Access denied.";

// Loop
for(var int i = 0; i < 5; i++) =
  print i;

// Input
var str[50] user_input = input "Enter something: ";
print user_input;

// String comparison
if(user_input == "secret") =
  print "Access granted!";

exit;
```

## Getting Started

Open https://el-lang.my.id/ for complete documentation of El Language.

## License

[MIT License](LICENSE)

Open source. Feel free to explore, contribute, and improve the language.
