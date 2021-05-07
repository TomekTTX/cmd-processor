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

byte_arraylist_t byte_arraylist_make(void);
bool byte_arraylist_push(byte_arraylist_t *list, uchar item);
void byte_arraylist_destroy(byte_arraylist_t *list);

tokenized_str_t tok_str_make(const char *str, char delim);
char *tok_str_get(tokenized_str_t *tok_str, uint index);
void tok_str_destroy(tokenized_str_t *tok_str);

arg_bundle_t arg_bundle_make(void);
bool arg_bundle_add_(arg_bundle_t *bundle, const void *src, uint size, bool dynamic);
uint arg_bundle_get_(arg_bundle_t *bundle, void *dst, uint size);
void *arg_bundle_get_raw_(arg_bundle_t *bundle);
void arg_bundle_destroy(arg_bundle_t *bundle);

#define arg_bundle_add(bundle, data) arg_bundle_add_(bundle, &data, sizeof(data))
#define arg_bundle_get(bundle, dst) arg_bundle_get_(bundle, &dst, sizeof(dst))
#define arg_bundle_getas(bundle, type) (*(type *)arg_bundle_get_raw_(bundle))
//#define next_arg arg_bundle_getas
