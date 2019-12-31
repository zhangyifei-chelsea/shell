#ifndef SHELL_EXECUTER_H
#define SHELL_EXECUTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "datastructure.h"

int Execute(const CommandTable_t* commandTable, int* my_child, BackgroundStack_t* backgroundcmd);

#endif //SHELL_EXECUTER_H
