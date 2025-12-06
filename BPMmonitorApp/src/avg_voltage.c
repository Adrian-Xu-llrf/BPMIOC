#include "avg_voltage.h"

#include <stddef.h>

static int avg_start = 0;
static int avg_stop = 0;
static int background_start = 0;
static int background_stop = 0;
static int avg_enabled = 0;
static float avg_voltages[8] = {0};

static int clamp_to_range(int value, int min, int max)
{
        if (value < min) {
                return min;
        }
        if (value > max) {
                return max;
        }
        return value;
}

static int channel_index(int ch_id)
{
        if (ch_id < 0) {
                return -1;
        }
        return ch_id / 2;
}

void configure_avg_window(int start, int stop, int bg_start, int bg_stop)
{
        avg_start = start;
        avg_stop = stop;
        background_start = bg_start;
        background_stop = bg_stop;
}

void get_avg_window(int *start, int *stop, int *bg_start, int *bg_stop)
{
        if (start) {
                *start = avg_start;
        }
        if (stop) {
                *stop = avg_stop;
        }
        if (bg_start) {
                *bg_start = background_start;
        }
        if (bg_stop) {
                *bg_stop = background_stop;
        }
}

void set_avg_enabled(int enabled)
{
        avg_enabled = enabled ? 1 : 0;
}

int is_avg_enabled(void)
{
        return avg_enabled;
}

float update_avg_voltage(int ch_id, float *data, int len)
{
        int i;
        float signal_sum = 0;
        float background_sum = 0;
        int signal_count;
        int background_count;
        int idx;
        int start;
        int stop;
        int bg_start;
        int bg_stop;

        if (!data || len <= 0) {
                return 0;
        }

        if (!avg_enabled) {
                return get_avg_voltage(ch_id);
        }

        start = avg_start;
        stop = avg_stop;
        bg_start = background_start;
        bg_stop = background_stop;

        start = clamp_to_range(start, 0, len - 1);
        stop = clamp_to_range(stop, start, len - 1);
        bg_start = clamp_to_range(bg_start, 0, len - 1);
        bg_stop = clamp_to_range(bg_stop, bg_start, len - 1);

        for (i = 0; i < len; ++i) {
                if (i >= start && i <= stop) {
                        signal_sum += data[i];
                }
                if (i >= bg_start && i <= bg_stop) {
                        background_sum += data[i];
                }
        }

        signal_count = stop - start + 1;
        background_count = bg_stop - bg_start + 1;

        idx = channel_index(ch_id);
        if (idx < 0 || idx >= (int)(sizeof(avg_voltages) / sizeof(avg_voltages[0]))) {
                return 0;
        }

        if (signal_count > 0 && background_count > 0) {
            avg_voltages[idx] = signal_sum / signal_count - background_sum / background_count;
        } else {
            avg_voltages[idx] = 0;
        }

        return avg_voltages[idx];
}

float get_avg_voltage(int ch_id)
{
        int idx = channel_index(ch_id);
        if (idx < 0 || idx >= (int)(sizeof(avg_voltages) / sizeof(avg_voltages[0]))) {
                return 0;
        }
        return avg_voltages[idx];
}
