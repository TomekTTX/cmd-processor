#pragma once
#include "typedefs.h"

#define CONTAINER_INIT_SIZE 2
#define UNREF(expr) (void)(expr)

cmd_map_t cmd_map_make(void);
bool cmd_map_add(cmd_map_t *map, const command_t *cmd);
const command_t *cmd_map_find(const cmd_map_t *map, const char *key);
void cmd_map_destroy(cmd_map_t *map);

ptr_arraylist_t arraylist_make(destroy_func_t elem_destr_func);
bool arraylist_push(ptr_arraylist_t *list, const void *item);
void arraylist_destroy(ptr_arraylist_t *list);

tokenized_str_t tok_str_make(const char *str, char delim);
char *tok_str_get(tokenized_str_t *tok_str, uint index);
void tok_str_destroy(tokenized_str_t *tok_str);
