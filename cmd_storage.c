#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd_storage.h"

#pragma warning (disable: 5045)

const arg_node_t *size_node_get(const char *key) {
	static const arg_node_t nodes[] = {
		{ "<SUBCMD>",	">",		0				},
		{ "<VOID>",		"$null",	0				},
		{ "<CHAR>",		"%c",		sizeof(char)	},
		{ "<BYTE>",		"%hh",		sizeof(char)	},
		{ "<UBYTE>",	"%hhu",		sizeof(uchar)	},
		{ "<SHORT>",	"%h",		sizeof(short)	},
		{ "<USHORT>",	"%hu",		sizeof(ushort)	},
		{ "<INT>",		"%d",		sizeof(int)		},
		{ "<UINT>",		"%u",		sizeof(uint)	},
		{ "<LONG>",		"%ld",		sizeof(long)	},
		{ "<ULONG>",	"%lu",		sizeof(ulong)	},
		{ "<LLONG>",	"%lld",		sizeof(llong)	},
		{ "<ULLONG>",	"%llu",		sizeof(ullong)	},
		{ "<STRING>",	"%s",		sizeof(char *)	},
		{ "<PTR>",		"%p",		sizeof(void *)	},
		{ "<CMD>",		"$cmd",		sizeof(void *)	},
	};

	if (key[0] != '<')
		return nodes + 0;

	for (uint i = 1; i < (sizeof(nodes) / sizeof(nodes[0])); ++i) 
		if (str_eq(key, nodes[i].key)) 
			return nodes + i;
		
	return nodes + 1;
}

void cmd_merge_subcmd(ptr_arraylist_t *list, command_t *cmd) {
	debug_only(printf("Merge called for %s\n", cmd->name);)
	arraylist_push(list, cmd);
}

void cmd_syntax_parse(char *args, command_t *cmd) {

	for (uint i = 0, j = 0; i < cmd->arg_cnt; ++i) {
		cmd->syntax[i] = size_node_get(args + j);
		while (args[j++] != '\0');
		if (cmd->syntax[i]->format[0] == '>') {
			cmd_proc_t action = cmd->action;
			command_t *subcommand = malloc(sizeof(command_t));

			cmd->action = NULL;
			if (subcommand)
				*subcommand = cmd_make(args, action);
			cmd_merge_subcmd(&cmd->subcommands, subcommand);
		}
	}
}

command_t *cmd_alloc(const char *input, cmd_proc_t proc) {
	command_t *ret = malloc(sizeof(command_t));
	if (ret) {
		*ret = cmd_make(input, proc);
		ret->is_dynamic_memory = true;
	}
	return ret;
}

//command_t cmd_make(const char *input, cmd_proc_t proc) {
//	command_t ret = { 0 };
//	extern cmd_map_t global_command_map;
//	char *str = _strdup(input);
//	
//	cmd_preprocess(str);
//	ret = cmd_parse(cmd_skip_existent(str, &global_command_map), proc);
//	free(str);
//	return ret;
//}

command_t cmd_make(char *str, cmd_proc_t proc) {
	command_t ret = { 0 };
	char *args = str;

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

