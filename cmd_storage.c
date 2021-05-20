#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd_storage.h"

#pragma warning (disable: 5045)

/*
* Provides size and format for sscanf() for a given argument type
* 
* key - argument type, such as <INT> or <STRING>
* 
* returns - pointer to a struct containing the argument's format and size
*/
const arg_node_t *size_node_get(const char *key) {
    static const arg_node_t nodes[] = {
        { "<SUBCMD>",  ">",      0              },
     // { "<VOID>",    "$null",  0              },
        { "<CHAR>",    "%c",     sizeof(char)   },
        { "<UCHAR>",   "%hhu",   sizeof(uchar)  },
        { "<BYTE>",    "%hhd",   sizeof(char)   },
        { "<UBYTE>",   "%hhu",   sizeof(uchar)  },
        { "<SHORT>",   "%hd",    sizeof(short)  },
        { "<USHORT>",  "%hu",    sizeof(ushort) },
        { "<INT>",     "%d",     sizeof(int)    },
        { "<UINT>",    "%u",     sizeof(uint)   },
        { "<LONG>",    "%ld",    sizeof(long)   },
        { "<ULONG>",   "%lu",    sizeof(ulong)  },
        { "<LLONG>",   "%lld",   sizeof(llong)  },
        { "<ULLONG>",  "%llu",   sizeof(ullong) },
        { "<STRING>",  "%511s",  sizeof(char *) },
        { "<PTR>",     "%p",     sizeof(void *) },
     // { "<CMD>",     "$cmd",   sizeof(void *) },
    };
    static const arg_node_t err = { "<ERROR>", "$unknown", 0 };



    if (key[0] != '<') {
        DEBUG_ONLY(printf("[INFO] Node returned: %s\n", nodes[0].key););
        return nodes + 0;
    }

    for (uint i = 1; i < (sizeof(nodes) / sizeof(nodes[0])); ++i) {
        if (str_eq(key, nodes[i].key)) {
            DEBUG_ONLY(printf("[INFO] Node returned: %s\n", nodes[i].key););
            return nodes + i;
        }
    }

    DEBUG_ONLY(puts("[WARN] size_node_get() returned <ERROR> node"));
    return &err;
}

// Temporary; adds cmd to list
// Used only by cmd_syntax_parse()
// Currently exists only for logging reasons
void cmd_merge_subcmd(ptr_arraylist_t *list, command_t *cmd) {
    DEBUG_ONLY(printf("[INFO] Merge called for %s\n", cmd->name);)
    arraylist_push(list, cmd);
}

/*
* Fills a command's syntax array
* Makes cross-recursive calls with cmd_make()/cmd_alloc() when handling subcommands
* 
* args - currently parsed command string
* cmd  - pointer to a command whose syntax array is to be filled
*/
void cmd_syntax_parse(char *args, command_t *cmd) {

    for (uint i = 0, j = 0; i < cmd->arg_cnt; ++i) {
        cmd->syntax[i] = (arg_node_t *)size_node_get(args + j);
        if (cmd->syntax[i]->format[0] == '>') {
            cmd_proc_t action = cmd->action;

            cmd->action.action = NULL;
            cmd_merge_subcmd(&cmd->subcommands, cmd_alloc(args + j, action));
        }
        while (args[j++] != '\0');
    }
}

/*
* Creates a heap-allocated command struct
* 
* input - command string
* proc  - a struct containing the command's function and static data
* 
* returns - pointer to the newly created command
*/
command_t *cmd_alloc(const char *input, cmd_proc_t proc) {
    command_t *ret = malloc(sizeof(command_t));
    if (ret) {
        *ret = cmd_make(input, proc);
        ret->is_dynamic_memory = true;
    }
    return ret;
}

/*
* Creates a stack-allocated command struct
* Makes cross-recursive calls with cmd_syntax_parse() when handling subcommands
*
* str  - command string
* proc - a struct containing the command's function and static data
*
* returns - the newly created command
*/
command_t cmd_make(const char *str, cmd_proc_t proc) {
    command_t ret = { 0 };
    char *args = (char *)str;

    if (!str)
        return ret;

    while (*++args);
    for (uint i = 0; args[i] || args[i + 1]; ++i) {
        if (args[i] == '\0') {
            ret.arg_cnt++;
            if (args[i + 1] != '<')
                break;          
        }
    }

    ret.name = _strdup(str);
    ret.syntax = ret.arg_cnt ? malloc(ret.arg_cnt * sizeof(arg_node_t *)) : NULL;
    ret.action = proc;
    ret.subcommands = arraylist_make(&cmd_destroy);
    cmd_syntax_parse(args + 1, &ret);

    return ret;
}

/*
* Frees all memory allocated by cmd_make() / cmd_alloc()
* The command becomes unusable, obviously
* 
* cmd - pointer to a command that is to be deleted
*/
void cmd_destroy(command_t *cmd) {
    if (!cmd)
        return;
    if (cmd->name)
        free(cmd->name);
    if (cmd->syntax)
        free(cmd->syntax);
    arraylist_destroy(&cmd->subcommands);
    if (cmd->is_dynamic_memory)
        free(cmd);
}




// EXPERIMENTAL
command_t *cmd_alloc_(const tokenized_str_t *str, cmd_proc_t proc, uint str_index) {
    command_t *ret = malloc(sizeof(command_t));
    if (ret) {
        *ret = cmd_make_(str, proc, str_index);
        ret->is_dynamic_memory = true;
    }
    return ret;
}

// EXPERIMENTAL
void cmd_syntax_parse_(const tokenized_str_t *str, command_t *cmd, uint str_index) {
    for (uint i = 0; i < cmd->arg_cnt; ++i) {
        cmd->syntax[i] = (arg_node_t *)size_node_get(tok_str_get(str, str_index));
        if (cmd->syntax[i]->format[0] == '>') {
            cmd_proc_t action = cmd->action;

            cmd->action.action = NULL;
            arraylist_push(&cmd->subcommands, cmd_alloc_(str, action, str_index));
        }
        ++str_index;
    }
}

// EXPERIMENTAL
command_t cmd_make_(const tokenized_str_t *str, cmd_proc_t proc, uint str_index) {
    command_t ret = { 0 };

    if (!str)
        return ret;

    ret.name = _strdup(tok_str_get(str, str_index));

    while (str_index + ret.arg_cnt + 1 < str->parts.count) {
        char *token = tok_str_get(str, str_index + ++ret.arg_cnt);        
        if (token[0] != '<')
            break;
    }

    ret.syntax = ret.arg_cnt ? malloc(ret.arg_cnt * sizeof(arg_node_t *)) : NULL;
    ret.action = proc;
    ret.subcommands = arraylist_make(&cmd_destroy);
    cmd_syntax_parse_(str, &ret, str_index + 1);

    return ret;
}