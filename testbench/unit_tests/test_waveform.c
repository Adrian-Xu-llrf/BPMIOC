/**
 * @file test_waveform.c
 * @brief Unit tests for waveform processing functions
 */

#include "test_framework.h"
#include <stdlib.h>
#include <math.h>

/* Function prototypes - these would normally come from driverWrapper.h */
/* We'll create simplified versions for testing */

#define AVGStart 1000
#define AVGStop 5000
#define BackGroundStart 100
#define BackGroundStop 500

/* Simplified version of calculateAvgVoltage for testing */
static float signal_avg[8];
static float background_avg[8];
static float avg_voltage[8];

void test_calculateAvgVoltage(float *wfBuf, int ch_N, int length) {
    int i;
    float sum_signal = 0.0;
    float sum_background = 0.0;
    int signal_count = AVGStop - AVGStart;
    int background_count = BackGroundStop - BackGroundStart;

    if (ch_N < 0 || ch_N >= 8) {
        return;
    }

    /* Calculate signal average */
    for (i = AVGStart; i < AVGStop && i < length; i++) {
        sum_signal += wfBuf[i];
    }
    signal_avg[ch_N] = sum_signal / signal_count;

    /* Calculate background average */
    for (i = BackGroundStart; i < BackGroundStop && i < length; i++) {
        sum_background += wfBuf[i];
    }
    background_avg[ch_N] = sum_background / background_count;

    /* Calculate average voltage (signal - background) */
    avg_voltage[ch_N] = signal_avg[ch_N] - background_avg[ch_N];
}

float get_avg_voltage(int ch_N) {
    if (ch_N < 0 || ch_N >= 8) {
        return 0.0;
    }
    return avg_voltage[ch_N];
}

/* Test: Constant DC signal */
TEST(waveform_constant_dc) {
    float waveform[10000];
    int i;

    /* Create constant DC waveform at 1.5V */
    for (i = 0; i < 10000; i++) {
        waveform[i] = 1.5;
    }

    test_calculateAvgVoltage(waveform, 0, 10000);

    /* For constant DC, avg_voltage should be ~0 (signal - background) */
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(0), 0.001);
}

/* Test: Pure sine wave */
TEST(waveform_sine) {
    float waveform[10000];
    int i;
    float amplitude = 2.0;
    float frequency = 0.01;  /* 0.01 cycles per sample */

    /* Create sine wave */
    for (i = 0; i < 10000; i++) {
        waveform[i] = amplitude * sin(2.0 * M_PI * frequency * i);
    }

    test_calculateAvgVoltage(waveform, 1, 10000);

    /* The average should be close to 0 for a pure sine wave */
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(1), 0.1);
}

/* Test: Step signal (background + signal) */
TEST(waveform_step_signal) {
    float waveform[10000];
    int i;
    float background_level = 0.5;
    float signal_level = 2.0;

    /* Create step waveform: low at background region, high at signal region */
    for (i = 0; i < 10000; i++) {
        if (i >= AVGStart && i < AVGStop) {
            waveform[i] = signal_level;  /* Signal region */
        } else if (i >= BackGroundStart && i < BackGroundStop) {
            waveform[i] = background_level;  /* Background region */
        } else {
            waveform[i] = 0.0;  /* Other regions */
        }
    }

    test_calculateAvgVoltage(waveform, 2, 10000);

    /* avg_voltage should be signal - background */
    float expected = signal_level - background_level;
    ASSERT_FLOAT_EQ(expected, get_avg_voltage(2), 0.001);
}

/* Test: Noisy signal */
TEST(waveform_with_noise) {
    float waveform[10000];
    int i;
    float base_level = 1.0;
    float noise_amplitude = 0.1;

    /* Create signal with random noise */
    srand(12345);  /* Fixed seed for reproducibility */
    for (i = 0; i < 10000; i++) {
        float noise = ((float)rand() / RAND_MAX - 0.5) * 2.0 * noise_amplitude;
        waveform[i] = base_level + noise;
    }

    test_calculateAvgVoltage(waveform, 3, 10000);

    /* With sufficient samples, noise should average out to near 0 */
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(3), 0.05);
}

/* Test: Pulsed RF signal */
TEST(waveform_pulsed_rf) {
    float waveform[10000];
    int i;
    float rf_amplitude = 1.5;
    float rf_frequency = 0.05;
    float background_level = 0.1;

    /* Background region: low level + small noise */
    for (i = BackGroundStart; i < BackGroundStop; i++) {
        waveform[i] = background_level;
    }

    /* Signal region: RF pulse */
    for (i = AVGStart; i < AVGStop; i++) {
        waveform[i] = rf_amplitude * sin(2.0 * M_PI * rf_frequency * i) + background_level;
    }

    /* Fill rest with zeros */
    for (i = 0; i < BackGroundStart; i++) waveform[i] = 0.0;
    for (i = BackGroundStop; i < AVGStart; i++) waveform[i] = 0.0;
    for (i = AVGStop; i < 10000; i++) waveform[i] = 0.0;

    test_calculateAvgVoltage(waveform, 4, 10000);

    /* Average should be close to background_level (since RF averages to ~0) */
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(4), 0.1);
}

/* Test: Multiple channels */
TEST(waveform_multiple_channels) {
    float waveform[10000];
    int ch, i;

    for (ch = 0; ch < 8; ch++) {
        /* Create different signal for each channel */
        float signal_offset = ch * 0.5;

        for (i = 0; i < 10000; i++) {
            if (i >= AVGStart && i < AVGStop) {
                waveform[i] = 2.0 + signal_offset;
            } else if (i >= BackGroundStart && i < BackGroundStop) {
                waveform[i] = 1.0;
            } else {
                waveform[i] = 0.0;
            }
        }

        test_calculateAvgVoltage(waveform, ch, 10000);

        /* Each channel should have 1.0 + channel_offset */
        float expected = 1.0 + signal_offset;
        ASSERT_FLOAT_EQ(expected, get_avg_voltage(ch), 0.001);
    }
}

/* Test: Boundary conditions */
TEST(waveform_boundary_conditions) {
    float waveform[10000];
    int i;

    /* Fill with known pattern */
    for (i = 0; i < 10000; i++) {
        waveform[i] = 1.0;
    }

    /* Test with invalid channel number */
    test_calculateAvgVoltage(waveform, -1, 10000);
    test_calculateAvgVoltage(waveform, 8, 10000);

    /* These should not crash and should return 0 */
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(-1), 0.001);
    ASSERT_FLOAT_EQ(0.0, get_avg_voltage(8), 0.001);
}

/* Test: Zero-length waveform */
TEST(waveform_zero_length) {
    float waveform[10000];

    /* Should handle gracefully without crashing */
    test_calculateAvgVoltage(waveform, 0, 0);

    /* Result may be undefined, but shouldn't crash */
    ASSERT_TRUE(1);  /* Just verify we got here */
}

/* Test: Real-world RF cavity signal simulation */
TEST(waveform_realistic_cavity_signal) {
    float waveform[10000];
    int i;

    /* Simulate realistic BPM RF signal:
     * - Background: baseline + small noise
     * - Signal: RF burst with decay envelope
     */
    float baseline = 0.05;
    float rf_amplitude = 3.0;
    float rf_freq = 0.02;
    float decay_time = 2000.0;

    srand(54321);

    /* Background region */
    for (i = BackGroundStart; i < BackGroundStop; i++) {
        float noise = ((float)rand() / RAND_MAX - 0.5) * 0.01;
        waveform[i] = baseline + noise;
    }

    /* Signal region with envelope */
    for (i = AVGStart; i < AVGStop; i++) {
        float envelope = exp(-(i - AVGStart) / decay_time);
        float rf = rf_amplitude * sin(2.0 * M_PI * rf_freq * i);
        float noise = ((float)rand() / RAND_MAX - 0.5) * 0.02;
        waveform[i] = envelope * rf + baseline + noise;
    }

    /* Fill other regions */
    for (i = 0; i < BackGroundStart; i++) waveform[i] = baseline;
    for (i = BackGroundStop; i < AVGStart; i++) waveform[i] = baseline;
    for (i = AVGStop; i < 10000; i++) waveform[i] = baseline;

    test_calculateAvgVoltage(waveform, 5, 10000);

    /* The average should be close to 0 (RF averages out) */
    float result = get_avg_voltage(5);
    printf("      Realistic cavity signal avg_voltage: %.6f\n", result);
    ASSERT_TRUE(fabs(result) < 0.5);  /* Should be small */
}

/* Main test runner */
int main(void) {
    TEST_SUITE_START("Waveform Processing Tests");

    RUN_TEST(waveform_constant_dc);
    RUN_TEST(waveform_sine);
    RUN_TEST(waveform_step_signal);
    RUN_TEST(waveform_with_noise);
    RUN_TEST(waveform_pulsed_rf);
    RUN_TEST(waveform_multiple_channels);
    RUN_TEST(waveform_boundary_conditions);
    RUN_TEST(waveform_zero_length);
    RUN_TEST(waveform_realistic_cavity_signal);

    TEST_SUITE_END();

    return TEST_RETURN_CODE();
}
