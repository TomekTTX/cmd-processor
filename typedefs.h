#pragma once
#include <stdbool.h>

#pragma warning (disable: 4820)

typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ullong;
typedef          long long  llong;

typedef void (*destroy_func_t)(void *);

//typedef struct obj_data_t_ {
//    bool is_dynamic_memory, is_valid;
//    uchar flags[2];
//} obj_data_t;

typedef struct arg_node_t_ {
    const char *key, *format;
    uint size;
} arg_node_t;

typedef struct ptr_arraylist_t_ {
    void **arr;
    uint size, count;
    destroy_func_t elem_destr_func;
} ptr_arraylist_t;

typedef struct byte_arraylist_t_ {
    uchar *arr;
    uint size, count;
} byte_arraylist_t;

typedef struct arg_bundle_t_ {
    void *static_data;
    byte_arraylist_t data;
    ptr_arraylist_t args, dynamic_blocks;
    uint index;
    bool empty;
} arg_bundle_t;

typedef void (*cmd_act_t)(arg_bundle_t *);

typedef struct cmd_proc_t_ {
    cmd_act_t action;
    void *static_data;
} cmd_proc_t;

typedef struct command_t_ {
    char *name;
    uint arg_size, arg_cnt;
    cmd_proc_t action;
    arg_node_t **syntax;
    ptr_arraylist_t subcommands;
    bool is_dynamic_memory;
} command_t;

typedef struct cmd_map_t_ {
    command_t *map;
    uint size, count;
} cmd_map_t;

typedef struct tokenized_str_t_ {
    char *str;
    ptr_arraylist_t parts;
} tokenized_str_t;

