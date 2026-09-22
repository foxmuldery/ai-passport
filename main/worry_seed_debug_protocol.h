#pragma once

#include <stdbool.h>

typedef enum {
    WORRY_DEBUG_COMMAND_SCREENSHOT,
    WORRY_DEBUG_COMMAND_BUTTON,
} worry_debug_command_type_t;

typedef enum {
    WORRY_DEBUG_KEY_UP,
    WORRY_DEBUG_KEY_DOWN,
    WORRY_DEBUG_KEY_OK,
} worry_debug_key_t;

typedef enum {
    WORRY_DEBUG_ACTION_CLICK,
    WORRY_DEBUG_ACTION_LONG,
} worry_debug_action_t;

typedef struct {
    worry_debug_command_type_t type;
    worry_debug_key_t key;
    worry_debug_action_t action;
} worry_debug_command_t;

// Parses one complete command line after CR/LF has been removed.
bool worry_debug_protocol_parse(const char *line, worry_debug_command_t *command);
