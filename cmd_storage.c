#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd_storage.h"
#include "struct_funcs.h"

#pragma warning (disable: 5045)

uint rol(uint num, uchar dist) {
	return (num << dist) | (num >> (8 * sizeof(num) - dist));
}

uint hash(const char *str) {
	uint hash = 0x539CA32B;

	for (uchar i = 0; str[i]; ++i) {
		hash = rol(hash, 3 + hash % 8);
		hash ^= str[i];
		hash ^= rol(hash, 4 + hash % 6);
	}

	return hash;
}

uint cmd_hash(const command_t *cmd) {
	uint ret = hash(cmd->name);
	//for (uint i = 0; i < cmd->arg_cnt; ++i)
	//	ret ^= hash(cmd->syntax[i]->key);

	return ret;
}

bool cmd_eq(const command_t *cmd1, const command_t *cmd2) {
	if (!(str_eq(cmd1->name, cmd2->name) && cmd1->arg_cnt == cmd2->arg_cnt))
		return false;
	for (uint i = 0; i < cmd1->arg_cnt; ++i)
		if (cmd1->syntax[i]->key != cmd2->syntax[i]->key)
			return false;

	return true;
}

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

void cmd_skip_existent(char *cmd_str, const cmd_map_t *merge_into) {
	if (!cmd_str || !merge_into)
		return;

	command_t *cur = NULL;

}

void cmd_preprocess(char *str) {
	const int len = strlen(str);

	for (int i = len - 1; i >= 0 && str[i] <= ' '; str[i--] = '\0');
	for (int i = 0; i < len; ++i)
		if (str[i] == ' ')
			str[i] = '\0';
}

void cmd_merge_subcmd(ptr_arraylist_t *list, command_t *cmd) {
	printf("Merge called for %s\n", cmd->name);
	arraylist_push(list, cmd);
}

void cmd_syntax_parse(const char *args, command_t *cmd) {

	for (uint i = 0, j = 0; i < cmd->arg_cnt; ++i) {
		cmd->syntax[i] = size_node_get(args + j);
		while (args[j++] != '\0');
		if (cmd->syntax[i]->format[0] == '>') {
			cmd_proc_t action = cmd->action;
			command_t *subcommand = malloc(sizeof(command_t));

			cmd->action = NULL;
			if (subcommand)
				*subcommand = cmd_parse(args, action);
			cmd_merge_subcmd(&cmd->subcommands, subcommand);
		}
	}
}

command_t cmd_make(const char *input, cmd_proc_t proc, const cmd_map_t *merge_into) {
	char *str = _strdup(input);

	cmd_preprocess(str);

	return cmd_parse(str, proc);
}

command_t cmd_parse(char *str, cmd_proc_t proc) {
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

	ret.name = str;
	ret.syntax = ret.arg_cnt ? malloc(ret.arg_cnt * sizeof(arg_node_t *)) : NULL;
	ret.action = proc;
	ret.subcommands = arraylist_make(&cmd_destroy);
	cmd_syntax_parse(args + 1, &ret);

	return ret;
}

void cmd_destroy(command_t *cmd) {
	if (!cmd) return;
	if (cmd->name)
		free(cmd->name);
	if (cmd->syntax)
		free(cmd->syntax);
	arraylist_destroy(&cmd->subcommands);
}




/*
if (args = strchr(str, ' ')) {
	ret.arg_cnt = 1;
	args[0] = '\0';
	for (uint i = 1; args[i]; ++i) {
		if (args[i] == ' ') {
			args[i] = '\0';
			ret.arg_cnt++;
			if (args[i + 1] != '<')
				break;
		}
	}
}
*/