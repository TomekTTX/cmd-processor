#include <stdbool.h>
#define DEBUG

typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef unsigned long long  ullong;
typedef          long long  llong;

typedef void (*cmd_proc_t)(void *);
typedef void (*destroy_func_t)(void *);

typedef struct arg_node_t_ {
    const char *key, *format;
    uint size;
} arg_node_t;

typedef struct ptr_arraylist_t_ {
    void **arr;
    uint size, count;
    destroy_func_t elem_destr_func;
} ptr_arraylist_t;

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

#ifdef DEBUG
#define debug_only(expr) expr
#else
#define debug_only(expr)
#endif // DEBUG