#include "buttons.h"
#include "ui.h"

#define BUTTON_COUNT 5
#define BTN_MENU 2
#define BTN_EXIT 1
#define BTN_UP 6
#define BTN_DOWN 4
#define BTN_CONFIRM 5

static int button_pins[BUTTON_COUNT] = { BTN_MENU, BTN_EXIT, BTN_UP, BTN_DOWN, BTN_CONFIRM };
static bool button_last_state[BUTTON_COUNT];

static void (*button_callbacks[BUTTON_COUNT])() = {
    ui_change_fan_mode,
    ui_change_hvac_mode,
    ui_inc_target_temp,
    ui_dec_target_temp,
    ui_full_refresh
};

void buttons_init()
{
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        pinMode(button_pins[i], INPUT_PULLUP);
        button_last_state[i] = digitalRead(button_pins[i]);
    }
}

void buttons_poll()
{
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        bool state = digitalRead(button_pins[i]);
        if (button_last_state[i] == HIGH && state == LOW) {
            if (button_callbacks[i]) button_callbacks[i]();
        }
        button_last_state[i] = state;
    }
}
