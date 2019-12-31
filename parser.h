#ifndef SHELL_PARSER_H
#define SHELL_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datastructure.h"
#define COMMAND_NUM 1025
#define BACKGROUND_COMMAND_NUM 20

void initParserHelper(ParserHelper_t* parser_helper);
void freeParserHelper(ParserHelper_t* parser_helper);
void initBackgroundCmd(BackgroundStack_t* backgroundcmd);
void freeBackgroundCmd(BackgroundStack_t* backgroundcmd);
void initCommandTable(CommandTable_t *command_table);
void freeCommandTable(CommandTable_t *command_table);
void initSimpleCommand(SimpleCommand_t *single_command);
void freeSimpleCommand(SimpleCommand_t *single_command);
int DetectQuote(const char *inputLine, int position, int quote_flag);
void AddSpace(char* inputLine);
void DeleteQuote(char *inputLine);
int Parse_old(char* inputLine, CommandTable_t* commandTable, BackgroundStack_t* backgroundCmd);
void SeparateSpace(char* inputLine, ParserHelper_t* parser_helper);
int Parse(char* inputLine, ParserHelper_t* parser_helper, CommandTable_t* commandTable, BackgroundStack_t* backgroundCmd);
int FindNonSpace(const char* inputLine, int position);
int FindNextQuote(const char* inputLine, int position, int quote_flag);
int FindNextSpace(const char* inputLine, int position);
int CheckSyntexError(char* str);
#endif //SHELL_PARSER_H
