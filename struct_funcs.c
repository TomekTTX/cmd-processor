#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "struct_funcs.h"
#include "cmd_storage.h"

#pragma warning (disable: 5045)

/*
* Rotates bits of a 32-bit unsigned value to the left
* 
* num -  the 32-bit number
* dist - rotation distance
* 
* returns - result of the rotation
*/
uint rol(uint num, uchar dist) {
    return (num << dist) | (num >> (8 * sizeof(num) - dist));
}

/*
* Calculates a hash of a string for hashmap purposes
* 
* str - the string to be hashed
* 
* returns - calculated hash value
*/
uint hash(const char *str) {
    uint hash = 0x539CA32B;

    for (uchar i = 0; str[i]; ++i) {
        hash = rol(hash, 3 + hash % 8);
        hash ^= str[i];
        hash ^= rol(hash, 4 + hash % 6);
    }

    return hash;
}

// Likely temporary
// Returns the hash of cmd's name
uint cmd_hash(const command_t *cmd) {
    uint ret = hash(cmd->name);
    //for (uint i = 0; i < cmd->arg_cnt; ++i)
    //  ret ^= hash(cmd->syntax[i]->key);

    return ret;
}

/*
* Checks if two commands are equal
* 
* cmd1 - command 1
* cmd2 - command 2
* 
* returns - whether cmd1 is equal to cmd2
*/
bool cmd_eq(const command_t *cmd1, const command_t *cmd2) {
    if (!(str_eq(cmd1->name, cmd2->name) && cmd1->arg_cnt == cmd2->arg_cnt))
        return false;
    for (uint i = 0; i < cmd1->arg_cnt; ++i)
        if (cmd1->syntax[i]->key != cmd2->syntax[i]->key)
            return false;

    return true;
}

/*
* Returns a stack-allocated command hashmap struct
*/
cmd_map_t cmd_map_make(void) {
    cmd_map_t ret = {
        .size = CONTAINER_INIT_SIZE,
        .count = 0,
        .map = calloc(CONTAINER_INIT_SIZE, sizeof(command_t)),
    };

    return ret;
}

/*
* Frees all memory allocated by cmd_map_make()
* 
* map - the command hashmap to be deleted
*/
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

/*
* Adds a command to a hashmap
* 
* map - pointer to the map where the command is to be added
* cmd - pointer to the addedd command
* 
* returns - whether the command was successfully added
*/
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

/*
* Searches a hashmap for a command with given name
* 
* map - pointer to the hashmap to search in
* key - the name to look for
* 
* returns - a pointer to a command with the given name (NULL if there is none)
*/
const command_t *cmd_map_find(const cmd_map_t *map, const char *key) {
    if (!map->map || map->count == 0)
        return NULL;

    const uint base_index = hash(key) % map->size;
    uint map_index = base_index;

    while (map->map[map_index].name != NULL && !str_eq(map->map[map_index].name, key)) {
        map_index++;
        if ((map_index %= map->size) == base_index)
            return NULL;
    }

    return map->map[map_index].name ? map->map + map_index : NULL;
}

/*
* Creates a stack-allocated arraylist of pointers
* 
* elem_destr_func - a (void (void *)) function to be applied to each
*                   element of the arraylist when it is destroyed
* 
* returns - the newly created arraylist
*/
ptr_arraylist_t arraylist_make(destroy_func_t elem_destr_func) {
    ptr_arraylist_t ret = {
        .arr = calloc(CONTAINER_INIT_SIZE, sizeof(void *)),
        .count = 0,
        .size = CONTAINER_INIT_SIZE,
        .elem_destr_func = elem_destr_func,
    };

    return ret;
}

/*
* Adds a pointer to an arraylist
* 
* list - the list where the pointer is to be added
* item - the pointer to add
* 
* returns - whether the pointer was successfully added
*/
bool arraylist_push(ptr_arraylist_t *list, void *item) {
    if (list->count == list->size) {
        list->size *= 2;
        void **new_arr = realloc(list->arr, list->size * sizeof(void *));
        if (list->size == 0 || !new_arr)
            return false;
        list->arr = new_arr;
    }

    list->arr[list->count++] = item;

    return true;
}

/*
* Frees all memory allocated by arraylist_make()
*
* list - the arraylist to be deleted
*/
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

/*
* Creates a stack-allocated arraylist of bytes
*
* returns - the newly created arraylist
*/
byte_arraylist_t byte_arraylist_make(void) {
    byte_arraylist_t ret = {
        .arr = calloc(CONTAINER_INIT_SIZE, sizeof(uchar)),
        .count = 0,
        .size = CONTAINER_INIT_SIZE,
    };

    return ret;
}

/*
* Adds a byte to an arraylist
*
* list - the list where the byte is to be added
* item - the byte to add
*
* returns - whether the byte was successfully added
*/
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

/*
* Frees all memory allocated by byte_arraylist_make()
*
* list - the arraylist to be deleted
*/
void byte_arraylist_destroy(byte_arraylist_t *list) {
    if (!list || !list->arr)
        return;

    free(list->arr);
    list->count = list->size = 0;
}

/*
* Creates a stack-allocated tokenized string
*
* str   - the string to be tokenized
* delim - the delimiter to cut the string on
* 
* returns - the newly created tokenized string
*/
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
        if (ret.str[i] < ' ')
            ret.str[i] = '\0';
    }

    return ret;
}

/*
* Returns a token from a tokenized string
* 
* tok_str - pointer to the tokenized string
* index   - the index of the token
* 
* returns - pointer to beginning of the token (NULL if index is out of bounds)
*/
char *tok_str_get(tokenized_str_t *tok_str, uint index) {
    if (!tok_str || index >= tok_str->parts.count)
        return NULL;

    return (char *)(tok_str->parts.arr[index]);
}

/*
* Frees all memory allocated by tok_str_make()
* 
* tok_str - pointer to the tokenized string to be deleted
*/
void tok_str_destroy(tokenized_str_t *tok_str) {
    if (!tok_str || !tok_str->str)
        return;

    arraylist_destroy(&tok_str->parts);
    free(tok_str->str);
}

/*
* Creates a stack-allocated argument bundle

* returns - the newly created argument bundle
*/
arg_bundle_t arg_bundle_make(void) {
    arg_bundle_t ret = {
        .args = arraylist_make(NULL),
        .dynamic_blocks = arraylist_make(&free),
        .data = byte_arraylist_make(),
        .index = 0,
        .empty = true,
    };

    return ret;
}

/*
* Pushes a sequence of bytes into an arg bundle
* The macro arg_bundle_add(bundle, data) can be used to easily push a variable
* 
* bundle  - the bundle to push into
* src     - the location to copy from
* size    - number of bytes to push
* dynamic - if the data should be free()'d along with the bundle
* 
* returns - whether the push was successful
*/
bool arg_bundle_add_(arg_bundle_t *bundle, const void *src, uint size, bool dynamic) {
    if (!bundle || !src || size == 0)
        return false;

    arraylist_push(&bundle->args, (void *)bundle->data.count);
    if (dynamic) {
        arraylist_push(&bundle->dynamic_blocks, *(void **)src);
    }
    for (uint i = 0; i < size; ++i) {
        byte_arraylist_push(&bundle->data, ((uchar *)src)[i]);
    }
    bundle->empty = false;

    return true;
}

/*
* Copies the current argument from a bundle to a given location
* Increments the bundle's index
* If the bundle is out of arguments, it is deleted
* The macro arg_bundle_get(bundle, dst) can be used to easily copy to a variable
* 
* bundle - the arg bundle to copy from
* dst    - the location to copy to
* size   - number of bytes to copy
* 
* returns - the number of copied bytes
*/
uint arg_bundle_get_(arg_bundle_t *bundle, void *dst, uint size) {
    if (!bundle || !dst || size == 0)
        return 0;
    if (bundle->index == bundle->args.count) {
        // arg_bundle_destroy(bundle);
        bundle->empty = true;
        return 0;
    }

    const uchar *src = bundle->data.arr + (uint)bundle->args.arr[bundle->index++];
    const uint iter_limit = (bundle->data.arr + bundle->data.count) - src;

    for (uint i = 0; i < size && i < iter_limit; ++i) {
        ((uchar *)dst)[i] = src[i];
    }

    return min(size, iter_limit);
}

/*
* Retrieves the pointer to a bundle's current argument
* Increments the bundle's index
* If the bundle is out of arguments, it is deleted
* The macro arg_bundle_getas(bundle, type) can be used to get the result as a given type
*
* bundle - the arg bundle to get the pointer from
* 
* returns - a pointer to the current argument (pointer to -1 if there are none left)
*/
void *arg_bundle_get_raw_(arg_bundle_t *bundle) {
    static const llong out_of_args = -1;

    if (!bundle)
        return NULL;
    if (bundle->index == bundle->args.count) {
        // arg_bundle_destroy(bundle);
        bundle->empty = true;
        return (void *)&out_of_args;
    }

    return bundle->data.arr + (uint)bundle->args.arr[bundle->index++];
}

/*
* Copies the current argument from a bundle to a given location
* Size is inferred from the next argument's start pointer / end of data array
* Increments the bundle's index
* If the bundle is out of arguments, it is deleted
* 
* bundle - the arg bundle to copy from
* dst_   - the location to copy to
* 
* returns - whether an argument was copied
*/
bool arg_bundle_unpack_one(arg_bundle_t *bundle, void *dst_) {
    if (!bundle || !dst_ || bundle->empty)
        return false;
    if (bundle->index == bundle->args.count) {
        // bad idea, don't do that here
        // arg_bundle_destroy(bundle);
        bundle->empty = true;
        return false;
    }
    const uchar *end_ptr;
    const uchar *src = bundle->data.arr + (uint)bundle->args.arr[bundle->index++];
    uchar *dst = (uchar *)dst_;

    if (bundle->index != bundle->args.count)
        end_ptr = bundle->data.arr + (uint)bundle->args.arr[bundle->index];
    else
        end_ptr = bundle->data.arr + bundle->data.count;

    while (src != end_ptr)
        *dst++ = *src++;

    return true; 
}

/*
* Variadic function that unpacks an arg bundle all at once
* The bundle's content will be sequentially copied to the provided memory locations
* Automatic deletion of the bundle was removed beacause it breaks string-type arguments
* 
* bundle      - the arg bundle to unpack
* static_data - pointer to a pointer where the bundle's static_data field will be copied
* ...         - pointers to variables the contents will be copied to
* (make sure the order of these pointers and variable types are the same as in the command)
* 
* returns - number of copied arguments
*/
uint arg_bundle_unpack(arg_bundle_t *bundle, void **static_data, ...) {
    void *dst = NULL;
    uint ret = 0;
    va_list args;
    va_start(args, static_data);

    if (static_data)
        *static_data = bundle->static_data;
    do {
        dst = va_arg(args, void *);
        ++ret;
    } while (arg_bundle_unpack_one(bundle, dst));

    va_end(args);
    return ret - 1;
}

/*
* Frees all memory allocated by arg_bundle_make()
* Can be safely called on an already deleted bundle
* In such case it doesn't do anything
* 
* bundle - the bundle to be deleted
*/
void arg_bundle_destroy(arg_bundle_t *bundle) {
    if (bundle->args.size != 0)
        arraylist_destroy(&bundle->args);
    if (bundle->dynamic_blocks.size != 0)
        arraylist_destroy(&bundle->dynamic_blocks);
    if (bundle->data.size != 0)
        byte_arraylist_destroy(&bundle->data);
}
