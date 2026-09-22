#include "worry_seed_debug_protocol.h"

#include <assert.h>
#include <stdio.h>

static void expect_button(const char *line, worry_debug_key_t key,
                          worry_debug_action_t action) {
    worry_debug_command_t command;
    assert(worry_debug_protocol_parse(line, &command));
    assert(command.type == WORRY_DEBUG_COMMAND_BUTTON);
    assert(command.key == key);
    assert(command.action == action);
}

int main(void) {
    worry_debug_command_t command;

    assert(worry_debug_protocol_parse("FAP_SCREENSHOT_V1", &command));
    assert(command.type == WORRY_DEBUG_COMMAND_SCREENSHOT);

    expect_button("FAP_KEY_V1 UP CLICK", WORRY_DEBUG_KEY_UP,
                  WORRY_DEBUG_ACTION_CLICK);
    expect_button("FAP_KEY_V1 DOWN LONG", WORRY_DEBUG_KEY_DOWN,
                  WORRY_DEBUG_ACTION_LONG);
    expect_button("FAP_KEY_V1 OK CLICK", WORRY_DEBUG_KEY_OK,
                  WORRY_DEBUG_ACTION_CLICK);
    expect_button("FAP_KEY_V1 OK LONG", WORRY_DEBUG_KEY_OK,
                  WORRY_DEBUG_ACTION_LONG);

    assert(!worry_debug_protocol_parse(NULL, &command));
    assert(!worry_debug_protocol_parse("FAP_KEY_V1 UP", &command));
    assert(!worry_debug_protocol_parse("FAP_KEY_V1 up CLICK", &command));
    assert(!worry_debug_protocol_parse("FAP_KEY_V1 OK DOUBLE", &command));
    assert(!worry_debug_protocol_parse("FAP_KEY_V1 OK CLICK EXTRA", &command));
    assert(!worry_debug_protocol_parse("garbage", &command));
    assert(!worry_debug_protocol_parse("FAP_SCREENSHOT_V1 EXTRA", &command));
    assert(!worry_debug_protocol_parse("FAP_KEY_V1 OK CLICK", NULL));

    puts("Worry-seed debug protocol tests: PASS");
    return 0;
}
