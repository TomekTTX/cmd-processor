#pragma once
#include <stdbool.h>

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;
typedef unsigned long long	ullong;
typedef          long long	llong;

typedef void (*cmd_proc_t)(void *);

typedef struct arg_node_t_ {
	const char *key, *format;
	uint size;
} arg_node_t;


typedef struct command_t_ {
	cmd_proc_t action;
	char *name;
	arg_node_t **syntax;
	uint arg_size, arg_cnt;
} command_t;

bool str_eq(const char *s1, const char *s2);

bool cmd_register(const char *cmd_str, cmd_proc_t action);
