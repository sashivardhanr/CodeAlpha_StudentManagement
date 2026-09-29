/*
 * CodeAlpha C Programming Internship
 * Task 3: Student Management System
 *
 * A menu-driven program to manage student records.
 * Operations : Add, Delete, Update, Search, Display
 * Storage    : Records are kept in a binary file (students.dat) using
 *              structures + fread/fwrite, so data survives after the
 *              program closes.
 *
 * Compile: gcc -Wall -Wextra -o student student_management.c
 * Run    : ./student        (Windows: student.exe)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#define DATA_FILE   "students.dat"
#define TEMP_FILE   "students.tmp"
#define NAME_LEN    50
#define COURSE_LEN  30
#define MAX_ROLL    999999

/* One student record. This struct is written to / read from the file. */
typedef struct {
    int   roll;
    char  name[NAME_LEN];
    char  course[COURSE_LEN];
    float marks;
} Student;

/* ------------------------------------------------------------------ */
/*  Input helpers                                                      */
/* ------------------------------------------------------------------ */

/* Removes leading and trailing whitespace from a string (in place). */
static void trim(char *s)
{
    size_t len = strlen(s);
    size_t start = 0;

    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    while (s[start] != '\0' && isspace((unsigned char)s[start])) {
        start++;
    }
    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

/* Prints a prompt and reads one trimmed line (at most size-1 characters). */
static void readLine(const char *prompt, char *buf, int size)
{
    size_t len;
    int c;

    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput ended. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        /* line was longer than the buffer: throw away the rest of it */
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    trim(buf);
}

/* Reads a line that must not be empty. */
static void readNonEmpty(const char *prompt, char *buf, int size)
{
    for (;;) {
        readLine(prompt, buf, size);
        if (buf[0] != '\0') {
            return;
        }
        printf("  This field cannot be empty.\n");
    }
}

/* Parses a whole string as a long. Returns 1 on success. */
static int parseLong(const char *s, long *out)
{
    char *end;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || errno == ERANGE) {
        return 0;
    }
    *out = v;
    return 1;
}

/* Parses a whole string as a finite double. Returns 1 on success. */
static int parseDouble(const char *s, double *out)
{
    char *end;
    double v;

    errno = 0;
    v = strtod(s, &end);
    if (end == s || *end != '\0' || errno == ERANGE || !isfinite(v)) {
        return 0;
    }
    *out = v;
    return 1;
}

/* Reads an integer in [min, max]. */
static int readInt(const char *prompt, int min, int max)
{
    char line[64];
    long v;

    for (;;) {
        readLine(prompt, line, sizeof line);
        if (!parseLong(line, &v)) {
            printf("  Invalid input. Please enter a whole number.\n");
        } else if (v < min || v > max) {
            printf("  Please enter a value between %d and %d.\n", min, max);
        } else {
            return (int)v;
        }
    }
}

/* Reads a decimal number in [min, max]. */
static double readDouble(const char *prompt, double min, double max)
{
    char line[64];
    double v;

    for (;;) {
        readLine(prompt, line, sizeof line);
        if (!parseDouble(line, &v)) {
            printf("  Invalid input. Please enter a number.\n");
        } else if (v < min || v > max) {
            printf("  Please enter a value between %g and %g.\n", min, max);
        } else {
            return v;
        }
    }
}

/* Asks a yes/no question. Returns 1 for yes, 0 for no. */
static int askYesNo(const char *prompt)
{
    char line[64];

    for (;;) {
        readLine(prompt, line, sizeof line);
        if (line[0] == 'y' || line[0] == 'Y') {
            return 1;
        }
        if (line[0] == 'n' || line[0] == 'N') {
            return 0;
        }
        printf("  Please answer with y or n.\n");
    }
}

/* Case-insensitive "does text contain pattern?" */
static int containsIgnoreCase(const char *text, const char *pattern)
{
    size_t n = strlen(text);
    size_t m = strlen(pattern);
    size_t i, j;

    if (m == 0) {
        return 1;
    }
    for (i = 0; i + m <= n; i++) {
        for (j = 0; j < m; j++) {
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)pattern[j])) {
                break;
            }
        }
        if (j == m) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Display helpers                                                    */
/* ------------------------------------------------------------------ */

static const char *getGrade(float marks)
{
    if (marks >= 90) return "A+";
    if (marks >= 80) return "A";
    if (marks >= 70) return "B";
    if (marks >= 60) return "C";
    if (marks >= 50) return "D";
    return "F";
}

static void printHeader(void)
{
    printf("\n%-8s %-25s %-20s %-8s %-5s\n", "Roll No", "Name", "Course", "Marks", "Grade");
    printf("-------- ------------------------- -------------------- -------- -----\n");
}

static void printStudent(const Student *s)
{
    printf("%-8d %-25.25s %-20.20s %-8.2f %-5s\n",
           s->roll, s->name, s->course, s->marks, getGrade(s->marks));
}

/* ------------------------------------------------------------------ */
/*  File helper                                                        */
/* ------------------------------------------------------------------ */

/* Looks for a roll number in the file. Returns 1 if found.
   'out' may be NULL if the caller only needs to know whether it exists. */
static int findStudent(int roll, Student *out)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    Student s;
    int found = 0;

    if (fp == NULL) {
        return 0;               /* file does not exist yet: no records */
    }
    while (fread(&s, sizeof s, 1, fp) == 1) {
        if (s.roll == roll) {
            if (out != NULL) {
                *out = s;
            }
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

/* ------------------------------------------------------------------ */
/*  Features                                                           */
/* ------------------------------------------------------------------ */

static void addStudent(void)
{
    Student s;
    FILE *fp;

    printf("\n--- Add Student ---\n");
    memset(&s, 0, sizeof s);

    s.roll = readInt("Enter roll number: ", 1, MAX_ROLL);
    if (findStudent(s.roll, NULL)) {
        printf("A student with roll number %d already exists.\n", s.roll);
        return;
    }
    readNonEmpty("Enter name: ", s.name, sizeof s.name);
    readNonEmpty("Enter course / branch: ", s.course, sizeof s.course);
    s.marks = (float)readDouble("Enter marks (0-100): ", 0, 100);

    fp = fopen(DATA_FILE, "ab");
    if (fp == NULL) {
        perror("Error opening data file");
        return;
    }
    if (fwrite(&s, sizeof s, 1, fp) != 1) {
        printf("Error: could not save the record.\n");
    } else {
        printf("Student added successfully.\n");
    }
    fclose(fp);
}

static void displayStudents(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    Student s;
    int count = 0;

    printf("\n--- All Students ---\n");
    if (fp == NULL) {
        printf("No records found.\n");
        return;
    }

    printHeader();
    while (fread(&s, sizeof s, 1, fp) == 1) {
        printStudent(&s);
        count++;
    }
    fclose(fp);

    if (count == 0) {
        printf("No records found.\n");
    } else {
        printf("\nTotal students: %d\n", count);
    }
}

static void searchStudent(void)
{
    int option;

    printf("\n--- Search Student ---\n");
    printf("1. Search by roll number\n");
    printf("2. Search by name\n");
    option = readInt("Enter your choice (1-2): ", 1, 2);

    if (option == 1) {
        Student s;
        int roll = readInt("Enter roll number: ", 1, MAX_ROLL);

        if (findStudent(roll, &s)) {
            printHeader();
            printStudent(&s);
        } else {
            printf("Student with roll number %d not found.\n", roll);
        }
    } else {
        char query[NAME_LEN];
        FILE *fp;
        Student s;
        int matches = 0;

        readNonEmpty("Enter name (or part of it): ", query, sizeof query);
        fp = fopen(DATA_FILE, "rb");
        if (fp == NULL) {
            printf("No records found.\n");
            return;
        }
        while (fread(&s, sizeof s, 1, fp) == 1) {
            if (containsIgnoreCase(s.name, query)) {
                if (matches == 0) {
                    printHeader();
                }
                printStudent(&s);
                matches++;
            }
        }
        fclose(fp);
        if (matches == 0) {
            printf("No student found matching \"%s\".\n", query);
        } else {
            printf("\n%d match(es) found.\n", matches);
        }
    }
}

static void updateStudent(void)
{
    FILE *fp;
    Student s;
    char prompt[128];
    char buf[128];
    char text[NAME_LEN];
    char courseText[COURSE_LEN];
    int roll, found = 0;

    printf("\n--- Update Student ---\n");
    roll = readInt("Enter roll number to update: ", 1, MAX_ROLL);

    fp = fopen(DATA_FILE, "r+b");
    if (fp == NULL) {
        printf("No records found.\n");
        return;
    }
    while (fread(&s, sizeof s, 1, fp) == 1) {
        if (s.roll == roll) {
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Student with roll number %d not found.\n", roll);
        fclose(fp);
        return;
    }

    printf("\nCurrent record:");
    printHeader();
    printStudent(&s);
    printf("\nEnter new details (press Enter to keep the current value).\n");

    snprintf(prompt, sizeof prompt, "New name   [%s]: ", s.name);
    readLine(prompt, text, sizeof text);
    if (text[0] != '\0') {
        snprintf(s.name, sizeof s.name, "%s", text);
    }

    snprintf(prompt, sizeof prompt, "New course [%s]: ", s.course);
    readLine(prompt, courseText, sizeof courseText);
    if (courseText[0] != '\0') {
        snprintf(s.course, sizeof s.course, "%s", courseText);
    }

    snprintf(prompt, sizeof prompt, "New marks  [%.2f]: ", s.marks);
    for (;;) {
        double m;

        readLine(prompt, buf, sizeof buf);
        if (buf[0] == '\0') {
            break;                              /* keep current marks */
        }
        if (parseDouble(buf, &m) && m >= 0 && m <= 100) {
            s.marks = (float)m;
            break;
        }
        printf("  Invalid marks. Enter a number from 0 to 100.\n");
    }

    /* Move back to the start of this record and overwrite it. */
    if (fseek(fp, -(long)sizeof s, SEEK_CUR) != 0 || fwrite(&s, sizeof s, 1, fp) != 1) {
        printf("Error: could not update the record.\n");
    } else {
        printf("Record updated successfully.\n");
    }
    fclose(fp);
}

static void deleteStudent(void)
{
    FILE *in, *out;
    Student s, t;
    int roll;

    printf("\n--- Delete Student ---\n");
    roll = readInt("Enter roll number to delete: ", 1, MAX_ROLL);

    if (!findStudent(roll, &s)) {
        printf("Student with roll number %d not found.\n", roll);
        return;
    }

    printHeader();
    printStudent(&s);
    if (!askYesNo("\nAre you sure you want to delete this record? (y/n): ")) {
        printf("Deletion cancelled.\n");
        return;
    }

    /* Copy every record except the one being deleted into a temp file,
       then replace the original file with the temp file. */
    in  = fopen(DATA_FILE, "rb");
    out = fopen(TEMP_FILE, "wb");
    if (in == NULL || out == NULL) {
        perror("Error opening file");
        if (in != NULL)  fclose(in);
        if (out != NULL) fclose(out);
        return;
    }
    while (fread(&t, sizeof t, 1, in) == 1) {
        if (t.roll != roll) {
            fwrite(&t, sizeof t, 1, out);
        }
    }
    fclose(in);
    fclose(out);

    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        printf("Error: could not finish deleting the record.\n");
    } else {
        printf("Record deleted successfully.\n");
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    int choice;

    printf("=====================================\n");
    printf("     STUDENT MANAGEMENT SYSTEM\n");
    printf("   CodeAlpha C Programming - Task 3\n");
    printf("=====================================\n");

    do {
        printf("\n1. Add Student\n");
        printf("2. Delete Student\n");
        printf("3. Update Student\n");
        printf("4. Search Student\n");
        printf("5. Display All Students\n");
        printf("6. Exit\n");
        choice = readInt("Enter your choice (1-6): ", 1, 6);

        switch (choice) {
        case 1: addStudent();      break;
        case 2: deleteStudent();   break;
        case 3: updateStudent();   break;
        case 4: searchStudent();   break;
        case 5: displayStudents(); break;
        case 6: printf("\nGoodbye! All records are saved in %s.\n", DATA_FILE); break;
        }
    } while (choice != 6);

    return 0;
}
