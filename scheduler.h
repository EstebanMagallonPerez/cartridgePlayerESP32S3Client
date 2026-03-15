#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define MAX_SCHEDULES 8
// HVAC Modes
enum HVACMode
{
    HVAC_OFF = 0,
    HVAC_HEAT,
    HVAC_COOL,
    HVAC_AUTO
};

// Fan Modes
enum FanMode
{
    FAN_OFF = 0,
    FAN_AUTO,
    FAN_ON
};

// Schedule definition
struct Schedule
{
    char name[20];          // optional name
    uint16_t on_minutes;    // minutes since midnight
    uint16_t off_minutes;   // minutes since midnight
    uint8_t hvac_mode;
    uint8_t fan_mode;
    uint8_t days_mask;      // bitmask (Sun=0 ... Sat=6)
};

// Action returned by scheduler
struct ScheduleAction
{
    bool trigger_on;
    bool trigger_off;
    uint8_t hvac_mode;
    uint8_t fan_mode;
};

void scheduler_init();

bool scheduler_add(const Schedule &s);

bool scheduler_remove(int index);

int scheduler_count();

Schedule *scheduler_get(int index);

void scheduler_clear();

ScheduleAction scheduler_tick(int minutes_now, int today);

void scheduler_generate_display_name(const Schedule &s, char *buf, size_t len);

#endif