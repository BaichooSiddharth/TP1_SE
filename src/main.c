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
    int rTimes;
};

struct command *new_node(char** n_call, enum op n_op,
        int n_count, bool n_also, int rTimes) {
    struct command *node = malloc(sizeof(struct command));
    node->call = n_call;
    node->operator = n_op;
    node->count = n_count;
    node->also = n_also;
    node->next = NULL; //bonne pratique!
    node->rTimes = rTimes;
    return node;
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

void freeStringArray(char **arr) {
    //if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    //}
    free(arr); 
}

void free_node_list(struct command *head) {
    int i = 0;
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
        return ERROR;
    line[0] = '\0';

    char ch;
    int at = 0;

    while ((ch = (char) getchar()) != '\n') {
        line[at] = ch; // sets ch at the current index and increments the index
        ++at;
        if (at >= size) {
            size *= 2;
            line = realloc(line, sizeof(char) * size);
            if (!line) {
                printf("failed%c", '\n');
                exit(0);
            }
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
                    word = malloc(sizeof(char) * (len + 1));
                    word[0] = NULL_TERMINATOR;
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

bool isDigit(char c) {
    return c == '0' ||
    c == '1' ||
    c == '2' ||
    c == '3' ||
    c == '4' ||
    c == '5' ||
    c == '6' ||
    c == '7' ||
    c == '8' ||
    c == '9';
}

int check_rN(char **call, int numWords) {
    char *first = *call;
    int rN = 0;
    int i = 1;

    if (first[0] != 'r')
        return 1;
    else {
        char cur;
        while ((cur = first[i++]) != '(') {
            if (isDigit(cur)) {
                rN *= 10;
                rN += (int) cur - 48;
            } else
                return 1;
        }
    }
    char *lastWord = call[numWords - 1];
    int iLast = strlen(lastWord) - 1;
    char lastChar = lastWord[iLast];
    if (lastChar != ')')
        return 1;

    lastWord[iLast] = '\0';

    int j = 0;
    char cur;
    while((cur = call[0][j + i]) != '\0' && cur != ' ') {
        call[0][j++] = cur;
    }
    call[0][j] = NULL_TERMINATOR;

    return rN;

}

struct command *parseLine(char *line) {
    int numWords;
    char **words = parseWords(line, &numWords);
    char **currentCall = malloc(sizeof(char*) * (numWords + 1));
    if (!currentCall)
        exit(0);
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

            int rn = check_rN(currentCall, j);
            nextNode = new_node(currentCall, symbol, j, symbol == ALSO, rn);

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
        if (symbol != BIDON)
            free(w);
    }
    free(words);
    return firstNode;
}

int runNode(struct command *head) {

    pid_t pid;
    int status;

    for (int i = 0; i < head->rTimes; i++) {
        pid = fork();
        if (pid == 0) {
            char *file = *head->call;
            error_code e = execvp(file, head->call);
            if (e == -1)
                printf("%s: command not found\n", head->call[0]);
            freeStringArray(head->call);
            exit(e);
        } else
            waitpid(pid, &status, 0);
    }

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

    return 0; // will ne run
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