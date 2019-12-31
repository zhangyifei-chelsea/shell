#include "parser.h"


void initParserHelper(ParserHelper_t *parser_helper)
{
    parser_helper->segment = malloc(COMMAND_NUM * sizeof(char *));
    parser_helper->num = 0;
}

void freeParserHelper(ParserHelper_t *parser_helper)
{
    for (int i = 0; i < parser_helper->num; ++i)
    {
        free(parser_helper->segment[i]);
    }
    free(parser_helper->segment);
}

void initBackgroundCmd(BackgroundStack_t *backgroundcmd)
{
    backgroundcmd->num = 0;
    backgroundcmd->background_commands = malloc(BACKGROUND_COMMAND_NUM * sizeof(char *));
    backgroundcmd->status = malloc(BACKGROUND_COMMAND_NUM * sizeof(int));
    backgroundcmd->pid = malloc(BACKGROUND_COMMAND_NUM * sizeof(int *));
    backgroundcmd->pid_size = malloc(BACKGROUND_COMMAND_NUM * sizeof(int *));
}

void freeBackgroundCmd(BackgroundStack_t *backgroundcmd)
{
    for (int i = 0; i < backgroundcmd->num; ++i)
    {
        free(backgroundcmd->background_commands[i]);
    }
    free(backgroundcmd->background_commands);
    for (int i = 0; i < backgroundcmd->num; ++i)
    {
        free(backgroundcmd->pid[i]);
    }
    free(backgroundcmd->pid);
    free(backgroundcmd->pid_size);
    free(backgroundcmd->status);
}

void initCommandTable(CommandTable_t *command_table)
{
    command_table->commands = malloc(COMMAND_NUM * sizeof(struct SimpleCommand *));
    for (int i = 0; i < COMMAND_NUM; ++i)
        command_table->commands[i] = NULL;
    command_table->background = 0;
    command_table->in_file = NULL;
    command_table->out_file = NULL;
    command_table->append_file = NULL;
    command_table->available_sc_space = COMMAND_NUM;
    command_table->current_sc = 0;
}

void freeCommandTable(CommandTable_t *command_table)
{
    for (int i = 0; i < command_table->current_sc; ++i)
    {
        freeSimpleCommand(command_table->commands[i]);
        free(command_table->commands[i]);
    }
    free(command_table->commands);
    if (command_table->in_file)
        free(command_table->in_file);
    if (command_table->out_file)
        free(command_table->out_file);
    if (command_table->append_file)
        free(command_table->append_file);
    command_table->in_file = NULL;
    command_table->out_file = NULL;
    command_table->append_file = NULL;
}

void initSimpleCommand(SimpleCommand_t *single_command)
{
    single_command->available_arg_space = 1024;
    single_command->current_argc = 0;
    single_command->command = (char **) malloc(1024 * sizeof(char *));
    for (int i = 0; i < 1024; ++i)
    {
        single_command->command[i] = NULL;
    }
}

void freeSimpleCommand(SimpleCommand_t *single_command)
{
    for (int i = 0; i < single_command->current_argc; ++i)
        free(single_command->command[i]);
    free(single_command->command);
}

int DetectQuote(const char *inputLine, int position, int quote_flag)
{
    // return the updated quote flag
    if (quote_flag == 0)
    {
        // if not set
        if (inputLine[position] == '\'')
            return 1;
        else if (inputLine[position] == '\"')
            return 2;
        else
            return 0;
    }
    else
    {
        if (inputLine[position] == '\'' && quote_flag == 1)
            return 0;
        else if (inputLine[position] == '\"' && quote_flag == 2)
            return 0;
        else
            return quote_flag;
    }
}

void AddSpace(char *inputLine)
{
    // detect >> | < > outside quotes, and add spaces before and after them
    int length = (int) strlen(inputLine) + 1; // add the position for \0
    int quote_flag = 0; // 0 means no quote, 1 means single, 2 means double
    int position = 0;
    while (position < length)
    {
        quote_flag = DetectQuote(inputLine, position, quote_flag);
        if (quote_flag == 0)
        {
            // if it is not in a quote
            // first judge >>
            int append_flag = 0;
            if (position + 1 < length)
            {
                char buffer[3]; // length 2 string to store temp
                buffer[0] = inputLine[position];
                buffer[1] = inputLine[position + 1];
                buffer[2] = '\0';
                if (strcmp(buffer, ">>") == 0)
                    append_flag = 1;
            }
            if (append_flag)
            {
                memmove(inputLine + position + 1, inputLine + position, length - position);
                inputLine[position] = ' ';
                length++;
                memmove(inputLine + position + 4, inputLine + position + 3, length - position - 3);
                inputLine[position + 3] = ' ';
                length++;
                position += 4;
            }
            else if (inputLine[position] == '|' || inputLine[position] == '>' || inputLine[position] == '<')
            {
                memmove(inputLine + position + 1, inputLine + position, length - position);
                inputLine[position] = ' ';
                length++;
                memmove(inputLine + position + 3, inputLine + position + 2, length - position - 2);
                inputLine[position + 2] = ' ';
                length++;
                position += 3;
            }
            else
                position++;
        }
        else
            position++;
    }
}

void DeleteQuote(char *inputLine)
{
    // delete paired " in the inputLine, except for _"<"_  _">"_  _"|"_  _">>"_ four cases (_ resembles space)
    int length = (int) strlen(inputLine) + 1; // add the position for \0
    int quote_flag = 0; // 0 means no quote, 1 means single, 2 means double
    int position = 0;
    while (position < length)
    {
        quote_flag = DetectQuote(inputLine, position, quote_flag);
        if (quote_flag == 0)
            position++;
        else
        {
            position++;
            int new = FindNextQuote(inputLine, position, quote_flag);
            quote_flag = 0;
            int pattern_flag = 0;
            if ((new == position + 1 &&
                 (inputLine[position] == '>' || inputLine[position] == '<' || inputLine[position] == '|')) ||
                (new == position + 2 && (inputLine[position] == '>' && inputLine[position + 1] == '>')))
                pattern_flag = 1;  // test whether there is "<"  ">"  "|"  ">>"
            if (pattern_flag && ((position - 2) >= 0 && inputLine[position - 2] == ' ') &&
                (new + 1 < length && inputLine[new + 1] == ' '))
            {
                // not remove this quote
                position = new + 1;
            }
            else if (pattern_flag && (position == 1 && (new + 1 < length && inputLine[new + 1] == ' ')))
            {
                // not remove the quote
                position = new + 1;
            }
            else if (pattern_flag && ((position - 2 >= 0 && inputLine[position - 2] == ' ') && (new + 2 == length)))
            {
                // not remove the quote
                position = new + 1;
            }
            else
            {
                memmove(inputLine + position - 1, inputLine + position, length - position);
                length--;
                memmove(inputLine + new - 1, inputLine + new, length - new);
                position = new - 1;
                length--;
            }
        }
    }
}

int CheckSyntexError(char *str)
{
    if (strcmp(str, "|") == 0 || strcmp(str, "<") == 0 || strcmp(str, ">") == 0 || strcmp(str, ">>") == 0)
    {
        printf("syntax error near unexpected token `%s'\n", str);
        fflush(stdout);
        return -1;
    }
    return 0;
}

int Parse_old(char *inputLine, CommandTable_t *commandTable, BackgroundStack_t *backgroundCmd)
{
    // return -1 if have error, return 0 if fine
    // check whether it is a background process
    int p = strlen(inputLine) - 1;
    while (inputLine[p] == ' ')
        p--;
    if (inputLine[p] == '&')
    {
        commandTable->background = 1;
        backgroundCmd->background_commands[backgroundCmd->num] = malloc(sizeof(char) * (strlen(inputLine) + 1));
        memcpy(backgroundCmd->background_commands[backgroundCmd->num], inputLine, strlen(inputLine));
        backgroundCmd->background_commands[backgroundCmd->num][strlen(inputLine)] = '\0';
        backgroundCmd->num++;
        inputLine[p] = ' ';
    }

    AddSpace(inputLine);
    DeleteQuote(inputLine);
    int length = (int) strlen(inputLine) + 1;
    int position = 0;
    int quote_flag = 0;
    SimpleCommand_t *current_cmd = malloc(sizeof(SimpleCommand_t));
    initSimpleCommand(current_cmd);
    commandTable->commands[commandTable->current_sc] = current_cmd;
    commandTable->current_sc++;
    int new = 0;
    while (position < length)
    {
        // skip the beginning spaces of an argv
        new = FindNonSpace(inputLine, position);
        if (new == -1) // already reaches \0
            break;
        position = new;
        if (inputLine[position] == '\'')
            quote_flag = 1;
        else if (inputLine[position] == '\"')
            quote_flag = 2;
        if (quote_flag > 0)
        {
            position++;
            new = FindNextQuote(inputLine, position, quote_flag);
            // add a new argv[.] into the single command
            current_cmd->command[current_cmd->current_argc] = malloc((new - position + 1) * sizeof(char));
            memcpy(current_cmd->command[current_cmd->current_argc], inputLine + position, new - position);
            current_cmd->command[current_cmd->current_argc][new - position] = '\0';
            current_cmd->current_argc++;

            quote_flag = 0;
            position = new;
            position++;
        }
        else
        {
            new = FindNextSpace(inputLine, position);
            char *tmp = malloc((new - position + 1) * sizeof(char));
            memcpy(tmp, inputLine + position, new - position);
            tmp[new - position] = '\0';
            position = new; // move after tmp
            if (strcmp(tmp, "|") == 0)
            {
                // move on to next simple command
                free(tmp);
                tmp = NULL;
                current_cmd = malloc(sizeof(SimpleCommand_t));
                initSimpleCommand(current_cmd);
                commandTable->commands[commandTable->current_sc] = current_cmd;
                commandTable->current_sc++;
                if (commandTable->out_file != NULL || commandTable->append_file != NULL)
                {
                    // this error already exists when we proceed here
                    printf("error: duplicated output redirection\n");
                    fflush(stdout);
                    return -1;
                }
                // deal with missing program error
                int position_tmp = FindNonSpace(inputLine, position);
                int new_tmp = FindNextSpace(inputLine, position_tmp);
                tmp = malloc((new_tmp - position_tmp + 1) * sizeof(char));
                memcpy(tmp, inputLine + position_tmp, new_tmp - position_tmp);
                tmp[new_tmp - position_tmp] = '\0';
                if (strcmp(tmp, "|") == 0)
                {
                    printf("error: missing program\n");
                    fflush(stdout);
                    free(tmp);
                    return -1;
                }
                free(tmp);
            }
            else if (strcmp(tmp, ">") == 0)
            {
                free(tmp);
                tmp = NULL;
                position = FindNonSpace(inputLine, position);
                new = FindNextSpace(inputLine, position);
                // syntex error happens eariler than multiple input error
                tmp = malloc((new - position + 1) * sizeof(char));
                memcpy(tmp, inputLine + position, new - position);
                tmp[new - position] = '\0';
                int state = CheckSyntexError(tmp);
                if (state == -1)
                {
                    free(tmp);
                    return -1;
                }
                free(tmp);
                if (commandTable->out_file == NULL && commandTable->append_file == NULL)
                {
                    commandTable->out_file = malloc((new - position + 1) * sizeof(char));
                    memcpy(commandTable->out_file, inputLine + position, new - position);
                    commandTable->out_file[new - position] = '\0';
                }
                else
                {
                    printf("error: duplicated output redirection\n");
                    fflush(stdout);
                    return -1;
                }
                position = new;
            }
            else if (strcmp(tmp, ">>") == 0)
            {
                free(tmp);
                tmp = NULL;
                position = FindNonSpace(inputLine, position);
                new = FindNextSpace(inputLine, position);
                tmp = malloc((new - position + 1) * sizeof(char));
                memcpy(tmp, inputLine + position, new - position);
                tmp[new - position] = '\0';
                int state = CheckSyntexError(tmp);
                if (state == -1)
                {
                    free(tmp);
                    return -1;
                }
                free(tmp);
                if (commandTable->append_file == NULL && commandTable->out_file == NULL)
                {
                    commandTable->append_file = malloc((new - position + 1) * sizeof(char));
                    memcpy(commandTable->append_file, inputLine + position, new - position);
                    commandTable->append_file[new - position] = '\0';
                }
                else
                {
                    printf("error: duplicated output redirection\n");
                    fflush(stdout);
                    return -1;
                }
                position = new;
            }
            else if (strcmp(tmp, "<") == 0)
            {
                free(tmp);
                tmp = NULL;
                position = FindNonSpace(inputLine, position);
                new = FindNextSpace(inputLine, position);
                tmp = malloc((new - position + 1) * sizeof(char));
                memcpy(tmp, inputLine + position, new - position);
                tmp[new - position] = '\0';
                int state = CheckSyntexError(tmp);
                if (state == -1)
                {
                    free(tmp);
                    return -1;
                }
                free(tmp);
                if (commandTable->in_file == NULL && commandTable->current_sc == 1)
                {
                    commandTable->in_file = malloc((new - position + 1) * sizeof(char));
                    memcpy(commandTable->in_file, inputLine + position, new - position);
                    commandTable->in_file[new - position] = '\0';
                }
                else
                {
                    printf("error: duplicated input redirection\n");
                    return -1;
                }
                position = new;
            }
            else
            {
                current_cmd->command[current_cmd->current_argc] = tmp;
                current_cmd->current_argc++;
            }
        }
    }
    return 0;
}

void SeparateSpace(char *inputLine, ParserHelper_t *parser_helper)
{
    // separate inputLine by space
    int length = strlen(inputLine) + 1;
    int position = 0;
    int quote_flag = 0;
    int segment_num = 0; // if meets space, segment_num++
    int begin=0,end=0;
    int search_nonspace=1;
    while (position < length)
    {
        // skip the beginning spaces of an argv
        if(search_nonspace)  // only after dealt with a whole segment, we need to seach for new non-space
        {
            begin = FindNonSpace(inputLine, position);
            position=begin;
            search_nonspace=0;
        }
        if (begin == -1) // already reaches \0
        {
            // in this case position is already reaches the last case
            break;
        }
        if (inputLine[position] == '\'')
            quote_flag = 1;
        else if (inputLine[position] == '\"')
            quote_flag = 2;
        if (quote_flag > 0)
        {
            position++;
            position = FindNextQuote(inputLine, position, quote_flag);
            quote_flag = 0;
            position++;
        }
        else
        {
            if(inputLine[position]==' ' || inputLine[position]=='\0')
            {
                end=position;
                parser_helper->segment[segment_num]=malloc(sizeof(char)*(end-begin+1));
                memcpy(parser_helper->segment[segment_num], inputLine+begin,end-begin);
                parser_helper->segment[segment_num][end-begin]='\0';
                segment_num++;
                search_nonspace=1;
            }
            position++;
        }
    }
    parser_helper->num=segment_num;
}

int
Parse(char *inputLine, ParserHelper_t *parser_helper, CommandTable_t *commandTable, BackgroundStack_t *backgroundCmd)
{
    // return error code if have error, return 0 if fine
    // check whether it is a background process
    int error_state=0;
    int p = strlen(inputLine) - 1;
    while (inputLine[p] == ' ')
        p--;
    if (inputLine[p] == '&')
    {
        commandTable->background = 1;
        backgroundCmd->background_commands[backgroundCmd->num] = malloc(sizeof(char) * (strlen(inputLine) + 1));
        memcpy(backgroundCmd->background_commands[backgroundCmd->num], inputLine, strlen(inputLine));
        backgroundCmd->background_commands[backgroundCmd->num][strlen(inputLine)] = '\0';
        backgroundCmd->num++;
        inputLine[p] = ' ';
    }
    AddSpace(inputLine);
    SeparateSpace(inputLine, parser_helper);
//    for(int i=0;i<parser_helper->num;++i)
//    {
//        printf("%s\n",parser_helper->segment[i]);
//    }
    SimpleCommand_t *current_cmd = malloc(sizeof(SimpleCommand_t));
    initSimpleCommand(current_cmd);
    commandTable->commands[commandTable->current_sc] = current_cmd;
    commandTable->current_sc++;
    int i=0;
    while(i<parser_helper->num)
    {
        char* current_segment=parser_helper->segment[i];
        if(strcmp(current_segment,"|")==0)
        {
            if (commandTable->out_file != NULL || commandTable->append_file != NULL)
            {
                // this error already exists when we proceed here
//                printf("error: duplicated output redirection\n");
//                fflush(stdout);
                error_state=5;
            }
            current_cmd = malloc(sizeof(SimpleCommand_t));
            initSimpleCommand(current_cmd);
            commandTable->commands[commandTable->current_sc] = current_cmd;
            commandTable->current_sc++;
        }
        else if(strcmp(current_segment,">>")==0)
        {
            if (commandTable->append_file == NULL && commandTable->out_file == NULL)
            {
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->append_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->append_file, parser_helper->segment[i], len);
                commandTable->append_file[len] = '\0';
            }
            else
            {
//                printf("error: duplicated output redirection\n");
//                fflush(stdout);
                error_state=5;
                free(commandTable->append_file);
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->append_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->append_file, parser_helper->segment[i], len);
                commandTable->append_file[len] = '\0';
            }
        }
        else if(strcmp(current_segment,">")==0)
        {
            if (commandTable->append_file == NULL && commandTable->out_file == NULL)
            {
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->out_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->out_file, parser_helper->segment[i], len);
                commandTable->out_file[len] = '\0';
            }
            else
            {
//                printf("error: duplicated output redirection\n");
//                fflush(stdout);
                error_state = 5;
                free(commandTable->out_file);
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->out_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->out_file, parser_helper->segment[i], len);
                commandTable->out_file[len] = '\0';
            }
        }
        else if(strcmp(current_segment,"<")==0)
        {
            if (commandTable->in_file == NULL && commandTable->current_sc == 1)
            {
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->in_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->in_file, parser_helper->segment[i], len);
                commandTable->in_file[len] = '\0';
            }
            else
            {
//                printf("error: duplicated input redirection\n");
//                fflush(stdout);
                error_state = 4;
                free(commandTable->in_file);
                i++;
                DeleteQuote(parser_helper->segment[i]);
                int len=strlen(parser_helper->segment[i]);
                commandTable->in_file = malloc((len+1) * sizeof(char));
                memcpy(commandTable->in_file, parser_helper->segment[i], len);
                commandTable->in_file[len] = '\0';
            }
        }
        else
        {
            DeleteQuote(parser_helper->segment[i]);
            int len=strlen(parser_helper->segment[i]);
            current_cmd->command[current_cmd->current_argc] = malloc((len+1) * sizeof(char));
            memcpy(current_cmd->command[current_cmd->current_argc], parser_helper->segment[i], len);
            current_cmd->command[current_cmd->current_argc][len] = '\0';
            current_cmd->current_argc++;
        }
        i++;
    }
    return error_state;
}

int FindNonSpace(const char *inputLine, int position)
{
    while (inputLine[position] == ' ')
    {
        position++;
    }
    if (inputLine[position] == '\0')
        return -1; //means there is nothing left
    else
        return position;
}

int FindNextSpace(const char *inputLine, int position)
{
    while (inputLine[position] != ' ' && inputLine[position] != '\0')
    {
        position++;
    }
    return position;
}

int FindNextQuote(const char *inputLine, int position, int quote_flag)
{
    if (quote_flag == 1)
    {
        while (inputLine[position] != '\'' && inputLine[position] != '\0')
            position++;
    }
    else if (quote_flag == 2)
    {
        while (inputLine[position] != '\"' && inputLine[position] != '\0')
            position++;
    }
    else
    {
        printf("Error: Wrong quote state!!!\n");
    }
    return position;
}

