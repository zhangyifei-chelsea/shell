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
#include <setjmp.h>

#include "datastructure.h"
#include "parser.h"
#include "executer.h"

#define COMMAND_SIZE 3073  // maximum command size is 1024, adding space will triple it, adding \0 makes it to 3073


typedef enum
{
    false, true
} bool;


static sigjmp_buf env;
void sigint_handler_1()
{
    siglongjmp(env, 42);
}

void sigint_handler_2()
{
// empty handler
}

int CheckValid(const char *inputLine)
{
    // return 2 if valid, 1 if not complete "", 0 if no complete filename or command
    // incomplete quote has higher priority than others
    int position = 0;
    int quote_flag = 0;
    while (position < (int) strlen(inputLine))
    {
        if (inputLine[position] == '\'')
            quote_flag = 1;
        else if (inputLine[position] == '\"')
            quote_flag = 2;
        if (quote_flag)
        {
            position++;
            position = FindNextQuote(inputLine, position, quote_flag);
            if (inputLine[position] != '\0')
                quote_flag = 0;
        }
        position++;
    }
    if (quote_flag)
        return 1; // if quote incomplete, satisfy the situation with quote

    // check whether end with > < |
    position = strlen(inputLine) - 1;
    while (inputLine[position] == ' ')
    {
        position--;
    }
    if (inputLine[position] == '>' || inputLine[position] == '<' || inputLine[position] == '|')
        return 0;
    // when valid
    return 2;
}

void CheckSyntex(const char* inputLine, char* syntex)
{
    int position=0;
    int quote_flag=0;
    int len=strlen(inputLine)+1;
    while(position<len)
    {
        if (inputLine[position] == '\'')
            quote_flag = 1;
        else if (inputLine[position] == '\"')
            quote_flag = 2;
        if(quote_flag)
        {
            position++;
            position=FindNextQuote(inputLine,position,quote_flag);
            position++;
            quote_flag=0;
        }
        else
        {
            if(inputLine[position] == '<')
            {
                int next=position+1;
                next=FindNonSpace(inputLine,next);
                if(next==-1)
                    return;
                if(inputLine[next] == '<'||inputLine[next] == '>'||inputLine[next] == '|')
                {
                    syntex[0]=inputLine[next];
                    syntex[1]='\0';
                    return;
                }
            }
            else if(inputLine[position] == '>')
            {
                int next=position+1;
                if(inputLine[next] == '>')
                {
                    position++;
                    continue;
                }
                next=FindNonSpace(inputLine,next);
                if(next==-1)
                    return;
                if(inputLine[next] == '<'||inputLine[next] == '>'||inputLine[next] == '|')
                {
                    syntex[0]=inputLine[next];
                    syntex[1]='\0';
                    return;
                }
            }
            position++;
        }
    }
}



int main()
{
    char *inputLine = malloc(COMMAND_SIZE * sizeof(char));
    char *inputBuffer = malloc(COMMAND_NUM * sizeof(char));
    BackgroundStack_t backgroundCmd;
    initBackgroundCmd(&backgroundCmd);
    while (true)
    {
        if (sigsetjmp(env, 1) == 42) {
            continue;
        }
        printf("sh $ ");
        fflush(stdout);
        int tmp_length = 0;

        inputLine[0] = '\0';
        inputBuffer[0] = '\0';
        // deal with ctrl C when input is not complete
        signal(SIGINT, sigint_handler_1);
        int syntex_flag=0;
        while (true)
        {

            inputBuffer[0] = '\0';
            fgets(inputBuffer, COMMAND_SIZE, stdin);
            int buffer_len = strlen(inputBuffer);
            if (buffer_len == 0 && strlen(inputLine) == 0 && feof(stdin))
            {
                // single ctrl D exit
                printf("exit\n");
                fflush(stdout);
                free(inputBuffer);
                free(inputLine);
                exit(0);
            }
            else if (buffer_len > 0 && inputBuffer[buffer_len - 1] != '\n' && feof(stdin))
            {
                // ctrl D while there is something in the command line for the first time
                memcpy(inputLine + tmp_length, inputBuffer, strlen(inputBuffer));
                tmp_length += strlen(inputBuffer);
                continue;
            }
            else if (buffer_len > 0 && inputBuffer[buffer_len - 1] == '\n')
            {
                // press enter
                memcpy(inputLine + tmp_length, inputBuffer, strlen(inputBuffer) - 1);
                tmp_length += strlen(inputBuffer) - 1;
                inputLine[tmp_length] = '\0';
                if (strlen(inputLine) > 0)
                {
                    // check syntex error
                    char* syntex=malloc(2* sizeof(char));
                    syntex[0]='\0';
                    CheckSyntex(inputLine, syntex);
                    if(syntex[0]!='\0')
                    {
                        printf("syntax error near unexpected token `%s'\n",syntex);
                        fflush(stdout);
                        syntex_flag=1;
                        free(syntex);
                        break;
                    }
                    free(syntex);

                    // check whether the input is complete
                    int valid = CheckValid(inputLine);
                    if (valid==2)
                        break;
                    else
                    {
                        printf("> ");
                        fflush(stdout);
                        if(valid==1)
                        {
                            // has incomplete quote
                            inputLine[tmp_length] = '\n';
                            tmp_length++;
                            inputLine[tmp_length] = '\0';
                        }
                    }
                }
                else
                    break;
            }
            else
            {
                // continuing multiple ctrl D while there is something in the command line
                continue;
            }
        }
        if(syntex_flag)
        {
            continue;
        }
        signal(SIGINT, sigint_handler_2);
        int length = strlen(inputLine);
        inputLine[length] = '\0';   // add necessary \0
        if (length >= 1025)
        {
            printf("Command too long. \n");
            fflush(stdout);
            continue;
        }
        else if (length==0)
            continue;

        // start parsing
        CommandTable_t command_table;
        initCommandTable(&command_table);

        ParserHelper_t parser_helper;
        initParserHelper(&parser_helper);

        int parse_state=Parse(inputLine, &parser_helper, &command_table, &backgroundCmd);
        freeParserHelper(&parser_helper);
        // check for missing program error
        int missing_flag=0;
        for(int i=0;i<command_table.current_sc;++i)
        {
            if(command_table.commands[i]->current_argc==0)
            {
                printf("error: missing program\n");
                fflush(stdout);
                freeCommandTable(&command_table);  // hey final free is here!
                missing_flag=1;
                break;
            }
        }
        if(missing_flag)
            continue;

        if(parse_state!=0)
        {
            if(parse_state==5)
            {
                printf("error: duplicated output redirection\n");
                fflush(stdout);
            }
            else if(parse_state==4)
            {
                printf("error: duplicated input redirection\n");
                fflush(stdout);
            }
            freeCommandTable(&command_table);  // hey final free is here!
            continue;
        }

        int* my_child = malloc(command_table.current_sc * sizeof(int));

        // start execute
        int return_status = Execute(&command_table, my_child, &backgroundCmd);

        freeCommandTable(&command_table);  // hey final free is here!
        free(my_child);
        if (return_status == 1)
        {
            printf("exit\n");
            fflush(stdout);
            break;
        }
    }
    free(inputBuffer);
    free(inputLine);
    freeBackgroundCmd(&backgroundCmd);
    return 0;
}
