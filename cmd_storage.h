#pragma once
#include "cmd_main.h"

#define CMD_MAP_INIT_SIZE 8
#define UNREF(expr) (void)(expr)

typedef struct cmd_map_t_ {
	command_t *map;
	uint size, count;
} cmd_map_t;


cmd_map_t cmd_map_make(void);
bool cmd_map_add(cmd_map_t *map, const command_t *cmd);
const command_t *cmd_map_find(const cmd_map_t *map, const char *key);
void cmd_map_destroy(cmd_map_t *map);

command_t cmd_make(const char *input, cmd_proc_t proc);
