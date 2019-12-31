#ifndef SHELL_DATASTRUCTURE_H
#define SHELL_DATASTRUCTURE_H
typedef struct SimpleCommand
{
    char **command;
    int available_arg_space;
    int current_argc;
} SimpleCommand_t;

typedef struct ParserHelper
{
    char **segment;
    int num;
} ParserHelper_t;

typedef struct BackgroundStack
{
    char **background_commands;
    int **pid;
    int *pid_size;
    int *status;
    int num;
} BackgroundStack_t;

typedef struct CommandTable
{
    struct SimpleCommand **commands;
    int background;
    char *in_file;
    char *append_file;
    char *out_file;
    int available_sc_space; // available simple command space
    int current_sc; // current sc space
} CommandTable_t;

#endif //SHELL_DATASTRUCTURE_H
