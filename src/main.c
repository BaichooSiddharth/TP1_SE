#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/wait.h>

#define true 1
#define false 0
#define bool int

typedef int error_code;

#define ERROR (-1)
#define HAS_ERROR(code) ((code) < 0)
#define NULL_TERMINATOR '\0'

enum op {   //todo custom shell operators. might want to use them to represent &&, ||, & and "no operator"
    BIDON, NONE, OR, AND, ALSO    //BIDON is just to make NONE=1, BIDON is unused
};

struct command {    //todo might want to use this to represent commands found on line
    char **call;
    enum op operator;
    struct command *next;
    int count;
    bool also;
};
//hint hint: this looks suspiciously similar to a linked list we saw in the demo. I wonder if I could use the same ideas here??

void freeStringArray(char **arr) {  //todo probably add this to free the "call" parameter inside of command
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr); 
}

error_code readline(char **out) {
    size_t size = 2;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    if (line == NULL)
        return ERROR;   // if we can't, terminate because of a memory issue

    char ch;
    int at = 0;

    while ((ch = (char) getchar()) != '\n') {
        line[at] = ch; // sets ch at the current index and increments the index
        ++at;
        if (at >= size) {
            size *= 2;
            line = realloc(line, sizeof(char) * size);
        }
    }

    line[at] = NULL_TERMINATOR;    // finish the line with return 0
    out[0] = line;
    return 0;
}

int execute(char *line, char **args) {
    char *arg0 = line;
    char *arg1 = NULL;
    args[0] = arg0;
    args[1] = arg1;

    do execvp(line, args);
    while (strcmp(line, "exit") != 0);

    return 0;
}

error_code parseLine(char *line) {

    // 1. parcourir la ligne, caractère par caractère
    // 2. printer les mots 1 à la fois
    // 3. mettre chaque mot dans la struct
    // 4. comme c'est une liste chainée, retourner le 1er élément

    printf("%s\n", line);
    return 0;
}

int main (void) {
    char *line;
    char **args = malloc(sizeof(char *) * 3);

    // reads line
    if (HAS_ERROR(readline(&line)))
        return ERROR;

    parseLine(line);

    // executes line
    execute(line, args);

    free(line);
    free(args);
    return 0;
}
