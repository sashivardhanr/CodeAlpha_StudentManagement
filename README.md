# CodeAlpha_StudentManagement

A simple menu-driven student record management program written in C, built as part of my CodeAlpha C Programming Internship.

## Features

- Add a student record
- Delete a student record
- Update a student record
- Search by roll number or name
- Display all students
- Duplicate roll number prevention

## Concepts Used

- Structures
- Functions
- File handling (fread / fwrite)
- Loops
- Conditional statements

## Data Storage

Student records are saved in a binary file (`students.dat`) so they remain available after restarting the program.

## How to Compile

```bash
gcc student_management.c -o student_management
```

## How to Run

```bash
./student_management
```

## Example

```
1. Add Student
2. Delete Student
3. Update Student
4. Search Student
5. Display All Students
6. Exit

--- Add Student ---
Enter roll number: 101
Enter name: Sashi Vardhan
Enter course / branch: Computer Science
Enter marks (0-100): 88
Student added successfully.
```

## Author

Sashi Vardhan — CodeAlpha C Programming Intern
