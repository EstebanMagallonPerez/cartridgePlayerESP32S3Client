#include <Arduino.h>
#include "wifiManager.h"
#include "buttons.h"
#include "ui.h"
#include "scheduler.h"

void setup()
{
    Serial.begin(115200);

    connectWiFi();
    scheduler_init();

    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

    buttons_init();
    ui_init();
    wifi_fetch_weather();
    ui_full_refresh();
}

void loop()
{
    ui_render_loop();
    wifi_maintenance();
    buttons_poll();
    ui_handle_inactivity();

    delay(5);
}