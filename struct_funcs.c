#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "struct_funcs.h"
#include "cmd_storage.h"

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
    //  ret ^= hash(cmd->syntax[i]->key);

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

cmd_map_t cmd_map_make(void) {
    cmd_map_t ret = {
        .size = CONTAINER_INIT_SIZE,
        .count = 0,
        .map = calloc(CONTAINER_INIT_SIZE, sizeof(command_t)),
    };

    return ret;
}

void cmd_map_destroy(cmd_map_t *map) {
    if (!map || !map->map)
        return;

    for (uint i = 0; i < map->size; ++i) {
        if ((map->map + i) != NULL)
            cmd_destroy(map->map + i);
    }

    free(map->map);
    memset(map, 0, sizeof(*map));
}

bool cmd_map_add(cmd_map_t *map, const command_t *cmd) {
    uint map_index = cmd_hash(cmd) % map->size;

    if (!map || !map->map || !cmd)
        return false;

    if (map->count == map->size) {
        command_t *prev_map = map->map;
        if (!(map->map = calloc(2 * map->size, sizeof(command_t))))
            return false;
        map->count = 0;
        map->size *= 2;

        for (uint i = 0; i < map->size / 2; ++i) {
            if (prev_map[i].name != NULL)
                cmd_map_add(map, prev_map + i);
        }

        free(prev_map);
    }

    while (map->map[map_index].name != NULL)
        map_index = (map_index + 1) % map->size;

    map->map[map_index] = *cmd;
    map->count++;

    return true;
}

const command_t *cmd_map_find(const cmd_map_t *map, const char *key) {
    if (!map->map || map->count == 0)
        return NULL;

    const uint base_index = hash(key) % map->size;
    uint map_index = base_index;

    while (map->map[map_index].name != NULL && !str_eq(map->map[map_index].name, key)) {
        map_index++;
        if ((map_index %= map->size) == base_index)
            break;
    }

    return map->map[map_index].name ? map->map + map_index : NULL;
}

ptr_arraylist_t arraylist_make(destroy_func_t elem_destr_func) {
    ptr_arraylist_t ret = {
        .arr = calloc(CONTAINER_INIT_SIZE, sizeof(void *)),
        .count = 0,
        .size = CONTAINER_INIT_SIZE,
        .elem_destr_func = elem_destr_func,
    };

    return ret;
}

bool arraylist_push(ptr_arraylist_t *list, void *item) {
    if (list->count == list->size) {
        list->size *= 2;
        void **new_arr = realloc(list->arr, list->size * sizeof(void *));
        if (list->size == 0 || !new_arr)
            return false;
        list->arr = new_arr;
    }

    //debug_only(
    //command_t *cmd = (command_t *)item;
    //printf("adding '%s' to arraylist at %p\n", cmd->name, list);
    //)

    list->arr[list->count++] = item;

    return true;
}

void arraylist_destroy(ptr_arraylist_t *list) {
    if (!list || !list->arr)
        return;

    if (list->elem_destr_func != NULL) {
        for (uint i = 0; i < list->count; ++i)
            (*list->elem_destr_func)(list->arr[i]);
    }

    free(list->arr);
    list->count = list->size = 0;
}

byte_arraylist_t byte_arraylist_make(void) {
    byte_arraylist_t ret = {
        .arr = calloc(CONTAINER_INIT_SIZE, sizeof(uchar)),
        .count = 0,
        .size = CONTAINER_INIT_SIZE,
    };

    return ret;
}

bool byte_arraylist_push(byte_arraylist_t *list, uchar item) {
    if (list->count == list->size) {
        list->size *= 2;
        uchar *new_arr = realloc(list->arr, list->size * sizeof(uchar));
        if (list->size == 0 || !new_arr)
            return false;
        list->arr = new_arr;
    }

    list->arr[list->count++] = item;

    return true;
}

void byte_arraylist_destroy(byte_arraylist_t *list) {
    if (!list || !list->arr)
        return;

    free(list->arr);
    list->count = list->size = 0;
}


tokenized_str_t tok_str_make(const char *str, char delim) {
    tokenized_str_t ret = {
        .str = _strdup(str),
        .parts = arraylist_make(NULL),
    };

    if (!ret.str)
        return ret;

    arraylist_push(&ret.parts, ret.str);
    for (uint i = 0; ret.str[i]; ++i) {
        if (ret.str[i] == delim) {
            ret.str[i] = '\0';
            arraylist_push(&ret.parts, ret.str + i + 1);
        }
    }

    return ret;
}

char *tok_str_get(tokenized_str_t *tok_str, uint index) {
    if (!tok_str || index >= tok_str->parts.count)
        return NULL;

    return (char *)(tok_str->parts.arr[index]);
}

void tok_str_destroy(tokenized_str_t *tok_str) {
    if (!tok_str || !tok_str->str)
        return;

    arraylist_destroy(&tok_str->parts);
    free(tok_str->str);
}

arg_bundle_t arg_bundle_make(void) {
    arg_bundle_t ret = {
        .args = arraylist_make(NULL),
        .data = byte_arraylist_make(),
        .index = 0,
    };

    return ret;
}

bool arg_bundle_add_(arg_bundle_t *bundle, void *src, uint size) {
    if (!bundle || !src || size == 0)
        return false;

    arraylist_push(&bundle->args, (void *)bundle->data.count);
    for (uint i = 0; i < size; ++i) {
        byte_arraylist_push(&bundle->data, ((uchar *)src)[i]);
    }

    return true;
}

uint arg_bundle_get_(arg_bundle_t *bundle, void *dst, uint size) {
    if (!bundle || !dst || size == 0)
        return 0;
    if (bundle->index == bundle->args.count) {
        arg_bundle_destroy(bundle);
        return 0;
    }

    const uchar *src = bundle->data.arr + (uint)bundle->args.arr[bundle->index++];
    const uint iter_limit = (bundle->data.arr + bundle->data.count) - src;

    for (uint i = 0; i < size && i < iter_limit; ++i) {
        ((uchar *)dst)[i] = src[i];
    }

    return min(size, iter_limit);
}

void *arg_bundle_get_raw_(arg_bundle_t *bundle) {
    if (!bundle)
        return NULL;
    if (bundle->index == bundle->args.count) {
        arg_bundle_destroy(bundle);
        return NULL;
    }

    return bundle->data.arr + (uint)bundle->args.arr[bundle->index++];
}

void arg_bundle_destroy(arg_bundle_t *bundle) {
    arraylist_destroy(&bundle->args);
    byte_arraylist_destroy(&bundle->data);
}
