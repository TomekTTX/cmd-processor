#include "cmd_main.h"
#include "cmd_storage.h"
#include <string.h>

static cmd_map_t command_map = { 0 };

bool str_eq(const char *s1, const char *s2) {
	return strcmp(s1, s2) == 0;
}

bool cmd_register(const char *cmd_str, cmd_proc_t action) {
	if (command_map.map == NULL)
		command_map = cmd_map_make();

	command_t cmd = cmd_make(cmd_str, action);

	return cmd_map_add(&command_map, &cmd);
}