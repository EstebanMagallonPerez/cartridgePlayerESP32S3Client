#include "scheduler.h"

static Schedule schedules[MAX_SCHEDULES];
static int schedule_count_internal = 0;

void scheduler_init()
{
    schedule_count_internal = 0;
}

bool scheduler_add(const Schedule &s)
{
    if (schedule_count_internal >= MAX_SCHEDULES)
        return false;

    schedules[schedule_count_internal++] = s;
    return true;
}

bool scheduler_remove(int index)
{
    if (index < 0 || index >= schedule_count_internal)
        return false;

    for (int i = index; i < schedule_count_internal - 1; i++)
    {
        schedules[i] = schedules[i + 1];
    }

    schedule_count_internal--;
    return true;
}

int scheduler_count()
{
    return schedule_count_internal;
}

Schedule *scheduler_get(int index)
{
    if (index < 0 || index >= schedule_count_internal)
        return nullptr;

    return &schedules[index];
}

void scheduler_clear()
{
    schedule_count_internal = 0;
}

ScheduleAction scheduler_tick(int minutes_now, int today)
{
    ScheduleAction result;
    result.trigger_on = false;
    result.trigger_off = false;
    result.hvac_mode = HVAC_OFF;
    result.fan_mode = FAN_OFF;

    for (int i = 0; i < schedule_count_internal; i++)
    {
        Schedule &s = schedules[i];

        // check if schedule applies today
        if (!(s.days_mask & (1 << today)))
            continue;

        // ON trigger
        if (minutes_now == s.on_minutes)
        {
            result.trigger_on = true;
            result.hvac_mode = s.hvac_mode;
            result.fan_mode = s.fan_mode;
        }

        // OFF trigger
        if (minutes_now == s.off_minutes)
        {
            result.trigger_off = true;
        }
    }

    return result;
}

void scheduler_generate_display_name(const Schedule &s, char *buf, size_t len)
{
    if (strlen(s.name) > 0)
    {
        strncpy(buf, s.name, len);
        buf[len - 1] = '\0';
        return;
    }

    int on_h = s.on_minutes / 60;
    int on_m = s.on_minutes % 60;

    int off_h = s.off_minutes / 60;
    int off_m = s.off_minutes % 60;

    snprintf(buf, len,
             "%02d:%02d-%02d:%02d",
             on_h, on_m,
             off_h, off_m);
}