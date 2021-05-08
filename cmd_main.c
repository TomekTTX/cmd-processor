#include "struct_funcs.h"
#include "cmd_main.h"
#include "cmd_storage.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#pragma warning (disable: 5045 4996)

// the main command map
// this is where cmd_register() puts its commands
// and where cmd_execute() looks to find them
cmd_map_t global_command_map = { 0 };

// a struct to allow cmd_skip_existent() to return 2 values
typedef struct cmd_tree_location_t_ {
    char *ptr;
    command_t *parent;
} cmd_tree_location_t;

// state enum used by cmd_execute()
typedef enum parser_state_t_ {
    COMMAND_EXPECTED,
    VALUE_EXPECTED,
    ERROR,
    READY,
    UNKNOWN,
} parser_state_t;

/*
* Checks if 2 strings are equal
* 
* s1 - string 1
* s2 - string 2
* 
* returns - whether strings 1 and 2 are equal
*/
bool str_eq(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

/*
* Changes all blank characters (including spaces) to NULs
* 
* str - the string where the chars are to be replaced
*/
void cmd_preprocess(char *str) {
    if (!str)
        return;

    const int len = strlen(str);

    for (int i = len - 1; i >= 0 && str[i] <= ' '; str[i--] = '\0');
    for (int i = 0; i < len; ++i)
        if (str[i] == ' ')
            str[i] = '\0';
}

/*
* Sequentially searches an array list for a command with a given name
* 
* key  - name of the command to find
* list - pointer to the arraylist to look in
* 
* returns - pointer to the found command (NULL if nothing was found)
*/
command_t *find_subcommand(const char *key, const ptr_arraylist_t *list) {
    for (uint j = 0; j < list->count; ++j) {
        DEBUG_ONLY(printf("[INFO] compare: %s - %s\n", key, ((command_t *)list->arr[j])->name));
        if (str_eq(key, ((command_t *)list->arr[j])->name)) {
            DEBUG_ONLY(puts("[INFO] equal!"));
            return (command_t *)list->arr[j];
        }
    }
    return NULL;
}

/*
* Helper function of cmd_register()
* Checks if parts of the given command already exist in the tree
* 
* cmd_str - see cmd_register() description
* cmd_map - a command hashmap where the existence of the command is to be checked
* 
* returns - a struct of:
*           - a pointer to the location in cmd_str where parsing should start
*           - a pointer to the new command's parent command (NULL if there is none)
*/
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
        if (cmd_str[i] == '\0' && cmd_str[i + 1] != '<') {
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

/*
* Adds a command to the registered command tree
* This is what should be called from the main program to create new commands
* 
* cmd_str     - a c-string that specifies how calls to the command should look
* action      - pointer to a (void (arg_bundle_t *)) function that will be called when the command is run
* static_data - a pointer that will be available under bundle->static_data inside the given function
* 
* returns - whether the command was properly added
* 
* IMPORTANT: currently cmd_str should have a space, CR, LF or a similar blank char at the end
*            otherwise this function will most likely crash the program
*/
bool cmd_register(const char *cmd_str, cmd_act_t action, const void *static_data) {
    if (global_command_map.map == NULL)
        global_command_map = cmd_map_make();

    DEBUG_ONLY(printf("[INFO] REGISTER START (%s)\n", cmd_str);)

    char *str = _strdup(cmd_str);
    cmd_preprocess(str);
    const cmd_tree_location_t loc = cmd_skip_existent(str, &global_command_map);
    const cmd_proc_t proc = { .action = action, .static_data = (void *)static_data };

    if (loc.parent == NULL) {
        // a completely new command - add it to the global hashmap
        command_t cmd = cmd_make(loc.ptr, proc);
        free(str);
        DEBUG_ONLY(printf("[INFO] REGISTER FINISH (%s)\n", cmd_str);)
        return cmd_map_add(&global_command_map, &cmd);
    }
    else {
        // parts of this commands already exist
        // skip them and add a new subcommand in the tree
        command_t *cmd = cmd_alloc(loc.ptr, proc);
        free(str);
        DEBUG_ONLY(printf("[INFO] REGISTER FINISH (%s)\n", cmd_str);)
        return arraylist_push(&loc.parent->subcommands, cmd);
    }
}

/*
* Helper function of cmd_execute()
* Determines the state machine's new state
* 
* cur_cmd     - pointer to the currently evaluated command
* args_parsed - how many arguments of cur_cmd have been properly parsed
* 
* returns - the next state for cmd_execute's state machine
*/
parser_state_t next_state(const command_t *cur_cmd, uint args_parsed) {
    if (!cur_cmd)
        // the command doesn't exist
        return ERROR;
    if (cur_cmd->arg_cnt > args_parsed) {
        if (cur_cmd->syntax[args_parsed]->format[0] == '>')
            // if the next argument is a subcommand
            return COMMAND_EXPECTED;  
        // otherwise
        return VALUE_EXPECTED;
    }
    if (cur_cmd->arg_cnt == args_parsed)
        // all arguments have been found and parsed
        return READY;
    // something went wrong
    return UNKNOWN;
}

/*
* Helper function of cmd_execute()
* Adds a parsed argument to the arg bundle
* Responsible for the special handling of string-type arguments
* 
* args - pointer to the arg bundle the argument should be added to
* data - pointer to the argument's raw data
* syntax_node - element from a command's syntax array that corresponds to the argument
*/
void bundle_push(arg_bundle_t *args, const uchar *data, const arg_node_t *syntax_node) {
    if (str_eq(syntax_node->key, "<STRING>")) {
        char *str = _strdup((const char *)data);
        arg_bundle_add_(args, &str, syntax_node->size, true);
    }
    else
        arg_bundle_add_(args, data, syntax_node->size, false);
}

/*
* Runs a command based on a given string
* This is what should be called to run a command from the main program
* 
* cmd_str - c-string to be parsed and run
* 
* returns - whether a command was executed
*/
bool cmd_execute(const char *cmd_str) {
    tokenized_str_t input = tok_str_make(cmd_str, ' ');
    arg_bundle_t args = arg_bundle_make();
    const char *cur_token = (const char *)tok_str_get(&input, 0), *last_search = cur_token;
    const command_t *cur_cmd = cmd_map_find(&global_command_map, cur_token);
    uint args_parsed = 0;
    parser_state_t state = next_state(cur_cmd, args_parsed);
    uchar buffer[512];

    // state machine based string parsing
    for (uint i = 1; i <= input.parts.count; ++i) {
        if (i < input.parts.count)
            cur_token = (const char *)tok_str_get(&input, i);
        else if (state == COMMAND_EXPECTED || state == VALUE_EXPECTED) {
            // the final iteration; check if the given string terminated too soon
            INTERACTIVE_ONLY(printf("[ERROR] Missing argument %u for %s\n", args_parsed + 1, cur_cmd->name));
            state = ERROR;
        }

        switch (state) {
        case COMMAND_EXPECTED:
            // check if current token is a valid subcommand
            cur_cmd = (const command_t *)find_subcommand((last_search = cur_token), &cur_cmd->subcommands);
            args_parsed = 0;
            state = next_state(cur_cmd, args_parsed);
            break;
        case VALUE_EXPECTED:
            // attempt to parse current token as specified type
            if (sscanf(cur_token, cur_cmd->syntax[args_parsed]->format, (void *)buffer) > 0) {
                // if successful, store the result's raw bytes in an arg bundle
                bundle_push(&args, buffer, cur_cmd->syntax[args_parsed++]);
                state = next_state(cur_cmd, args_parsed);
            }
            else {
                INTERACTIVE_ONLY(printf("[ERROR] Non-parseable token '%s' given for argument of type %s\n",
                    cur_token, cur_cmd->syntax[args_parsed]->key));
                state = ERROR;
            }
            break;
        case READY:
            // command is valid and all arguments provided, run it
            args.static_data = cur_cmd->action.static_data;
            (*cur_cmd->action.action)(&args);
            /* FALLTHROUGH */
        case ERROR:
            INTERACTIVE_ONLY(if (!cur_cmd) printf("[ERROR] Unknown command '%s'\n", last_search));
            // clean memory up after a command runs or an error occurs
            tok_str_destroy(&input);
            arg_bundle_destroy(&args);
            return (state == READY);
        case UNKNOWN:
            // this code should never run
            INTERACTIVE_ONLY(puts("[ERROR] UNKNOWN state detected!"));
            return false;
        }
    }

    // neither should this
    INTERACTIVE_ONLY(puts("[ERROR] How did we get here?"));
    return false;
}

/*
* Recursive command printing function used by cmd_dumpall()
* 
*  cmd   -  pointer to a command to be printed out
*  depth -  current depth of recursion, used to determine indent size
* 
* The macro cmd_print(cmd) can be used to print a single command and its subtree 
*/
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

// Dumps the entire command tree to stdout in the format:
// COMMAND_NAME <ARGUMENT> <LIST> -> calls FUNCTION_ADDRESS(STATIC_DATA)
void cmd_dumpall(void) {
    for (uint i = 0; i < global_command_map.size; ++i) {
        if (global_command_map.map[i].name != NULL) {
            putchar('\n');
            cmd_print(global_command_map.map + i);
        }
    }
    putchar('\n');
}
