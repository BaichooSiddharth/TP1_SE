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

void setNullStrings(char ***ptr, int n) {
    for (int i = 0; i < n; i++)
        ptr[0][i] = NULL;
}

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

//struct command *free_node(struct command *node) {
//    struct command *next = node->next;
//    free(node);
//    return next;
//}

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

void freeStringArray(char **arr) {
    //if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    //}
    free(arr); 
}

void free_node_list(struct command *head) {
    while (head != NULL) {
        struct command *next = head->next;
        if (head->call)
            freeStringArray(head->call);
        free(head);
        head = next;
    }
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
    char **words = malloc(sizeof(char*) * (len + 1));
    char *word = malloc(sizeof(char) * (len + 1));
    char cur;
    int j = 0;
    int k = 0;

    for (int i = 0; i <= len; i++) {
        cur = line[i];
        if (cur == ' ' || cur == '\0') {
            if (strlen(word) > 0) {
                word[k] = NULL_TERMINATOR;
                words[j] = word;
                ++j;
                if (cur != NULL_TERMINATOR) {
                    printf("%i\n", i);
                    word = malloc(sizeof(char) * (len + 1));
                }
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
    int numWords;
    char **words = parseWords(line, &numWords);
    char **currentCall = malloc(sizeof(char*) * (numWords + 1));
    if (!currentCall)
        exit(0);
    printf("numWords: %i\n", numWords);
    setNullStrings(&currentCall, numWords);

    int j = 0;
    char *w;
    enum op symbol;

    struct command *currentNode = NULL;
    struct command *firstNode;
    struct command *nextNode;

    // i is the current word index in words
    // j is the current word index in currentCall
    for (int i = 0; i < numWords; i++) {
        w = words[i];
        symbol = whichOp(w);

        if (symbol == BIDON) { // word is not a symbol
            currentCall[j] = w;
            ++j;
        }
        if (symbol != BIDON || i == numWords - 1) { //word is a symbol or end of line
            currentCall[j] = NULL;
            nextNode = new_node(currentCall, symbol, j, symbol == ALSO);

            if (!currentNode) { // current node is first node
                currentNode = nextNode;
                firstNode = currentNode;
            } else {
                currentNode->next = nextNode;
                currentNode = nextNode;
            }
            j = 0;
            if (i != numWords - 1) {
                currentCall = malloc(sizeof(char *) * (numWords + 1));
                if (!currentCall)
                    exit(0);
                setNullStrings(&currentCall, numWords);
            }
        }
    }
    free(words);
    return firstNode;
}

int runNode(struct command *head) {

    pid_t pid;
    int status;

    // while(head): check le résultat du précédent call.
    // en fonction du exit code, run ou ne run pas le || ou le &&
    // pis si &, run en background.

    pid = fork();
    if (pid == 0) {
        char *file = *head->call;
        error_code e = execvp(file, head->call);
        printf("encountered error %i\n", e);
//        free_node_list(head);
        exit(e);
    } else
        waitpid(pid, &status, 0);

    if (status == 0)
        while (head && head->operator != AND)
            head = head->next;
    else
        while (head && head->operator != OR)
            head = head->next;

    if (!head || !head->next) {
        status = 0;
        return 0;
    }
    else
        runNode(head->next);
}

bool checkIfLastAlso(struct command *node){
    if (node->next == NULL)
        return node->also;
    else
        return checkIfLastAlso(node->next);
}

int main (void) {
    char *line;
    struct command *commandFirstNode;

    pid_t pid;
    bool also;

    // executes lines until exit
    while (!HAS_ERROR(readline(&line)) && strcmp(line, "exit") != 0) {
        commandFirstNode = parseLine(line);
        also = checkIfLastAlso(commandFirstNode);

        if (also) {
            pid = fork();
            if (pid == 0) {
                runNode(commandFirstNode);
                free_node_list(commandFirstNode);
                free(line);
                exit(0);
            }
        } else {
            runNode(commandFirstNode);
            free_node_list(commandFirstNode);
            free(line);
        }
    }
    exit(0);
}