#!/usr/bin/env python3
"""Regression checks for the physical-button and scaled-icon UI contract."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "main" / "worry_seed_app.c").read_text(encoding="utf-8")
AUDIO_SOURCE = (ROOT / "main" / "worry_seed_audio.c").read_text(encoding="utf-8")
AUDIO_HEADER = (ROOT / "main" / "worry_seed_audio.h").read_text(encoding="utf-8")
POWER_HEADER = (ROOT / "main" / "worry_seed_power_policy.h").read_text(encoding="utf-8")
BSP_AUDIO_SOURCE = (ROOT / "components" / "bsp" / "src" / "bsp_audio.c").read_text(encoding="utf-8")


def function_body(name: str, next_name: str) -> str:
    start = SOURCE.index(name)
    end = SOURCE.index(next_name, start)
    return SOURCE[start:end]


class WorrySeedAppContractTests(unittest.TestCase):
    def test_up_down_navigation_uses_low_latency_press_event(self):
        body = function_body("static void handle_button", "static void handle_audio")
        self.assertIn("button == BSP_BTN_UP || button == BSP_BTN_DOWN", body)
        self.assertIn("action != BSP_BTN_PRESS", body)

    def test_existing_seed_ok_is_not_shadowed_by_navigation_branch(self):
        body = function_body("static void handle_garden", "static void handle_category")
        ok = body.index("action == BSP_BTN_CLICK && button == BSP_BTN_OK")
        navigation = body.index("action == BSP_BTN_CLICK && s.model.count")
        self.assertLess(ok, navigation)

    def test_scaled_icons_are_positioned_by_unscaled_pivot(self):
        body = function_body("static lv_obj_t *theme_icon_centered", "static void draw_tree")
        self.assertIn("center_x - 20", body)
        self.assertIn("center_y - 20", body)

    def test_requested_growth_copy_is_used_consistently(self):
        self.assertNotIn("陪伴", SOURCE)
        self.assertIn("浇灌", SOURCE)
        self.assertIn("埋下一颗烦恼，等它成长", SOURCE)
        self.assertIn("恭喜！旧烦恼变为成长的果实", SOURCE)
        self.assertNotIn("说出来，让它慢慢长大", SOURCE)
        self.assertIn("说出来，埋入土里", SOURCE)

    def test_fruited_state_uses_a_celebration_melody(self):
        start = AUDIO_SOURCE.index("case WORRY_AUDIO_TONE_FRUIT:")
        end = AUDIO_SOURCE.index("case WORRY_AUDIO_TONE_BUTTON:", start)
        fruit = AUDIO_SOURCE[start:end]
        self.assertGreaterEqual(fruit.count("write_note("), 7)
        self.assertIn("write_note(0,", fruit)

    def test_microphone_gain_is_raised_for_quiet_recordings(self):
        self.assertIn("esp_codec_dev_set_in_gain(s_dev, 42.0f);", BSP_AUDIO_SOURCE)
        self.assertNotIn("esp_codec_dev_set_in_gain(s_dev, 36.0f);", BSP_AUDIO_SOURCE)

    def test_recording_playback_uses_full_output_volume(self):
        self.assertIn("bsp_audio_set_volume(100);", AUDIO_SOURCE)
        self.assertNotIn("bsp_audio_set_volume(90);", AUDIO_SOURCE)

    def test_manual_playback_stop_is_not_reported_as_natural_completion(self):
        self.assertIn("WORRY_AUDIO_EVENT_PLAYBACK_STOPPED", AUDIO_HEADER)
        self.assertIn("s_stop ? WORRY_AUDIO_EVENT_PLAYBACK_STOPPED", AUDIO_SOURCE)
        self.assertIn("WORRY_AUDIO_EVENT_PLAYBACK_STOPPED", SOURCE)
        self.assertIn('set_message("已停止播放")', SOURCE)

    def test_latest_playback_status_fits_the_240px_safe_area(self):
        self.assertNotIn("正在播放最近的进展，确定可停止", SOURCE)
        self.assertIn("正在播放进展，确定停止", SOURCE)
        self.assertLessEqual(len("正在播放进展，确定停止"), 12)

    def test_idle_power_stages_match_the_product_contract(self):
        self.assertIn("WORRY_POWER_DIM_AFTER_MS       30000ULL", POWER_HEADER)
        self.assertIn("WORRY_POWER_SCREEN_OFF_AFTER_MS 60000ULL", POWER_HEADER)
        self.assertIn("WORRY_POWER_DEEP_SLEEP_AFTER_MS 300000ULL", POWER_HEADER)
        self.assertIn("worry_seed_audio_busy()", SOURCE)

    def test_black_screen_wake_is_consumed_before_button_audio_or_action(self):
        task = function_body("static void app_task", "esp_err_t worry_seed_app_start")
        wake = task.index("consume_wake_button")
        beep = task.index("worry_seed_audio_beep", wake)
        action = task.index("handle_button", wake)
        self.assertLess(wake, beep)
        self.assertLess(wake, action)

    def test_deep_sleep_uses_the_shared_adc_button_gpio_as_a_mask(self):
        body = function_body("static void enter_deep_sleep", "static void update_power_state")
        self.assertIn("1ULL << BSP_BTN_GPIO", body)
        self.assertIn("ESP_GPIO_WAKEUP_GPIO_LOW", body)
        self.assertIn("bsp_battery_sleep()", body)
        self.assertIn("bsp_audio_prepare_deep_sleep()", body)
        self.assertIn("bsp_i2c_prepare_deep_sleep()", body)
        self.assertIn("bsp_display_prepare_deep_sleep()", body)

    def test_deep_sleep_retains_last_valid_battery_soc(self):
        self.assertIn("RTC_DATA_ATTR static int s_retained_battery_soc", SOURCE)
        sleep_body = function_body("static void enter_deep_sleep", "static void update_power_state")
        self.assertIn("bsp_battery_soc()", sleep_body)
        self.assertIn("s_retained_battery_soc", sleep_body)
        screen_body = function_body("static lv_obj_t *screen_base", "static void footer")
        self.assertRegex(screen_body, r"soc == 0\s*&&\s*s_retained_battery_soc > 0")
        start_body = SOURCE[SOURCE.index("esp_err_t worry_seed_app_start"):]
        self.assertIn("esp_sleep_get_wakeup_cause()", start_body)


if __name__ == "__main__":
    unittest.main()
