#pragma once
#include "cmd_main.h"

#define CONTAINER_INIT_SIZE 2
#define UNREF(expr) (void)(expr)

typedef void (*destroy_func_t)(void *);

typedef struct cmd_map_t_ {
	command_t *map;
	uint size, count;
} cmd_map_t;

typedef struct ptr_arraylist_t_ {
	void **arr;
	uint size, count;
	destroy_func_t elem_destr_func;
} ptr_arraylist_t;


cmd_map_t cmd_map_make(void);
bool cmd_map_add(cmd_map_t *map, const command_t *cmd);
const command_t *cmd_map_find(const cmd_map_t *map, const char *key);
void cmd_map_destroy(cmd_map_t *map);

ptr_arraylist_t arraylist_make(destroy_func_t elem_destr_func);
bool arraylist_push(ptr_arraylist_t *list, const void *item);
void arraylist_destroy(ptr_arraylist_t *list);

command_t cmd_make(const char *input, cmd_proc_t proc);
