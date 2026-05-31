#pragma once
#include <Arduino.h>

void ui_init();
void ui_render_loop();
void ui_full_refresh();
void ui_setText(const char* text);
void ui_handle_inactivity();
void ui_mark_dirty();
void ui_updateNowPlayingTracks(String titles[], int selectedIndex);
void ui_updateArtwork(uint8_t * buf);
