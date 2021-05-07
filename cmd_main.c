#include "cmd_main.h"
#include "cmd_storage.h"
#include "struct_funcs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#pragma warning (disable: 5045 4996)

cmd_map_t global_command_map = { 0 };

typedef struct cmd_tree_location_t_ {
    char *ptr;
    command_t *parent;
} cmd_tree_location_t;

typedef enum parser_state_t_ {
    COMMAND_EXPECTED,
    VALUE_EXPECTED,
    ERROR,
    READY,
    UNKNOWN,
} parser_state_t;

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
            ret.parent = (command_t *)cur;
            cur = (const command_t *)find_subcommand(cmd_str + i + 1, &cur->subcommands);
            if (cur == NULL) {
                ret.ptr = cmd_str + i + 1;
                return ret;
            }
        }
    }

    return ret;
}

bool cmd_register(const char *cmd_str, cmd_act_t action, const void *static_data) {
    if (global_command_map.map == NULL)
        global_command_map = cmd_map_make();

    debug_only(printf("REGISTER START (%s)\n", cmd_str);)

    char *str = _strdup(cmd_str);
    cmd_preprocess(str);
    const cmd_tree_location_t loc = cmd_skip_existent(str, &global_command_map);
    const cmd_proc_t proc = { .action = action, .static_data = static_data };

    if (loc.parent == NULL) {
        command_t cmd = cmd_make(loc.ptr, proc);
        free(str);
        debug_only(printf("REGISTER FINISH (%s)\n", cmd_str);)
        return cmd_map_add(&global_command_map, &cmd);
    }
    else {
        command_t *cmd = cmd_alloc(loc.ptr, proc);
        free(str);
        debug_only(printf("REGISTER FINISH (%s)\n", cmd_str);)
        return arraylist_push(&loc.parent->subcommands, cmd);
    }
}

parser_state_t next_state(const command_t *cur_cmd, uint args_parsed) {
    if (!cur_cmd)
        return ERROR;
    if (cur_cmd->arg_cnt > args_parsed) {
        if (cur_cmd->syntax[args_parsed]->format[0] == '>')
            return COMMAND_EXPECTED;       
        return VALUE_EXPECTED;
    }
    if (cur_cmd->arg_cnt == args_parsed)
        return READY;
    return UNKNOWN;
}

void bundle_push(arg_bundle_t *args, const uchar *data, const arg_node_t *syntax_node) {
    if (str_eq(syntax_node->key, "<STRING>")) {
        char *str = _strdup(data);
        arg_bundle_add_(args, &str, syntax_node->size, true);
    }
    else
        arg_bundle_add_(args, data, syntax_node->size, false);
}

bool cmd_execute(const char *cmd_str) {
    tokenized_str_t input = tok_str_make(cmd_str, ' ');
    arg_bundle_t args = arg_bundle_make();
    const char *cur_token = (const char *)tok_str_get(&input, 0);
    const command_t *cur_cmd = cmd_map_find(&global_command_map, cur_token);
    uint args_parsed = 0;
    parser_state_t state = next_state(cur_cmd, args_parsed);
    uchar buffer[512];


    for (uint i = 1; i <= input.parts.count; ++i) {
        if (i < input.parts.count)
            cur_token = (const char *)tok_str_get(&input, i);
        else if (state == COMMAND_EXPECTED || state == VALUE_EXPECTED)
            state = ERROR;

        switch (state) {
        case COMMAND_EXPECTED:
            cur_cmd = (const command_t *)find_subcommand(cur_token, &cur_cmd->subcommands);
            state = next_state(cur_cmd, args_parsed);
            args_parsed = 0;
            break;
        case VALUE_EXPECTED:
            if (sscanf(cur_token, cur_cmd->syntax[args_parsed]->format, (void *)buffer) > 0) {
                bundle_push(&args, buffer, cur_cmd->syntax[args_parsed++]);
                state = next_state(cur_cmd, args_parsed);
            }
            else
                state = ERROR;
            break;
        case READY:
            (*cur_cmd->action.action)(&args, cur_cmd->action.static_data);
            /* FALLTHROUGH */
        case ERROR:
            tok_str_destroy(&input);
            arg_bundle_destroy(&args);
            return (state == READY);
        default:
            debug_only(puts("UNKNOWN state detected!"));
            break;
        }
    }

    debug_only(puts("How did we get here?"));
    return false;
}

void cmd_print_rec(const command_t *cmd, uint depth) {
    printf("%s ", cmd->name);
    for (uint i = 0; i < cmd->arg_cnt; ++i)
        printf("%s ", cmd->syntax[i]->key);
    if (cmd->action.action != NULL)
        printf("-> calls %p(%p)", cmd->action.action, cmd->action.static_data);
    for (uint i = 0; i < cmd->subcommands.count; ++i) {
        const command_t *subcmd = (const command_t *)cmd->subcommands.arr[i];
        putchar('\n');
        for (uint j = 0; j <= depth * 2; ++j)
            putchar(' ');
        cmd_print_rec(subcmd, depth + 1);
    }
}

void cmd_dumpall(void) {
    for (uint i = 0; i < global_command_map.size; ++i) {
        if (global_command_map.map[i].name != NULL)
            cmd_print(global_command_map.map + i);
    }
    putchar('\n');
}
