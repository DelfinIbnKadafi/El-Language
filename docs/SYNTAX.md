# Syntax

Note : The syntax flow follows each new update.

## 1. Print

   ```el
   print "Text";
   ```

   Example :
   ```el
   print "Hello Word!";
   ```

   ```el
   print variable;
   ```
   ```el
   print 1 + 10;
   ```
## 2. Exit

Use this if you wanna stop a program/elvm.

   ```el
   exit;
   ```
   
## 3. Variable

   #### Declaration

   ```el
   var data_type name;
   ```

   Example :
   ```el
   var int age = 10;
   var bool did_you_love_me;
   ```

   Supported Data Type :
   - int
   - str
   - bool
   - float

   #### Usage

   Example :

   ```el
   age = 10;
   did_you_love_me = false;
   age++;
   age--;
   ```
   
## Math

```el
10 + 5
(10 + 5) * 5
```

Example :
```el
age = 10 - 1;
```

## If/Else

#### If

Use this syntax for write if statement:

```el
if(condision) =
  // code here
```

You can use 1 or more space indentation for if/else statement.

Example :
```el
if(Elfaria > 10) =
  // 2 indentation
if(Elfaria < 10) =
 // 1 indentation
```

#### Else

Use this syntax to create else statment, else statement should have if statement first.

Example :

```el
if(condision) =
  //
else =
  //
```

#### Else if

Else statement doesn't have condision, but else if needed a condision.

For Example :

```el
if(condision) =
  //
else if(condision) =
 //
```

#### Multiple if statement

You can create a multiple if statement.

Example :

```el
if(condision) =
  if(condision) =
```


You also can use of statement for boolean type.

For Examples :

if you create this.

```el
if(life) =
```

it's same with :

```el
if(life == true) =
```

Or use **!** for false equals.

```el
if(!life) =
```

same with :

```el
if(life == false) =
```
