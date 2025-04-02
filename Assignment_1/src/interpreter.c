#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int MAX_ARGS_SIZE = 3;

int badcommand () {
    printf ("Unknown Command\n");
    return 1;
}

int badcommandGeneral (char *message) {
    printf ("Bad command: %s\n", message);
    return 2;
}

// For source command only
int badcommandFileDoesNotExist () {
    printf ("Bad command: File not found\n");
    return 3;
}

int help ();
int quit ();
int set (char *var, char *value);
int print (char *var);
int source (char *script);
int badcommandFileDoesNotExist ();
int echo (char *input);
int isAlNum (char c);
int my_ls ();
int compare (const void *a, const void *b);
int min (int a, int b);
int my_mkdir (char *dirname);
int my_touch (char *filename);
int my_cd (char *dirname);
int run (char *arguments[], int args_size);

// Interpret commands and their arguments
int interpreter (char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand ();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn (command_args[i], "\r\n")] = 0;
    }

    if (strcmp (command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand ();
        return help ();

    } else if (strcmp (command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand ();
        return quit ();

    } else if (strcmp (command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand ();
        return set (command_args[1], command_args[2]);

    } else if (strcmp (command_args[0], "print") == 0) {
        //print
        if (args_size != 2)
            return badcommand ();
        return print (command_args[1]);

    } else if (strcmp (command_args[0], "source") == 0) {
        //source
        if (args_size != 2)
            return badcommand ();
        return source (command_args[1]);

    } else if (strcmp (command_args[0], "echo") == 0) {
        //echo
        if (args_size != 2)
            return badcommand ();
        return echo (command_args[1]);
    } else if (strcmp (command_args[0], "my_ls") == 0) {
        //my_ls
        if (args_size != 1)
            return badcommand ();
        return my_ls ();
    } else if (strcmp (command_args[0], "my_mkdir") == 0) {
        //my_mkdir
        if (args_size != 2)
            return badcommand ();
        return my_mkdir (command_args[1]);
    } else if (strcmp (command_args[0], "my_touch") == 0) {
        //my_touch
        if (args_size != 2)
            return badcommand ();
        return my_touch (command_args[1]);
    } else if (strcmp (command_args[0], "my_cd") == 0) {
        //my_cd
        if (args_size != 2)
            return badcommand ();
        return my_cd (command_args[1]);
    } else if (strcmp (command_args[0], "run") == 0) {
        //run
        return run ((command_args + 1), (args_size - 1));
    } else {
        return badcommand ();
    }
}

int help () {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf ("%s\n", help_string);
    return 0;
}

int quit () {
    printf ("Bye!\n");
    exit (0);
}

int set (char *var, char *value) {
    char *link = " ";

    // Hint: If "value" contains multiple tokens, you'll need to loop through them, 
    // concatenate each token to the buffer, and handle spacing appropriately. 
    // Investigate how `strcat` works and how you can use it effectively here.

    mem_set_value (var, value);
    return 0;
}

int print (char *var) {
    printf ("%s\n", mem_get_value (var));
    return 0;
}

int source (char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen (script, "rt");     // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist ();
    }

    fgets (line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput (line);    // which calls interpreter()
        memset (line, 0, sizeof (line));

        if (feof (p)) {
            break;
        }
        fgets (line, MAX_USER_INPUT - 1, p);
    }

    fclose (p);

    return errCode;
}

int echo (char *input) {
    int isVar = 0;              // indicates if the input is a variable in memory 

    // Check if the input is a variable (starts with '$')
    if (input[0] == '$')
        isVar = 1;

    // Handle variable lookup
    if (isVar) {
        char *varName = input + sizeof (char);  // Skip '$' and get variable name
        char *value = mem_get_value (varName);

        if (strcmp (value, "Variable does not exist") == 0) {
            printf ("\n");      // Print an empty line if the variable is not found
        } else {
            printf ("%s\n", value);
        }
    } else {
        // Validate input: must be alphanumeric
        for (int i = 0; input[i] != '\0'; i++) {
            if (!isAlNum (input[i])) {  // Reject invalid characters
                return
                    badcommandGeneral
                    ("echo expects an alphanumeric message input");
            }
        }
        printf ("%s\n", input); // Print regular input
    }

    return 0;
}

// checks if character is alphanumeric
int isAlNum (char c) {
    return (c >= 'A' && c <= 'Z') ||    // Uppercase letters
        (c >= 'a' && c <= 'z') ||       // Lowercase letters
        (c >= '0' && c <= '9'); // Digits
}

int my_ls () {
    struct dirent *entry;
    DIR *dir = opendir (".");

    // count files
    int num = 0;
    while ((entry = readdir (dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;           // Skip hidden files
        num++;
    }

    if (num == 0) {
        closedir (dir);
        printf (".\n..\n");
        return 0;
    }
    // Allocate memory for file names
    char **names = malloc (num * sizeof (char *));

    // Rewind and read files into array
    rewinddir (dir);
    int i = 0;
    while ((entry = readdir (dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        names[i++] = strdup (entry->d_name);
    }

    closedir (dir);

    // Sort file names using custom compare function
    qsort (names, num, sizeof (char *), compare);

    printf (".\n..\n");
    // Print sorted names
    for (int i = 0; i < num; i++) {
        printf ("%s\n", names[i]);
        free (names[i]);
    }
    free (names);
    return 1;
}

int compare (const void *a, const void *b) {
    const char *str1 = *(const char **) a;
    const char *str2 = *(const char **) b;

    int min_length = min (strlen (str1), strlen (str2));

    for (int i = 0; i < min_length; i++) {
        char c1 = str1[i];
        char c2 = str2[i];
        // if they are the same no need for the rest of the computation, move to next character
        if (c1 == c2)
            continue;

        // both digits
        if (isdigit (c1) && isdigit (c2))
            return (c1 - '0') - (c2 - '0');
        // c1 is a number, c2 is a letter
        if (isdigit (c1))
            return -1;
        // c1 is a letter, c2 is a number
        if (isdigit (c2))
            return 1;
        // both letters 
        char lower1 = tolower (c1);
        char lower2 = tolower (c2);
        // Alphabetical sorting
        if (lower1 != lower2)
            return lower1 - lower2;
        // if they're the same letter
        return c2 - c1;         // Uppercase has a smaller ASCII value, so it comes first
    }
    // pick shorter if all characters are the same
    return strlen (str1) - strlen (str2);
}

int min (int a, int b) {
    return (a < b) ? a : b;
}

int my_mkdir (char *dirname) {
    int isVar = 0;              // indicates if the input is a variable in memory 

    // Check if the input is a variable (starts with '$')
    if (dirname[0] == '$')
        isVar = 1;

    // Handle variable lookup
    if (isVar) {
        char *varName = dirname + 1;    // Skip '$' and get variable name
        char *value = mem_get_value (varName);

        if (strcmp (value, "Variable does not exist") == 0)
            return badcommandGeneral ("my_mkdir");
        // Validate input: must be alphanumeric
        for (int i = 0; value[i] != '\0'; i++) {
            if (!isAlNum (value[i])) {  // Reject invalid characters
                return badcommandGeneral ("my_mkdir");
            }
        }
        return mkdir (value, 0777);

    } else {
        // Validate input: must be alphanumeric
        for (int i = 0; dirname[i] != '\0'; i++) {
            if (!isAlNum (dirname[i])) {        // Reject invalid characters
                return badcommandGeneral ("my_mkdir");
            }
        }
        // input is an alphanumeric string
        return mkdir (dirname, 0777);
    }
}

int my_touch (char *filename) {
    // Validate input: must be alphanumeric
    for (int i = 0; filename[i] != '\0'; i++) {
        if (!isAlNum (filename[i])) {   // Reject invalid characters
            return
                badcommandGeneral
                ("my_touch expected an alphanumeric filename");
        }
    }
    // input is an alphanumeric string
    FILE *file = fopen (filename, "w");
    if (file) {
        fclose (file);          // Close the file to save changes
        return 0;
    } else {
        return
            badcommandGeneral
            ("my_touch could not create a file with the given name");
    }
}

int my_cd (char *dirname) {
    for (int i = 0; dirname[i] != '\0'; i++) {
        if (!isAlNum (dirname[i])) {    // Reject invalid characters
            return
                badcommandGeneral
                ("my_cd expected an alphanumeric directory name");
        }
    }

    // Check if dirname exists and is a directory
    struct stat st;
    if (stat (dirname, &st) != 0 || !S_ISDIR (st.st_mode))
        return badcommandGeneral ("my_cd");;

    // Attempt to change the directory
    if (chdir (dirname) != 0) {
        badcommandGeneral ("my_cd could not move into the given directory");
    }
    return 1;
}

int run (char *arguments[], int args_size) {
    pid_t pid;
    fflush (stdout);
    if ((pid = fork ())) {
        // parent code
        wait (NULL);
    } else {
        // child code 
        execvp (arguments[0], arguments);
    }
    return 0;
}
