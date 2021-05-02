#pragma once
#include "struct_funcs.h"

bool str_eq(const char *s1, const char *s2);

bool cmd_register(const char *cmd_str, cmd_proc_t action);
