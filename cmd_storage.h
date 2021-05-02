#pragma once
#include "cmd_main.h"

//command_t cmd_make(const char *input, cmd_proc_t proc);
command_t *cmd_alloc(const char *input, cmd_proc_t proc);
command_t cmd_make(char *str, cmd_proc_t proc);
void cmd_destroy(command_t *cmd);

