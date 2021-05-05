#include "cmd_main.h"
#include "cmd_storage.h"
#include "struct_funcs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define cmd_print(cmd) cmd_print_rec(cmd, 0)
#pragma warning (disable: 5045)

cmd_map_t global_command_map = { 0 };

typedef struct cmd_tree_location_t_ {
    char *ptr;
    command_t *parent;
} cmd_tree_location_t;

bool str_eq(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

void cmd_preprocess(char *str) {
    if (!str)
        return;

    const int len = strlen(str);

    for (int i = len - 1; i >= 0 && str[i] <= ' '; str[i--] = '\0');
    for (int i = 0; i < len; ++i)
        if (str[i] == ' ')
            str[i] = '\0';
}

command_t *find_subcommand(const char *key, const ptr_arraylist_t *list) {
    for (uint j = 0; j < list->count; ++j) {
        debug_only(printf("compare: %s - %s\n", key, ((command_t *)list->arr[j])->name);)
        if (str_eq(key, ((command_t *)list->arr[j])->name)) {
            debug_only(puts("equal!");)
            return (command_t *)list->arr[j];
        }
    }
    return NULL;
}

cmd_tree_location_t cmd_skip_existent(char *cmd_str, const cmd_map_t *cmd_map) {
    cmd_tree_location_t ret = { 0 };

    if (!cmd_str || !cmd_map)
        return ret;

    const command_t *cur = cmd_map_find(cmd_map, cmd_str);

    if (cur == NULL) {
        ret.ptr = cmd_str;
        return ret;
    }

    for (uint i = 0; cmd_str[i] || cmd_str[i + 1]; ++i) {
        if (cmd_str[i] == '\0') {
            ret.parent = cur;
            cur = find_subcommand(cmd_str + i + 1, &cur->subcommands);
            if (cur == NULL) {
                ret.ptr = cmd_str + i + 1;
                return ret;
            }
        }
    }

    return ret;
}

bool cmd_register(const char *cmd_str, cmd_proc_t action) {
    if (global_command_map.map == NULL)
        global_command_map = cmd_map_make();

    debug_only(printf("REGISTER START (%s)\n", cmd_str);)

    char *str = _strdup(cmd_str);
    cmd_preprocess(str);
    cmd_tree_location_t loc = cmd_skip_existent(str, &global_command_map);


    if (loc.parent == NULL) {
        command_t cmd = cmd_make(loc.ptr, action);
        free(str);
        debug_only(printf("REGISTER FINISH (%s)\n", cmd_str);)
        return cmd_map_add(&global_command_map, &cmd);
    }
    else {
        command_t *cmd = cmd_alloc(loc.ptr, action);
        free(str);
        debug_only(printf("REGISTER FINISH (%s)\n", cmd_str);)
        return arraylist_push(&loc.parent->subcommands, cmd);
    }
}

void cmd_print_rec(const command_t *cmd, uint depth) {
    printf("%s ", cmd->name);
    for (uint i = 0; i < cmd->arg_cnt; ++i)
        printf("%s ", cmd->syntax[i]->key);
    for (uint i = 0; i < cmd->subcommands.count; ++i) {
        putchar('\n');
        for (uint i = 0; i <= depth * 2; ++i)
            putchar(' ');
        cmd_print_rec((const command_t *)cmd->subcommands.arr[i], depth + 1);
    }
}

void cmd_dumpall(void) {
    for (uint i = 0; i < global_command_map.size; ++i) {
        if (global_command_map.map[i].name != NULL)
            cmd_print(global_command_map.map + i);
    }
}
