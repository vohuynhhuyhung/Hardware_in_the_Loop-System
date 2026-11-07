/*
* cmd.c
*
* Created on: Sep 28, 2025
* Author: VyTran
*
*/

#include "cmd.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

static int Cmd_help(int argc, char *argv[]);
static int Cmd_Clear_CLI(int argc, char *argv[]);

typedef struct {
    char commandBuffer[COMMAND_MAX_LENGTH];
    uint16_t commandBufferIndex;
    char commandHistory[MAX_HISTORY][COMMAND_MAX_LENGTH];
    uint16_t historyCount;
    uint16_t historyIndex;
} CMDLine_Context;

static CMDLine_Context pContext = {0};

const char *ErrorCode[4] = {
		"OK\r\n",
        "BAD_CMD\r\n",
        "TOO_MANY_ARGS\r\n",
        "TOO_FEW_ARGS\r\n"
};

tCmdLineEntry g_psCmdTable[MAX_CMDS];
static int g_cmdCount = 0;

/*----------------- DEFAUTL COMMAND FUNCTIONS -----------------*/
static int Cmd_help(int argc, char *argv[]) {
    tCmdLineEntry *pEntry = &g_psCmdTable[0];
    while(pEntry->pcCmd) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s - %s\r\n", pEntry->pcCmd, pEntry->pcHelp);
        Console_Write(buffer);
        pEntry++;
    }
    return CMDLINE_OK;
}

static int Cmd_Clear_CLI(int argc, char *argv[]) {
    Console_Write("\033[2J\033[H"); // clear screen
    return CMDLINE_OK;
}

// ================= CLI Core =================

void CLI_RegisterCommand(const char *cmd, int (*handler)(int, char **), const char *help)
{
    if (g_cmdCount < MAX_CMDS) {
    	g_psCmdTable[g_cmdCount].pcCmd = cmd;
    	g_psCmdTable[g_cmdCount].pfnCmd = handler;
    	g_psCmdTable[g_cmdCount].pcHelp = help;
        g_cmdCount++;
    }
}

static void process_command(char rxData, CMDLine_Context *context) {
    if(rxData == KEY_ENTER) {
        if(context->commandBufferIndex > 0) {
            context->commandBuffer[context->commandBufferIndex] = '\0';

            // save history
            if(context->historyCount == 0 || strcmp(context->commandHistory[context->historyCount-1], context->commandBuffer)!=0) {
                if(context->historyCount < MAX_HISTORY) {
                    strcpy(context->commandHistory[context->historyCount], context->commandBuffer);
                    context->historyCount++;
                } else {
                    for(int i=0;i<MAX_HISTORY-1;i++)
                        strcpy(context->commandHistory[i], context->commandHistory[i+1]);
                    strcpy(context->commandHistory[MAX_HISTORY-1], context->commandBuffer);
                }
            }
            context->historyIndex = context->historyCount;

            int8_t ret_val = CmdLineProcess(context->commandBuffer);
            if(ret_val != CMDLINE_OK) {
                Console_Write(ErrorCode[ret_val]);
            }
            Console_Write("\r\n");
        }
        context->commandBufferIndex = 0;
        Console_Write(NAME_SHELL);
    } else if(rxData == KEY_BACKSPACE) {
        if(context->commandBufferIndex>0) {
            context->commandBufferIndex--;
            context->commandBuffer[context->commandBufferIndex] = '\0';
        }
    } else if(rxData >= 32 && rxData <= 126) {
        if(context->commandBufferIndex < COMMAND_MAX_LENGTH-1) {
            context->commandBuffer[context->commandBufferIndex++] = rxData;
            context->commandBuffer[context->commandBufferIndex] = '\0';
        }
    }
}

static void CLI_Task_Update(void) {
    if(Console_Available()) {
        char rxData = Console_Read();

        if(rxData >= 32 && rxData <= 126) {
            // Append vào buffer
            if(pContext.commandBufferIndex < COMMAND_MAX_LENGTH-1) {
                pContext.commandBuffer[pContext.commandBufferIndex++] = rxData;
                pContext.commandBuffer[pContext.commandBufferIndex] = '\0';
            }
            // Echo ra terminal
            char tmp[2] = {rxData, 0};
            Console_Write(tmp);
        } else if(rxData == KEY_BACKSPACE) {
            if(pContext.commandBufferIndex > 0) {
                pContext.commandBufferIndex--;
                pContext.commandBuffer[pContext.commandBufferIndex] = '\0';
                Console_Write("\b \b"); // xóa ký tự trên terminal
            }
        } else if(rxData == KEY_ENTER) {
            Console_Write("\r\n");          // xuống dòng
            process_command(rxData, &pContext);
        }
    }
}

void CLI_Task(void *pvParameters) {
    while(1) {
        CLI_Task_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void CLI_Init(void) {
    memset(pContext.commandBuffer, 0, sizeof(pContext.commandBuffer));
    pContext.commandBufferIndex = 0;

    Console_Init();
    Console_Write("\r\nHIL FIRMWARE\r\n");
    Console_Write(NAME_SHELL);

    // Register commands
    CLI_RegisterCommand("help", Cmd_help, "Display list of available commands | format: help");
    CLI_RegisterCommand("cls", Cmd_Clear_CLI, "Clear Console | format: cls");
    Cmd_SPI_Register();
    Cmd_I2C_Register();
}

