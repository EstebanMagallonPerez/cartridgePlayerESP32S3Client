#include "wifiManager.h"
#include "ui.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* ssid = "Blueground615";
static const char* password = "showupstartliving";
static const char* serverURL = "https://api.open-meteo.com/v1/forecast?latitude=47.615&longitude=-122.3228&current=temperature_2m,weather_code,is_day&temperature_unit=fahrenheit";
static const char* timeURL = "https://time.now/developer/api/timezone/America/Los_Angeles";

static unsigned long last_weather_update = 0;
static const unsigned long WEATHER_INTERVAL = 3600000; // 1 hour

// Time bookkeeping: store epoch ms from server and reference millis()
static long long start_epoch_ms = 0;
static unsigned long start_millis = 0;
static long long last_minute_update_epoch_ms = 0;

static void wifi_fetch_time()
{
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin(timeURL);

    if (http.GET() == HTTP_CODE_OK)
    {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, http.getString());

        long long utc_epoch_ms =
            (long long)doc["unixtime"].as<long long>() * 1000LL;

        int raw_offset = doc["raw_offset"] | 0;
        int dst_offset = doc["dst_offset"] | 0;

        int tz_offset_seconds = raw_offset + dst_offset;

        long long local_epoch_ms =
            utc_epoch_ms + (long long)tz_offset_seconds * 1000LL;

        start_epoch_ms = local_epoch_ms;
        start_millis = millis();

        ui_set_time_from_epoch(local_epoch_ms);
    }

    http.end();
}

static char weather_icon_from_code(int code, bool is_daytime)
{
    switch(code) {
        case 0: return is_daytime ? ')' : '(';
        case 1:
        case 2: return is_daytime ? '"' : '%';
        case 3:
        case 45:
        case 48: return '!';
        case 51:
        case 53:
        case 55:
        case 61:
        case 63:
        case 65:
        case 66:
        case 67: return '$';
        case 71:
        case 73:
        case 75:
        case 77: return '#';
        case 95:
        case 96:
        case 99: return '\'';
        default: return '!';
    }
}

void connectWiFi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.println("WiFi connected");
}

void wifi_fetch_weather()
{
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.setTimeout(10000);
    http.begin(serverURL);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            float weather_temp = doc["current"]["temperature_2m"].as<float>();
            int weather_code = doc["current"]["weather_code"].as<int>();
            bool is_daytime = doc["current"]["is_day"].as<bool>();
            int actual_temp = round(weather_temp);
            char icon = weather_icon_from_code(weather_code, is_daytime);
            ui_set_outside(actual_temp, icon);
        } else {
            ui_set_outside(-999, '?');
        }
    } else {
        ui_set_outside(-999, '?');
    }
    http.end();
    last_weather_update = millis();
    // Refresh time when we refresh weather
    wifi_fetch_time();
}

void wifi_maintenance()
{
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    if (millis() - last_weather_update > WEATHER_INTERVAL) {
        wifi_fetch_weather();
    }

    // Update displayed clock every minute using internal millis reference
    if (start_epoch_ms != 0) {
        unsigned long now_ms = millis();
        long long current_epoch_ms = start_epoch_ms + (long long)(now_ms - start_millis);
        long long current_minute_epoch = current_epoch_ms - (current_epoch_ms % 60000LL);
        if (current_minute_epoch != last_minute_update_epoch_ms) {
            last_minute_update_epoch_ms = current_minute_epoch;
            ui_set_time_from_epoch(current_epoch_ms);
        }
    }
}
