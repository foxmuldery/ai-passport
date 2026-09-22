#include "worry_seed_debug_protocol.h"

#include <string.h>

bool worry_debug_protocol_parse(const char *line, worry_debug_command_t *command) {
    static const char screenshot[] = "FAP_SCREENSHOT_V1";
    static const char prefix[] = "FAP_KEY_V1 ";
    const char *key;
    const char *action;

    if (line == NULL || command == NULL) return false;

    if (strcmp(line, screenshot) == 0) {
        command->type = WORRY_DEBUG_COMMAND_SCREENSHOT;
        return true;
    }
    if (strncmp(line, prefix, sizeof(prefix) - 1) != 0) return false;

    key = line + sizeof(prefix) - 1;
    if (strncmp(key, "UP ", 3) == 0) {
        command->key = WORRY_DEBUG_KEY_UP;
        action = key + 3;
    } else if (strncmp(key, "DOWN ", 5) == 0) {
        command->key = WORRY_DEBUG_KEY_DOWN;
        action = key + 5;
    } else if (strncmp(key, "OK ", 3) == 0) {
        command->key = WORRY_DEBUG_KEY_OK;
        action = key + 3;
    } else {
        return false;
    }

    if (strcmp(action, "CLICK") == 0) {
        command->action = WORRY_DEBUG_ACTION_CLICK;
    } else if (strcmp(action, "LONG") == 0) {
        command->action = WORRY_DEBUG_ACTION_LONG;
    } else {
        return false;
    }
    command->type = WORRY_DEBUG_COMMAND_BUTTON;
    return true;
}
