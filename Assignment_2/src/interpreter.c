#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int badcommand () {
    printf ("Unknown Command\n");
    return 1;
}

int badCommandCustom (char *message) {
    fprintf (stderr, "Bad command: %s\n", message);
    return 1;
}

int help ();
int quit ();
int set (char *var, char *value);
int print (char *var);
int source (char *script);
int echo (char *input);
int isAlNum (char *string);
int my_ls ();
int compare (const void *a, const void *b);
int min (int a, int b);
int my_mkdir (char *dirname);
int my_touch (char *filename);
int my_cd (char *dirname);
int run (char *arguments[], int args_size);
int exec (char *arguments[], int args_size);

// Interpret commands and their arguments
int interpreter (char *command_args[], int args_size) {
    int i;

    if (args_size < 1) {
        badCommandCustom ("interpreter called with no arguments\n");
        exit (1);
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
        if (args_size < 2) {
            return badcommand ();
        }
        return run ((command_args + 1), (args_size - 1));
    } else if (strcmp (command_args[0], "exec") == 0) {
        //exec
        if (args_size < 3) {
            return badcommand ();
        }
        return exec ((command_args + 1), (args_size - 1));
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
    mem_set_value (var, value);
    return 0;
}

int print (char *var) {
    char *value = mem_get_value (var);
    if (value) {
        printf ("%s\n", value);
        free (value);
    } else {
        printf ("Variable does not exist\n");
    }
    return 0;
}

int source (char *filename) {
    int errCode = 0;

    // create PCB for this process
    errCode = addProcess (filename, "FCFS");
    errCode = runScheduler ("FCFS");

    return errCode;
}

int echo (char *input) {
    int mustFree = 0;

    // Check if the input is a variable (starts with '$')
    if (input[0] == '$') {
        input++;                // Skip '$'
        input = mem_get_value (input);

        if (strcmp (input, "Variable does not exist") == 0) {
            input = "";         // Print an empty line if the variable is not found
        } else {
            mustFree = 1;
        }
    }

    printf ("%s\n", input);

    if (mustFree) {
        free (input);
    }

    return 0;
}

// checks if a string is alphanumeric
int isAlNum (char *string) {
    for (char c = *string; c != '\0'; c = *++string) {
        if (!(isdigit (c) || isalpha (c)))
            return 0;
    }
    return 1;
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
            return
                badCommandCustom
                ("my_mkdir expected a variable name that exists");
        // Validate input: must be alphanumeric
        if (!isAlNum (value)) {
            return
                badCommandCustom
                ("my_mkdir expected a variable that stores an alphanumeric directory name");
        }
        return mkdir (value, 0777);

    } else {
        // Validate input: must be alphanumeric
        if (!isAlNum (dirname)) {
            return
                badCommandCustom
                ("my_mkdir expected an alphanumeric directory name");
        }
        // input is an alphanumeric string
        return mkdir (dirname, 0777);
    }
}

int my_touch (char *filename) {
    // Validate input: must be alphanumeric
    if (!isAlNum (filename)) {
        return badCommandCustom ("my_touch expected an alphanumeric filename");
    }
    // input is an alphanumeric string
    FILE *file = fopen (filename, "w");
    if (file) {
        fclose (file);          // Close the file to save changes
        return 0;
    } else {
        return
            badCommandCustom
            ("my_touch could not create a file with the given name");
    }
}

int my_cd (char *dirname) {
    if (!isAlNum (dirname)) {   // Reject invalid characters
        return
            badCommandCustom ("my_cd expected an alphanumeric directory name");
    }
    // Check if dirname exists and is a directory
    struct stat st;
    if (stat (dirname, &st) != 0 || !S_ISDIR (st.st_mode))
        return
            badCommandCustom ("my_cd expected an existing, valid directory");;

    // Attempt to change the directory
    if (chdir (dirname) != 0) {
        badCommandCustom ("my_cd could not move into the given directory");
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

int exec (char *arguments[], int args_size) {
    int errCode = 0;

    for (int i = 0; i < args_size - 1; i++) {
        for (int j = 0; j < args_size - 1; j++) {
            if (i != j && strcmp (arguments[i], arguments[j]) == 0) {
                badCommandCustom ("exec expected unique filenames");
                return 1;
            }
        }
    }

    if (firstBackground == 0) {
        if (backgroundMode == 0) {
            initReadyQueue ();
        }
        for (int i = 0; i < args_size - 1; i++) {
            errCode = addProcess (arguments[i], arguments[args_size - 1]);
            if (errCode != 0) {
                for (int j = 0; j < i; j++) {
                    freeScript (arguments[j]);
                }
                return 1;
            }
        }
        errCode = runScheduler (arguments[args_size - 1]);

        return errCode;
    } else if (firstBackground == 1) {
        for (int i = 0; i < args_size - 1; i++) {
            errCode = addProcess (arguments[i], arguments[args_size - 1]);
            if (errCode != 0) {
                for (int j = 0; j < i; j++) {
                    freeScript (arguments[j]);
                }
                return 1;
            }
        }
        errCode = addProcess (batchScript, arguments[args_size - 1]);
        if (errCode != 0) {
            freeScript (batchScript);
            return 1;
        }
        firstBackground = 0;
        errCode = runScheduler (arguments[args_size - 1]);
        return 0;
    }
    return 1;
}
