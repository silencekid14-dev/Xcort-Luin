**Luin (Silex) Programming Language**

**Version:** 2.1.2
**Date:** 12th July 2026.  
**Author:** Xcort Team  

**New Update Feature On v2.1.2:** Bug Fixing and Enhancing array indexing capabilities.

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

### 5. Get( ) expansion & modules
Supports math module(cin,pow & upto 15 more!) 
```sx
# ============================================================
# demo_v2_features.sx — math module, get() read mode, del(), crt()
# ============================================================

# ---------- import math ----------
import math
show(math.sqrt(16))
show(math.pow(2, 10))
show(math.sin(math.pi / 2))
show(math.max(3, 9))

# ---------- crt(): create a file and a folder ----------
crt("notes.txt")
crt("data")                      # folder (no extension)
crt("nested.txt", "data/sub")    # data/sub/nested.txt, creating the path

# ---------- get() WRITE: value -> file ----------
name = "Luin"
get(name, "notes.txt")
get("hello from data/sub", "data/sub/nested.txt")

# ---------- get() READ: file -> variable ----------
get("notes.txt", loadedName)
get("data/sub/nested.txt", loadedNested)
show(f"notes.txt contains: {loadedName}")
show(f"nested file contains: {loadedNested}")

# ---------- try/get(e) protecting a bad read ----------
try {
    get("this_file_does_not_exist.txt", missing)
}
show(f"read error safely caught: {get(e)}")

# ---------- del(): clean everything up ----------
del("notes.txt")
del("data")     # removes the whole folder tree, including nested.txt
show("cleanup done, program did not crash")

```

### 6. Bug Fixes and Extended capabilities.
Now supports indexing and fixed bugs, has shown in the code below
```sx
# ============================================================
# EMPLOYEE MANAGEMENT SYSTEM - Luin v2.1.2 & 2.1 only!
# Corrected Syntax - Based on Official Examples
# Demonstrates: Classes, File I/O, Error Handling, Math, Loops
# ============================================================

import math

# ---------- Employee Class ----------
cls Employee {
    fn init(id, name, salary, department) {
        self.id = id
        self.name = name
        self.salary = salary
        self.department = department
        self.bonus = 0
    }

    fn giveBonus(percent) {
        self.bonus = self.salary * (percent / 100)
        self.salary = self.salary + self.bonus
        show(f"Bonus of {percent}% given to {self.name}")
        show(f"New salary: {self.salary} (Bonus: {self.bonus})")
    }

    fn display() {
        show(f"ID: {self.id} | Name: {self.name} | Dept: {self.department} | Salary: {self.salary}")
    }

    fn getAnnualSalary() {
        rtn self.salary * 12
    }
}

# ---------- Company Class ----------
cls Company {
    fn init(name) {
        self.name = name
        self.employees = []
        self.employeeCount = 0
    }

    fn hire(employee) {
        self.employees = self.employees + [employee]
        self.employeeCount = self.employeeCount + 1
        show(f"Hired: {employee.name}")
        try {
            get(f"Hired: {employee.name} at {self.name}", "hire_log.txt")
        }
        show(f"Log written to hire_log.txt")
    }

    fn fire(employeeId) {
        try {
            newList = []
            i = 0
            while i < len(self.employees) {
                emp = self.employees[i]
                if emp.id != employeeId: newList = newList + [emp]
                els: show(f"Fired: {emp.name}")
                i = i + 1
            }
            self.employees = newList
            self.employeeCount = self.employeeCount - 1
        }
        show(f"Fire operation completed. Error: {get(e)}")
    }

    fn listAll() {
        show(f"=== {self.name} - Employee List ({self.employeeCount} employees) ===")
        i = 0
        while i < len(self.employees) {
            self.employees[i].display()
            i = i + 1
        }
    }

    fn getTotalPayroll() {
        total = 0
        i = 0
        while i < len(self.employees) {
            total = total + self.employees[i].getAnnualSalary()
            i = i + 1
        }
        rtn total
    }

    fn getAverageSalary() {
        if self.employeeCount > 0: rtn self.getTotalPayroll() / self.employeeCount
        rtn 0
    }

    fn saveToFile(filename) {
        try {
            data = f"Company: {self.name}\n"
            data = data + f"Employees: {self.employeeCount}\n"
            data = data + "---\n"
            i = 0
            while i < len(self.employees) {
                emp = self.employees[i]
                data = data + f"{emp.id},{emp.name},{emp.salary},{emp.department}\n"
                i = i + 1
            }
            get(data, filename)
            show(f"Saved company data to {filename}")
        }
        show(f"Save error: {get(e)}")
    }

    fn loadFromFile(filename) {
        try {
            get(filename, fileData)
            show(f"Loaded data from {filename}")
            show(f"File contents: {fileData}")
        }
        show(f"Load error: {get(e)}")
    }

    fn findHighestPaid() {
        if self.employeeCount == 0: {
            show("No employees")
            rtn
        }
        highest = self.employees[0]
        i = 1
        while i < len(self.employees) {
            if self.employees[i].salary > highest.salary: highest = self.employees[i]
            i = i + 1
        }
        show(f"Highest paid employee:")
        highest.display()
        rtn highest
    }

    fn findDepartmentStats() {
        # Simple department counter (limited to 3 depts for demo)
        dept1 = ""
        dept2 = ""
        dept3 = ""
        count1 = 0
        count2 = 0
        count3 = 0

        i = 0
        while i < len(self.employees) {
            dept = self.employees[i].department
            if dept1 == "": {
                dept1 = dept
                count1 = 1
            } elf dept == dept1: {
                count1 = count1 + 1
            } elf dept2 == "": {
                dept2 = dept
                count2 = 1
            } elf dept == dept2: {
                count2 = count2 + 1
            } elf dept3 == "": {
                dept3 = dept
                count3 = 1
            } elf dept == dept3: {
                count3 = count3 + 1
            }
            i = i + 1
        }

        show("=== Department Statistics ===")
        if count1 > 0: show(f"{dept1}: {count1} employees")
        if count2 > 0: show(f"{dept2}: {count2} employees")
        if count3 > 0: show(f"{dept3}: {count3} employees")
    }
}

# ---------- Helper Functions ----------
fn createEmployee(id, name, salary, department) {
    try {
        if salary <= 0: {
            show("Error: Salary must be positive!")
            rtn
        }
        emp = Employee(id, name, salary, department)
        show(f"Created employee: {name}")
        rtn emp
    }
    show(f"Error creating employee: {get(e)}")
    rtn
}

fn calculateStats(numbers) {
    show("=== Math Stats ===")
    sum = 0
    i = 0
    while i < len(numbers) {
        sum = sum + numbers[i]
        i = i + 1
    }
    avg = sum / len(numbers)
    show(f"Sum: {sum}")
    show(f"Average: {avg}")
    show(f"Max: {math.max(numbers)}")
    show(f"Square root of sum: {math.sqrt(sum)}")
}

# ============================================================
# MAIN PROGRAM
# ============================================================

show("========================================")
show("   EMPLOYEE MANAGEMENT SYSTEM v2.1")
show("   Built with Luin (SILEX)")
show("========================================")

# ---------- Create Company ----------
company = Company("Xcort Industries")
show(f"Welcome to {company.name}!")

# ---------- Hire Employees ----------
show("\n--- Hiring Employees ---")
emp1 = createEmployee(101, "Alice Johnson", 75000, "Engineering")
if emp1: company.hire(emp1)

emp2 = createEmployee(102, "Bob Smith", 68000, "Marketing")
if emp2: company.hire(emp2)

emp3 = createEmployee(103, "Carol White", 92000, "Engineering")
if emp3: company.hire(emp3)

emp4 = createEmployee(104, "David Brown", 54000, "Sales")
if emp4: company.hire(emp4)

emp5 = createEmployee(105, "Eva Green", 81000, "Engineering")
if emp5: company.hire(emp5)

# ---------- List All Employees ----------
show("\n--- Employee Directory ---")
company.listAll()

# ---------- Give Bonus to Engineering Team ----------
show("\n--- Giving Bonuses ---")
i = 0
while i < len(company.employees) {
    emp = company.employees[i]
    if emp.department == "Engineering": emp.giveBonus(10)  # 10% bonus
    i = i + 1
}

# ---------- Company Statistics ----------
show("\n--- Company Statistics ---")
show(f"Total Annual Payroll: ${company.getTotalPayroll()}")
show(f"Average Annual Salary: ${company.getAverageSalary()}")

# ---------- Find Highest Paid ----------
show("\n--- Top Employee ---")
company.findHighestPaid()

# ---------- Department Stats ----------
company.findDepartmentStats()

# ---------- Save to File ----------
show("\n--- Saving Data ---")
company.saveToFile("company_data.txt")

# ---------- Load from File (with error handling) ----------
show("\n--- Loading Data ---")
company.loadFromFile("company_data.txt")

# ---------- Try loading a non-existent file (error handling demo) ----------
show("\n--- Error Handling Demo ---")
try {
    get("non_existent_file.txt", missingData)
    show("This line won't run")
}
show(f"Error caught: {get(e)}")

# ---------- Math Module Demo ----------
show("\n--- Math Module Demo ---")
numbers = [10, 25, 30, 45, 50]
calculateStats(numbers)

# ---------- File Creation Demo ----------
show("\n--- File Creation Demo ---")
crt("reports")
crt("summary.txt", "reports")
show("Created reports folder and summary.txt inside it")

# ---------- Cleanup ----------
show("\n--- Cleanup ---")
del("company_data.txt")
del("hire_log.txt")
del("reports")
show("Cleanup complete!")

# ---------- Program End ----------
show("\n========================================")
show("   Program completed successfully!")
show("   Thank you for using Luin v2.1")
show("========================================")
```

### 7. Old code supported with and without Len( )
Supports Len( ), also added ***else*** has an alias of **els** , and supporting if statements with {...} and with ":" etc.
```sx
# ============================================================
# EMPLOYEE MANAGEMENT SYSTEM - Luin v2.1.2 Compatible and 2.1 only!
# No len(), no null, no indexing, no array concat.
# ============================================================

import math

# ---------- Employee Class ----------
cls Employee {
    fn init(id, name, salary, department) {
        self.id = id
        self.name = name
        self.salary = salary
        self.department = department
        self.bonus = 0
    }

    fn giveBonus(percent) {
        self.bonus = self.salary * (percent / 100)
        self.salary = self.salary + self.bonus
        show(f"Bonus of {percent}% given to {self.name}")
        show(f"New salary: {self.salary} (Bonus: {self.bonus})")
    }

    fn display() {
        show(f"ID: {self.id} | Name: {self.name} | Dept: {self.department} | Salary: {self.salary}")
    }

    fn getAnnualSalary() {
        rtn self.salary * 12
    }
}

# ---------- Company Class ----------
cls Company {
    fn init(name, employees) {
        self.name = name
        self.employees = employees
        # Count employees manually (no len)
        self.employeeCount = 0
        for emp employees {
            self.employeeCount = self.employeeCount + 1
        }
    }

    fn listAll() {
        show(f"=== {self.name} - Employee List ({self.employeeCount} employees) ===")
        for emp self.employees {
            emp.display()
        }
    }

    fn getTotalPayroll() {
        total = 0
        for emp self.employees {
            total = total + emp.getAnnualSalary()
        }
        rtn total
    }

    fn getAverageSalary() {
        if self.employeeCount > 0 :  rtn self.getTotalPayroll() / self.employeeCount
        rtn 0
    }

    fn findHighestPaid() {
        if self.employeeCount == 0 {
            show("No employees")
            rtn
        }
        # Use indexing (now supported) to grab the first employee, then
        # compare the rest against it.
        highest = self.employees[0]
        i = 1
        while i < len(self.employees) {
            if self.employees[i].salary > highest.salary {
                highest = self.employees[i]
            }
            i = i + 1
        }
        show("Highest paid employee:")
        highest.display()
        rtn highest
    }

    # Department stats using only for loops
    fn findDepartmentStats() {
        dept1 = ""
        dept2 = ""
        dept3 = ""
        count1 = 0
        count2 = 0
        count3 = 0

        for emp in self.employees {
            dept = emp.department
            if dept1 == "" {
                dept1 = dept
                count1 = 1
            } elf dept == dept1 {
                count1 = count1 + 1
            } elf dept2 == "" {
                dept2 = dept
                count2 = 1
            } elf dept == dept2 {
                count2 = count2 + 1
            } elf dept3 == "" {
                dept3 = dept
                count3 = 1
            } elf dept == dept3 {
                count3 = count3 + 1
            }
        }

        show("=== Department Statistics ===")
        if count1 > 0 { show(f"{dept1}: {count1} employees") }
        if count2 > 0 { show(f"{dept2}: {count2} employees") }
        if count3 > 0 { show(f"{dept3}: {count3} employees") }
    }

    fn saveToFile(filename) {
        try {
            data = f"Company: {self.name}\n"
            data = data + f"Employees: {self.employeeCount}\n"
            data = data + "---\n"
            for emp in self.employees {
                data = data + f"{emp.id},{emp.name},{emp.salary},{emp.department}\n"
            }
            get(data, filename)
            show(f"Saved company data to {filename}")
        }
        show(f"Save error: {get(e)}")
    }

    fn loadFromFile(filename) {
        try {
            get(filename, fileData)
            show(f"Loaded data from {filename}")
            show(f"File contents: {fileData}")
        }
        show(f"Load error: {get(e)}")
    }
}

# ---------- Helper Functions ----------
fn createEmployee(id, name, salary, department) {
    try {
        if salary <= 0 {
            show("Error: Salary must be positive!")
            rtn
        }
        emp = Employee(id, name, salary, department)
        show(f"Created employee: {name}")
        rtn emp
    }
    show(f"Error creating employee: {get(e)}")
    rtn
}

# Compute stats without len and without math.max(array)
fn calculateStats(numbers) {
    show("=== Math Stats ===")
    sum = 0
    count = 0
    maxVal = -999999999   # a very small number (or use a flag)
    minVal = 999999999
    first = true
    for n in numbers {
        sum = sum + n
        count = count + 1
        if first {
            maxVal = n
            minVal = n
            first = false
        } else {
            if n > maxVal { maxVal = n }
            if n < minVal { minVal = n }
        }
    }
    avg = sum / count
    show(f"Sum: {sum}")
    show(f"Average: {avg}")
    show(f"Max: {maxVal}")
    show(f"Min: {minVal}")
    show(f"Square root of sum: {math.sqrt(sum)}")
}

# ============================================================
# MAIN PROGRAM
# ============================================================

show("========================================")
show("   EMPLOYEE MANAGEMENT SYSTEM v2.1")
show("   Built with Luin (SILEX)")
show("========================================")

# ---------- Create Employees ----------
emp1 = createEmployee(101, "Alice Johnson", 75000, "Engineering")
emp2 = createEmployee(102, "Bob Smith", 68000, "Marketing")
emp3 = createEmployee(103, "Carol White", 92000, "Engineering")
emp4 = createEmployee(104, "David Brown", 54000, "Sales")
emp5 = createEmployee(105, "Eva Green", 81000, "Engineering")

# Put them in an array literal (no dynamic addition)
employees = [emp1, emp2, emp3, emp4, emp5]

# ---------- Create Company ----------
company = Company("Xcort Industries", employees)
show(f"Welcome to {company.name}!")

# ---------- List All Employees ----------
show("\n--- Employee Directory ---")
company.listAll()

# ---------- Give Bonus to Engineering Team ----------
show("\n--- Giving Bonuses ---")
for emp in company.employees {
    if emp.department == "Engineering" {
        emp.giveBonus(10)   # 10% bonus
    }
}

# ---------- Company Statistics ----------
show("\n--- Company Statistics ---")
show(f"Total Annual Payroll: ${company.getTotalPayroll()}")
show(f"Average Annual Salary: ${company.getAverageSalary()}")

# ---------- Find Highest Paid ----------
show("\n--- Top Employee ---")
company.findHighestPaid()

# ---------- Department Stats ----------
company.findDepartmentStats()

# ---------- Save to File ----------
show("\n--- Saving Data ---")
company.saveToFile("company_data.txt")

# ---------- Load from File (with error handling) ----------
show("\n--- Loading Data ---")
company.loadFromFile("company_data.txt")

# ---------- Try loading a non-existent file (error handling demo) ----------
show("\n--- Error Handling Demo ---")
try {
    get("non_existent_file.txt", missingData)
    show("This line won't run")
}
show(f"Error caught: {get(e)}")

# ---------- Math Module Demo ----------
show("\n--- Math Module Demo ---")
numbers = [10, 25, 30, 45, 50]
calculateStats(numbers)

# ---------- File Creation Demo ----------
show("\n--- File Creation Demo ---")
crt("reports")
crt("summary.txt", "reports")
show("Created reports folder and summary.txt inside it")

# ---------- Cleanup ----------
show("\n--- Cleanup ---")
del("company_data.txt")
del("reports")
show("Cleanup complete!")

# ---------- Program End ----------
show("\n========================================")
show("   Program completed successfully!")
show("   Thank you for using Luin v2.1")
show("========================================")
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

Luin is under active development. Version 2.1 supports:
- Variables and basic types
- Control flow (if/elf/els, while, for, loop)
- Functions and classes
- f-strings and interactive input
- imports and more.

More features (import system, modules, hardware support, etc.) are coming soon.

---

**Made with passion All over the world**  
**Xcort Team** — Building the future, one line at a time.

