#pragma once
#include "cmd_main.h"

command_t cmd_make(const char *input, cmd_proc_t proc/*, const cmd_map_t *merge_into*/);
command_t cmd_parse(char *str, cmd_proc_t proc);
void cmd_destroy(command_t *cmd);

