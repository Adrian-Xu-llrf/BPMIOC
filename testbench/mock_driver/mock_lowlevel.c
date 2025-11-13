/**
 * @file mock_lowlevel.c
 * @brief Mock implementation of low-level hardware driver
 *
 * This provides a simulated hardware driver for testing the BPM monitor
 * application without requiring real hardware.
 */

#include "mock_lowlevel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Mock hardware state */
static struct {
    /* RF signals (channels 3-10) */
    float rf_amplitude[8];    /* RF3-RF10 amplitude */
    float rf_phase[8];        /* RF3-RF10 phase */

    /* BPM positions */
    float x1, y1, x2, y2;

    /* BPM parameters */
    float k1, k2, k3;
    float phase_offset;
    float kx, ky;
    float x_offset, y_offset;
    float x_min, x_max, y_min, y_max;

    /* Control flags */
    int history_trigger_enabled;
    int trigger_ratio;
    int history_data_ready;
    int wr_capture_enabled;

    /* Digital I/O */
    int digital_inputs[32];
    int digital_outputs[32];
    int led0_state, led1_state;
    int arm_led_enabled;
    int fan_led_status;
    int adc_clk_state;

    /* White Rabbit timestamp */
    uint32_t tai_seconds;
    uint32_t tai_nanoseconds;

    /* Test mode */
    int test_mode;
    int error_injection_enabled;

} mock_hw_state;

/* Statistics */
static MockStats stats = {0, 0, 0, 0};

/* Data buffers for waveform simulation */
#define MOCK_BUF_SIZE 10000
static float mock_waveform_buffer[8][MOCK_BUF_SIZE];  /* 8 channels */
static float mock_history_buffer[8][100000];           /* History buffer */
static int waveform_initialized = 0;

/* Initialize mock hardware state */
static void init_mock_state(void) {
    int i;

    /* Initialize RF signals with realistic values */
    for (i = 0; i < 8; i++) {
        mock_hw_state.rf_amplitude[i] = 1.0 + i * 0.1;  /* 1.0 to 1.7 */
        mock_hw_state.rf_phase[i] = i * 10.0;            /* 0 to 70 degrees */
    }

    /* Initialize BPM positions */
    mock_hw_state.x1 = 0.5;
    mock_hw_state.y1 = 0.3;
    mock_hw_state.x2 = -0.2;
    mock_hw_state.y2 = 0.4;

    /* Initialize parameters */
    mock_hw_state.k1 = 1.0;
    mock_hw_state.k2 = 1.0;
    mock_hw_state.k3 = 1.0;
    mock_hw_state.phase_offset = 0.0;
    mock_hw_state.kx = 1.0;
    mock_hw_state.ky = 1.0;
    mock_hw_state.x_offset = 0.0;
    mock_hw_state.y_offset = 0.0;

    /* Initialize digital I/O */
    for (i = 0; i < 32; i++) {
        mock_hw_state.digital_inputs[i] = 0;
        mock_hw_state.digital_outputs[i] = 0;
    }

    mock_hw_state.adc_clk_state = 1;  /* ADC clock OK */
    mock_hw_state.led0_state = 1;
    mock_hw_state.led1_state = 1;

    /* Initialize timestamp */
    mock_hw_state.tai_seconds = (uint32_t)time(NULL);
    mock_hw_state.tai_nanoseconds = 0;

    waveform_initialized = 1;
}

/* Generate simulated waveform data */
static void generate_waveform(int channel) {
    int i;
    float amplitude = mock_hw_state.rf_amplitude[channel];
    float phase = mock_hw_state.rf_phase[channel] * M_PI / 180.0;
    float frequency = 0.001 * (channel + 3);  /* Different freq for each channel */

    for (i = 0; i < MOCK_BUF_SIZE; i++) {
        /* Simulate RF signal: amplitude * sin(2*pi*f*t + phase) + noise */
        float signal = amplitude * sin(2.0 * M_PI * frequency * i + phase);
        float noise = ((float)rand() / RAND_MAX - 0.5) * 0.01;  /* 1% noise */
        mock_waveform_buffer[channel][i] = signal + noise;
    }
}

/* Update timestamp (simulates time progression) */
static void update_timestamp(void) {
    mock_hw_state.tai_nanoseconds += 1000000;  /* Add 1ms */
    if (mock_hw_state.tai_nanoseconds >= 1000000000) {
        mock_hw_state.tai_nanoseconds -= 1000000000;
        mock_hw_state.tai_seconds++;
    }
}

/* Public API implementation */

void mock_set_test_mode(int mode) {
    mock_hw_state.test_mode = mode;
    printf("[MOCK] Test mode set to: %d\n", mode);
}

void mock_reset_state(void) {
    memset(&mock_hw_state, 0, sizeof(mock_hw_state));
    memset(&stats, 0, sizeof(stats));
    init_mock_state();
    printf("[MOCK] Hardware state reset\n");
}

void mock_set_rf_amplitude(int channel, float value) {
    if (channel >= 3 && channel <= 10) {
        int idx = channel - 3;
        mock_hw_state.rf_amplitude[idx] = value;
        generate_waveform(idx);
        printf("[MOCK] RF%d amplitude set to: %.3f\n", channel, value);
    }
}

void mock_set_rf_phase(int channel, float value) {
    if (channel >= 3 && channel <= 10) {
        int idx = channel - 3;
        mock_hw_state.rf_phase[idx] = value;
        generate_waveform(idx);
        printf("[MOCK] RF%d phase set to: %.3f degrees\n", channel, value);
    }
}

void mock_set_bpm_position(float x1, float y1, float x2, float y2) {
    mock_hw_state.x1 = x1;
    mock_hw_state.y1 = y1;
    mock_hw_state.x2 = x2;
    mock_hw_state.y2 = y2;
    printf("[MOCK] BPM positions set: X1=%.3f, Y1=%.3f, X2=%.3f, Y2=%.3f\n",
           x1, y1, x2, y2);
}

void mock_enable_error_injection(int enable) {
    mock_hw_state.error_injection_enabled = enable;
    printf("[MOCK] Error injection %s\n", enable ? "enabled" : "disabled");
}

void mock_set_timestamp(uint32_t sec, uint32_t nsec) {
    mock_hw_state.tai_seconds = sec;
    mock_hw_state.tai_nanoseconds = nsec;
    printf("[MOCK] Timestamp set to: %u.%09u\n", sec, nsec);
}

/* Hardware API - RF and BPM data */

static struct {
    float rf_amp[8];
    float rf_phase[8];
} rf_info_buffer;

void* GetRfInfo(void) {
    int i;

    if (!waveform_initialized) {
        init_mock_state();
    }

    /* Add some variation to simulate real hardware */
    for (i = 0; i < 8; i++) {
        float variation = ((float)rand() / RAND_MAX - 0.5) * 0.02;  /* 2% variation */
        rf_info_buffer.rf_amp[i] = mock_hw_state.rf_amplitude[i] * (1.0 + variation);
        rf_info_buffer.rf_phase[i] = mock_hw_state.rf_phase[i];
    }

    stats.read_count++;
    update_timestamp();

    return &rf_info_buffer;
}

static struct {
    float bpm_phase[2];
} bpm_phase_buffer;

void* GetBPMPhaseValue(void) {
    if (!waveform_initialized) {
        init_mock_state();
    }

    bpm_phase_buffer.bpm_phase[0] = mock_hw_state.rf_phase[0] + 5.0;
    bpm_phase_buffer.bpm_phase[1] = mock_hw_state.rf_phase[1] + 5.0;

    stats.read_count++;
    return &bpm_phase_buffer;
}

static struct {
    float x1, y1, x2, y2;
} position_buffer;

void* GetxyPosition(void) {
    if (!waveform_initialized) {
        init_mock_state();
    }

    /* Add small variations to simulate beam jitter */
    position_buffer.x1 = mock_hw_state.x1 + ((float)rand() / RAND_MAX - 0.5) * 0.01;
    position_buffer.y1 = mock_hw_state.y1 + ((float)rand() / RAND_MAX - 0.5) * 0.01;
    position_buffer.x2 = mock_hw_state.x2 + ((float)rand() / RAND_MAX - 0.5) * 0.01;
    position_buffer.y2 = mock_hw_state.y2 + ((float)rand() / RAND_MAX - 0.5) * 0.01;

    stats.read_count++;
    return &position_buffer;
}

/* Hardware API - Parameter setters */

int SetBPMk1(float value) {
    if (mock_hw_state.error_injection_enabled && value < 0) {
        stats.error_count++;
        return -1;
    }
    mock_hw_state.k1 = value;
    stats.write_count++;
    return 0;
}

int SetBPMk2(float value) {
    if (mock_hw_state.error_injection_enabled && value < 0) {
        stats.error_count++;
        return -1;
    }
    mock_hw_state.k2 = value;
    stats.write_count++;
    return 0;
}

int SetBPMk3(float value) {
    if (mock_hw_state.error_injection_enabled && value < 0) {
        stats.error_count++;
        return -1;
    }
    mock_hw_state.k3 = value;
    stats.write_count++;
    return 0;
}

int SetBPMPhaseOffset(float value) {
    mock_hw_state.phase_offset = value;
    stats.write_count++;
    return 0;
}

int SetBPMkxy(float kx, float ky) {
    mock_hw_state.kx = kx;
    mock_hw_state.ky = ky;
    stats.write_count++;
    return 0;
}

int SetBPMxyOffset(float x_offset, float y_offset) {
    mock_hw_state.x_offset = x_offset;
    mock_hw_state.y_offset = y_offset;
    stats.write_count++;
    return 0;
}

int SetBPMxyLimits(float x_min, float x_max, float y_min, float y_max) {
    mock_hw_state.x_min = x_min;
    mock_hw_state.x_max = x_max;
    mock_hw_state.y_min = y_min;
    mock_hw_state.y_max = y_max;
    stats.write_count++;
    return 0;
}

/* Hardware API - Trigger and history */

int SetHistoryTrigger(int enable) {
    mock_hw_state.history_trigger_enabled = enable;
    if (enable) {
        mock_hw_state.history_data_ready = 1;
        stats.trigger_count++;
    }
    stats.write_count++;
    return 0;
}

int SetTriggerExtractDataRatio(int ratio) {
    mock_hw_state.trigger_ratio = ratio;
    stats.write_count++;
    return 0;
}

int GetHistoryDataReady(void) {
    stats.read_count++;
    return mock_hw_state.history_data_ready;
}

void* GetTriggerAllData(void) {
    static struct {
        float data[8][MOCK_BUF_SIZE];
    } trigger_data;

    int i, ch;

    if (!waveform_initialized) {
        init_mock_state();
        for (ch = 0; ch < 8; ch++) {
            generate_waveform(ch);
        }
    }

    /* Copy waveform data to trigger buffer */
    for (ch = 0; ch < 8; ch++) {
        for (i = 0; i < MOCK_BUF_SIZE; i++) {
            trigger_data.data[ch][i] = mock_waveform_buffer[ch][i];
        }
    }

    stats.read_count++;
    return &trigger_data;
}

void* GetHistoryChannelData(int channel) {
    if (channel < 0 || channel >= 8) {
        return NULL;
    }

    stats.read_count++;
    return mock_history_buffer[channel];
}

/* Hardware API - Digital I/O */

int GetDI(int channel) {
    if (channel < 0 || channel >= 32) {
        return -1;
    }
    stats.read_count++;
    return mock_hw_state.digital_inputs[channel];
}

int SetDO(int channel, int value) {
    if (channel < 0 || channel >= 32) {
        return -1;
    }
    mock_hw_state.digital_outputs[channel] = value;
    stats.write_count++;
    return 0;
}

int GetFPGA_LED0_RBK(void) {
    stats.read_count++;
    return mock_hw_state.led0_state;
}

int GetFPGA_LED1_RBK(void) {
    stats.read_count++;
    return mock_hw_state.led1_state;
}

int SetArmLedEnable(int enable) {
    mock_hw_state.arm_led_enabled = enable;
    stats.write_count++;
    return 0;
}

int SetFanLedStatus(int status) {
    mock_hw_state.fan_led_status = status;
    stats.write_count++;
    return 0;
}

int GetADclkState(void) {
    stats.read_count++;
    return mock_hw_state.adc_clk_state;
}

/* Hardware API - White Rabbit timestamp */

int GetWRStatus(void) {
    stats.read_count++;
    return 1;  /* Always return OK in mock mode */
}

int SetWRCaputureDataTrigger(int enable) {
    mock_hw_state.wr_capture_enabled = enable;
    stats.write_count++;
    return 0;
}

static struct {
    uint32_t tai_sec;
    uint32_t tai_nsec;
} timestamp_buffer;

void* GetTimestampData(void) {
    update_timestamp();

    timestamp_buffer.tai_sec = mock_hw_state.tai_seconds;
    timestamp_buffer.tai_nsec = mock_hw_state.tai_nanoseconds;

    stats.read_count++;
    return &timestamp_buffer;
}

/* Statistics and verification */

MockStats* mock_get_statistics(void) {
    return &stats;
}

int mock_verify_call_sequence(const char* expected_sequence) {
    /* Simple verification - can be extended */
    printf("[MOCK] Call sequence verification: %s\n", expected_sequence);
    return 0;
}
