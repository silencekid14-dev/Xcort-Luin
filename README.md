**Luin (Silex) Programming Language**

**Version:** 2.0
**Date:** 10th July 2026  
**Author:** Xcort Team  

**New Update Feature On v2.0:** Introducing **try** Feature with **get( )** for errors & and making files!.

Luin (also known as Silex) is a modern, simple, and powerful programming language designed for both beginners and experienced developers. It combines clean syntax with powerful features, making it suitable for real-world software development.

## Goals & Vision

Luin aims to be a **versatile general-purpose language** capable of powering:
- **Frontend** development
- **Backend** systems
- **High-performance Math computation**
- **PCB hardware design & embedded systems**
- **Operating System architecture** and low-level programming

Our mission is to create one language that can be used across the entire technology stack — from user interfaces to hardware and operating systems.

## Key Features

### 1. Clean & Expressive Syntax
Luin has a simple, readable syntax that reduces boilerplate while maintaining power.

**Example:**
```sx
name = "Lyan"
age = 15
ready = true

show(f"Hello, my name is {name} and I am {age} years old.")

if age >= 18:
    show("Adult")
elf age >= 13:
    show("Teenager")
els:
    show("Child")
```

### 2. Built-in Interactive Input (`ask`)
Easy user input without complex code.

```sx
ask("Enter your name and age: " >> name, age)
show(f"Welcome {name}, you are {age} years old.")
```

### 3. Object-Oriented Programming with Classes
Support for classes, methods, and `self` for real-world modeling.

```sx
cls Player {
    fn init(playerName, hp) {
        self.playerName = playerName
        self.hp = hp
    }

    fn takeDamage(amount) {
        self.hp = self.hp - amount
        show(f"{self.playerName} took {amount} damage. HP now: {self.hp}")
    }
}

p = Player("Hero", 100)
p.takeDamage(30)
```


### 4. Try Feature & get( )
Support for error handling and making files
```sx
# ---------- get(value, "filename") : write a variable to a file ----------
name = "anna"
get(name, "file_name.txt")
show("wrote name to file_name.txt")

score = 97
get(score, "score.py")
show("wrote score to score.py")

# ---------- try { ... } + get(e) : catch errors instead of crashing ----------
try {
    x = 10 / 0
    show("this line never runs")
}
show(f"caught error: {get(e)}")
show("program kept running after the error")

# a try block with no error leaves get(e) empty
try {
    y = 5 + 5
    show(f"y = {y}")
}
show(f"error after clean try: '{get(e)}'")

# an undefined-variable error also gets caught
try {
    show(doesNotExist)
}
show(f"second error: {get(e)}")

```

## How to Compile and Run

### Requirements
- g++ (GCC C++ compiler) installed

### Compilation Steps

1. Save your code in a file with `.sx` extension (e.g. `program.sx`)

2. Compile using g++:
```bash
g++ -std=c++17 *.cpp resource.res -o Luin_version_name.exe
```

3. Run your program:
```bash
./luin program.sx
```

### Example Workflow
```bash
# Compile the compiler once
g++ -std=c++17 *.cpp resource.res -o Luin_version_name.exe

# Run your Luin program
./luin mygame.sx
```

## Current Status

Luin is under active development. Version 1.9.1 supports:
- Variables and basic types
- Control flow (if/elf/els, while, for, loop)
- Functions and classes
- f-strings and interactive input

More features (import system, modules, hardware support, etc.) are coming soon.

---

**Made with passion All over the world**  
**Xcort Team** — Building the future, one line at a time.

