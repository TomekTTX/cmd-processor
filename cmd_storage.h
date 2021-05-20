#pragma once
#include "cmd_main.h"

command_t *cmd_alloc(const char *input, cmd_proc_t proc);
command_t cmd_make(const char *str, cmd_proc_t proc);
void cmd_destroy(command_t *cmd);

// EXPERIMENTAL
void cmd_syntax_parse_(const tokenized_str_t *str, command_t *cmd, uint str_index);
// EXPERIMENTAL
command_t cmd_make_(const tokenized_str_t *str, cmd_proc_t proc, uint str_index);
// EXPERIMENTAL
command_t *cmd_alloc_(const tokenized_str_t *str, cmd_proc_t proc, uint str_index);
