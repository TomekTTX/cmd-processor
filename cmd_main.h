#pragma once

#define INTERACTIVE
#define DEBUG

#ifdef DEBUG
#define DEBUG_ONLY(expr) expr
#else
#define DEBUG_ONLY(expr)
#endif // DEBUG

#ifdef INTERACTIVE
#define INTERACTIVE_ONLY(expr) expr
#else
#define INTERACTIVE_ONLY(expr)
#endif // INTERACTIVE

#include "struct_funcs.h"

bool str_eq(const char *s1, const char *s2);

bool cmd_register_(const char *cmd_str, cmd_act_t action, void *static_data);
bool cmd_execute(const char *cmd_str);
void cmd_loop(bool add_defaults);

void cmd_print_rec(const command_t *cmd, uint depth);
#define cmd_print(cmd) cmd_print_rec(cmd, 0)

void cmd_dumpall(void);
bool cmd_register(const char *cmd_str, cmd_act_t action, void *static_data);
