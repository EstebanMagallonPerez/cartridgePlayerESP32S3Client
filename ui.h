#pragma once
#include <Arduino.h>

void ui_init();
void ui_render_loop();
void ui_full_refresh();
void ui_set_outside(int temp, char icon);
void ui_set_time_from_epoch(long long epoch_ms);
void ui_change_fan_mode();
void ui_change_hvac_mode();
void ui_inc_target_temp();
void ui_dec_target_temp();
void ui_handle_inactivity();
void ui_mark_dirty();
