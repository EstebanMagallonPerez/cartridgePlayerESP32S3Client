#include "buttons.h"

#define BUTTON_COUNT 6
#define BTN_LEFT 19
#define BTN_LEFT_MIDDLE 17
#define BTN_MIDDLE 15
#define BTN_RIGHT_MIDDLE 9
#define BTN_RIGHT 3
#define BTN_HIDDEN 5

static int button_pins[BUTTON_COUNT] = { BTN_LEFT, BTN_LEFT_MIDDLE, BTN_MIDDLE, BTN_RIGHT_MIDDLE, BTN_RIGHT, BTN_HIDDEN};
static bool button_last_state[BUTTON_COUNT];

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
            Serial.print(i);
        }
        button_last_state[i] = state;
    }
}

