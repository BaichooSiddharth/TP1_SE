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

enum op whichOp(char *symbol) {
    enum op result;
    if (symbol == NULL)
        result = NONE;
    else if (strcmp(symbol, "&&") == 0)
        result = AND;
    else if (strcmp(symbol, "||") == 0)
        result = OR;
    else if (strcmp(symbol, "&") == 0)
        result = ALSO;
    else
        result = BIDON;
    return result;
}

// echo a && ls -a -l && pwd

// commands = {cm1, cm2}
// cmd1:
    // call = {"echo", "a"}
    // operator = "&&"
    // next = cmd2
    // count = 2
    // also = ????
// cmd2
    // call = {"ls", "-a", "-l"}
    // operator = NULL
    // next = NULL
    // count = 3
    // also = ???


struct command {
    char **call;
    enum op operator;
    struct command *next;
    int count;
    bool also;
};

struct command *new_node(char** n_call, enum op n_op, int n_count, bool n_also) {
    struct command *node = malloc(sizeof(struct command));
    node->call = n_call;
    node->operator = n_op;
    node->count = n_count;
    node->also = n_also;
    node->next = NULL; //bonne pratique!
    return node;
}

struct command *free_node(struct command *node) {
    struct command *next = node->next;
    free(node);
    return next;
}

void printCommands(struct command *head) {
    printf("PRINTING COMMAND%c", '\n');
    int i;
    char **curCall;
    const char *enumNames[] = {"BIDON", "NONE", "OR", "AND", "ALSO"};

    while (head != NULL) {
        curCall = head->call;
        i = 0;
        printf("%i words in call: \n", head->count);
        while(i < head->count)
            printf("%s\n", curCall[i++]);
        printf("end of first command%c", '\n');

        printf("ENUM: %s\n", enumNames[head->operator]);
        printf("ALSO: %i\n", head->also);
        head = head->next;
    }
}

void free_l_list(struct command *head) {
    while (head != NULL) {
        struct command *next = head->next;
        free(head);
        head = next;
    }
}


void freeStringArray(char **arr) {
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr); 
}

error_code readline(char **out) {
    size_t size = 8;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    if (line == NULL)
        return ERROR; // TODO: don't return error on empty input

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

char **parseWords(char *line, int *numWords) {
    int len = (int) strlen(line);
    char **words = malloc(sizeof(char*) * len + 1);
    char *word = malloc(sizeof(char) * len + 1);
    char cur;
    int j = 0;
    int k = 0;

    for (int i = 0; i <= len; i++) {
        cur = line[i];
        if (cur == ' ' || cur == '\0') {
            if (strlen(word) > 0) {
                words[j] = word;
                ++j;
                word = malloc(sizeof(char) * len + 1);
                k = 0;
            }
        } else {
            word[k] = cur;
            ++k;
        }
    }
    words[j] = NULL;
    *numWords = j;

    return words;
}

struct command *parseLine(char *line) {
    // 3. mettre chaque mot dans la struct command
    // 4. comme c'est une liste chainée, retourner le 1er élément
    int numWords;
    char **words = parseWords(line, &numWords);

    char **currentCall = malloc(sizeof(char*) * numWords);
    int j = 0;
    char *w = malloc(sizeof(char) * numWords);
    enum op symbol;
    struct command *currentNode = NULL;
    struct command *firstNode;
    struct command *nextNode;

    // i is the current word index in words
    // j is the current word index in currentCall
    for (int i = 0; i < numWords; i++) {
        w = words[i];
        symbol = whichOp(w);

        if (symbol == BIDON) {
            currentCall[j] = w;
            ++j;
        }
        if (symbol != BIDON || i == numWords - 1) {
            nextNode = new_node(currentCall, symbol, j, symbol == ALSO);
            if (!currentNode) {
                currentNode = nextNode;
                firstNode = currentNode;
            } else {
                currentNode->next = nextNode;
                currentNode = nextNode;
            }
            j = 0;
            currentCall = malloc(sizeof(char*) * numWords);
        }
    }
    return firstNode;
}

int runLine(struct command *firstNode) {
//    if (firstNode->count == 0)
//        return 0;

    //printCommands(firstNode);



    pid_t pid;

    // while(head): check le résultat du précédent call.
    // en fonction du exit code, run ou ne run pas le || ou le &&
    // pis si &, run en background.

    pid = fork();
    if (pid == 0) {
        char *file = *firstNode->call;
        error_code e = execvp(file, firstNode->call);
        printf("encountered error %i\n", e);
        exit(0);
    } else
        wait(&pid);

    return 0;
}

int main (void) {
    char *line;
    struct command *commandFirstNode;

    // executes line
    while (!HAS_ERROR(readline(&line)) && strcmp(line, "exit") != 0) {
        commandFirstNode = parseLine(line);
        runLine(commandFirstNode);
    }
    free(line);
    exit(0);
}