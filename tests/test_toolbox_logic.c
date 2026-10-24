#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "toolbox_logic.h"

static void test_score(void)
{
    char buf[16];

    tb_format_score(1525.726598f, buf, sizeof(buf));
    assert(strcmp(buf, "1525.7") == 0);

    tb_format_score(0.209182f, buf, sizeof(buf));
    assert(strcmp(buf, "0.21") == 0);

    tb_format_score(68.0f, buf, sizeof(buf));
    assert(strcmp(buf, "68") == 0);

    tb_format_score(9.5f, buf, sizeof(buf));
    assert(strcmp(buf, "9.50") == 0);
}

static void test_bars(void)
{
    assert(tb_bar_height(0, 100, 48, 3) == 0);
    assert(tb_bar_height(50, 100, 48, 3) == 24);
    assert(tb_bar_height(1, 100, 48, 3) == 3);
    assert(tb_bar_height(1000, 100, 48, 3) == 48);

    const int values[] = {3, 9, 1, 7};
    assert(tb_max_int(values, 4) == 9);
    assert(tb_max_int(NULL, 0) == 0);
}

static void test_dates(void)
{
    char buf[8];
    tb_short_date("2026-10-23", buf, sizeof(buf));
    assert(strcmp(buf, "10-23") == 0);

    tb_short_date("2026-10-23T16:00:00Z", buf, sizeof(buf));
    assert(strcmp(buf, "10-23") == 0);

    tb_short_date("bad", buf, sizeof(buf));
    assert(strcmp(buf, "—") == 0);
}

static void test_clamp(void)
{
    assert(tb_clamp_int(-5, 0, 100) == 0);
    assert(tb_clamp_int(42, 0, 100) == 42);
    assert(tb_clamp_int(500, 0, 100) == 100);
}

int main(void)
{
    test_score();
    test_bars();
    test_dates();
    test_clamp();
    puts("toolbox logic tests: PASS");
    return 0;
}
