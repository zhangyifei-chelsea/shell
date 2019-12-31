#include "executer.h"

int cd(const char *address)
{
    return chdir(address);
}

int Execute(const CommandTable_t *commandTable, int* my_child, BackgroundStack_t* backgroundcmd)
{
    // return 1 means exit, return 0 is normal, return -1 is error state
    // set default
    int default_in = dup(0);
    int default_out = dup(1);
    int childcount = 0;
    int fd0;
    int fd1;
    if (commandTable->in_file)
    {
        fd0 = open(commandTable->in_file, O_RDONLY, 0);
        if(fd0==-1)
        {
            printf("%s: No such file or directory\n", commandTable->in_file);
            fflush(stdout);
            return -1;
        }
    }
    else
        fd0 = dup(default_in);
    int pid;
    for (int i = 0; i < commandTable->current_sc; ++i)
    {
        dup2(fd0, STDIN_FILENO);
        close(fd0);
        if (i == commandTable->current_sc - 1)
        {
            if (commandTable->out_file && !commandTable->append_file)
            {
                fd1 = creat(commandTable->out_file, 0644);
                if(fd1==-1)
                {
                    printf("%s: Permission denied\n", commandTable->out_file);
                    fflush(stdout);
                    return -1;
                }
            }
            else if (commandTable->append_file && !commandTable->out_file)
            {
                fd1 = open(commandTable->append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if(fd1==-1)
                {
                    printf("%s: Permission denied\n", commandTable->append_file);
                    fflush(stdout);
                    return -1;
                }
            }
            else
            {
                fd1 = dup(default_out);
            }
        }
        else
        {
            int fdpipe[2];
            pipe(fdpipe);
            fd1 = fdpipe[1];
            fd0 = fdpipe[0];
        }
        dup2(fd1, STDOUT_FILENO);
        close(fd1);
        SimpleCommand_t *current_cmd = commandTable->commands[i];
        if (strcmp(current_cmd->command[0], "exit") == 0)
            return 1;
        else if (strcmp(current_cmd->command[0], "cd") == 0)
        {
            char *address = current_cmd->command[1];
            int arg = current_cmd->current_argc;
            if (arg > 2)
                printf("bash: cd: too many arguments\n");
            else if (!address)
            {
                // if address is NULL, cd will do nothing!
                continue;
            }
            else
            {
                if (cd(address) < 0)
                {
                    printf("%s: No such file or directory\n",address);
                    fflush(stdout);
//                    perror(address);
                }
            }
        }
        else if (strcmp(current_cmd->command[0], "pwd") == 0)
        {
            char address[1025] = {0};
            char *result = getcwd(address, 1024);
            if (result != NULL)
            {
                printf("%s\n", address);
                fflush(stdout);
            }
        }
        else if (strcmp(current_cmd->command[0], "jobs") == 0)
        {
            int current_num=backgroundcmd->num;
            for(int c=0;c<current_num;++c)
            {
                backgroundcmd->status[c]=1; // means done
                for(int p=0;p<backgroundcmd->pid_size[c];++p)
                {
                    int state=waitpid(backgroundcmd->pid[c][p],NULL,WNOHANG);
                    if(state==0)
                        backgroundcmd->status[c]=0; // means it is running
                }
                if(backgroundcmd->status[c]==1)
                    printf("[%d] done %s\n", c+1, backgroundcmd->background_commands[c]);
                else
                    printf("[%d] running %s\n", c+1, backgroundcmd->background_commands[c]);
            }
        }
        else
        {
            childcount++;
            pid = fork();
            if (pid == 0)
            {
                // this is a child process
                execvp(current_cmd->command[0], current_cmd->command);
                printf("%s: command not found\n",current_cmd->command[0]);
                fflush(stdout);
                exit(0);
            }
            else
            {
                my_child[childcount-1]=pid;
            }
        }
    }
    dup2(default_in, 0);
    dup2(default_out, 1);
    close(default_in);
    close(default_out);

    if (!commandTable->background)
    {
        for (int i = 0; i <childcount;++i)
            waitpid(my_child[i], NULL, 0);

    }
    else
    {
        int id=backgroundcmd->num;
        printf("[%d] %s\n", id, backgroundcmd->background_commands[id-1]);
        backgroundcmd->pid_size[id-1]=childcount;
        backgroundcmd->pid[id-1]=malloc(childcount* sizeof(int));
        for(int i=0;i<childcount;++i)
        {
            backgroundcmd->pid[id-1][i]=my_child[i];
        }
    }

    return 0;
}
