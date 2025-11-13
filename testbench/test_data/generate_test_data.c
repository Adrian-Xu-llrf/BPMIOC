/**
 * @file generate_test_data.c
 * @brief Generate test data files for BPM IOC testing
 *
 * This utility generates various waveform and parameter files for testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define WAVEFORM_SIZE 10000
#define NUM_CHANNELS 8

/* Generate pure sine wave */
void generate_sine_wave(const char* filename, float amplitude, float frequency, int add_noise) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# Sine wave: amplitude=%.2f, frequency=%.4f, noise=%s\n",
            amplitude, frequency, add_noise ? "yes" : "no");
    fprintf(fp, "# Sample, Channel0, Channel1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7\n");

    srand(time(NULL));

    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        fprintf(fp, "%d", i);
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float phase_offset = ch * M_PI / 4.0;  /* Different phase for each channel */
            float value = amplitude * sin(2.0 * M_PI * frequency * i + phase_offset);

            if (add_noise) {
                float noise = ((float)rand() / RAND_MAX - 0.5) * 0.02 * amplitude;
                value += noise;
            }

            fprintf(fp, ", %.6f", value);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate RF cavity pulse */
void generate_cavity_pulse(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# RF cavity pulse with exponential decay\n");
    fprintf(fp, "# Sample, Channel0, Channel1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7\n");

    float rf_freq = 0.02;
    float decay_time = 2000.0;
    float baseline = 0.05;

    srand(time(NULL) + 1);

    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        fprintf(fp, "%d", i);
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float amplitude = 2.0 + ch * 0.3;
            float value;

            if (i >= 1000 && i < 6000) {  /* Pulse region */
                float envelope = exp(-(i - 1000) / decay_time);
                float rf = amplitude * sin(2.0 * M_PI * rf_freq * i);
                float noise = ((float)rand() / RAND_MAX - 0.5) * 0.01;
                value = envelope * rf + baseline + noise;
            } else {  /* Baseline region */
                float noise = ((float)rand() / RAND_MAX - 0.5) * 0.005;
                value = baseline + noise;
            }

            fprintf(fp, ", %.6f", value);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate step signal for testing background subtraction */
void generate_step_signal(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# Step signal for background subtraction test\n");
    fprintf(fp, "# Background: samples 100-500 = 0.5V\n");
    fprintf(fp, "# Signal: samples 1000-5000 = 2.0V\n");
    fprintf(fp, "# Sample, Channel0, Channel1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7\n");

    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        fprintf(fp, "%d", i);
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float value = 0.0;

            if (i >= 100 && i < 500) {
                value = 0.5;  /* Background region */
            } else if (i >= 1000 && i < 5000) {
                value = 2.0 + ch * 0.1;  /* Signal region */
            }

            fprintf(fp, ", %.6f", value);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate noisy baseline */
void generate_noisy_baseline(const char* filename, float noise_level) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# Noisy baseline: noise_level=%.3f\n", noise_level);
    fprintf(fp, "# Sample, Channel0, Channel1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7\n");

    srand(time(NULL) + 2);

    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        fprintf(fp, "%d", i);
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float baseline = 1.0 + ch * 0.2;
            float noise = ((float)rand() / RAND_MAX - 0.5) * 2.0 * noise_level;
            float value = baseline + noise;

            fprintf(fp, ", %.6f", value);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate BPM position data */
void generate_bpm_positions(const char* filename, int num_samples) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# BPM position data with beam jitter\n");
    fprintf(fp, "# Sample, X1, Y1, X2, Y2\n");

    srand(time(NULL) + 3);

    float x1_center = 0.5, y1_center = 0.3;
    float x2_center = -0.2, y2_center = 0.4;
    float jitter = 0.05;

    for (int i = 0; i < num_samples; i++) {
        float x1 = x1_center + ((float)rand() / RAND_MAX - 0.5) * jitter;
        float y1 = y1_center + ((float)rand() / RAND_MAX - 0.5) * jitter;
        float x2 = x2_center + ((float)rand() / RAND_MAX - 0.5) * jitter;
        float y2 = y2_center + ((float)rand() / RAND_MAX - 0.5) * jitter;

        fprintf(fp, "%d, %.6f, %.6f, %.6f, %.6f\n", i, x1, y1, x2, y2);
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate test parameter CSV file */
void generate_test_parameters(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# BPM Test Parameters\n");
    fprintf(fp, "Parameter,Value,Unit,Description\n");
    fprintf(fp, "k1,1.0,,BPM calibration constant k1\n");
    fprintf(fp, "k2,1.0,,BPM calibration constant k2\n");
    fprintf(fp, "k3,1.0,,BPM calibration constant k3\n");
    fprintf(fp, "kx,1.0,,X position scaling factor\n");
    fprintf(fp, "ky,1.0,,Y position scaling factor\n");
    fprintf(fp, "x_offset,0.0,mm,X position offset\n");
    fprintf(fp, "y_offset,0.0,mm,Y position offset\n");
    fprintf(fp, "phase_offset,0.0,deg,Phase offset\n");
    fprintf(fp, "x_min,-10.0,mm,X minimum limit\n");
    fprintf(fp, "x_max,10.0,mm,X maximum limit\n");
    fprintf(fp, "y_min,-10.0,mm,Y minimum limit\n");
    fprintf(fp, "y_max,10.0,mm,Y maximum limit\n");
    fprintf(fp, "rf_frequency,1.3,GHz,RF frequency\n");
    fprintf(fp, "sample_rate,100,MHz,ADC sample rate\n");

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate timestamp test data */
void generate_timestamp_data(const char* filename, int num_samples) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# White Rabbit Timestamp Data\n");
    fprintf(fp, "# Sample, TAI_Seconds, TAI_Nanoseconds\n");

    uint32_t base_seconds = (uint32_t)time(NULL);
    uint32_t nanoseconds = 0;

    for (int i = 0; i < num_samples; i++) {
        fprintf(fp, "%d, %u, %u\n", i, base_seconds, nanoseconds);

        /* Increment by 1ms */
        nanoseconds += 1000000;
        if (nanoseconds >= 1000000000) {
            nanoseconds -= 1000000000;
            base_seconds++;
        }
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

/* Generate multi-tone signal (for frequency analysis tests) */
void generate_multitone(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    fprintf(fp, "# Multi-tone signal (3 frequencies)\n");
    fprintf(fp, "# Sample, Channel0, Channel1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7\n");

    float freq1 = 0.01;
    float freq2 = 0.02;
    float freq3 = 0.03;

    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        fprintf(fp, "%d", i);
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            float amp1 = 1.0, amp2 = 0.5, amp3 = 0.3;
            float value = amp1 * sin(2.0 * M_PI * freq1 * i) +
                          amp2 * sin(2.0 * M_PI * freq2 * i) +
                          amp3 * sin(2.0 * M_PI * freq3 * i);

            fprintf(fp, ", %.6f", value);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Generated: %s\n", filename);
}

int main(int argc, char* argv[]) {
    printf("BPM IOC Test Data Generator\n");
    printf("====================================\n\n");

    /* Create test data directory if it doesn't exist */
    system("mkdir -p output");

    /* Generate various test waveforms */
    generate_sine_wave("output/sine_clean.csv", 2.0, 0.01, 0);
    generate_sine_wave("output/sine_noisy.csv", 2.0, 0.01, 1);
    generate_cavity_pulse("output/cavity_pulse.csv");
    generate_step_signal("output/step_signal.csv");
    generate_noisy_baseline("output/noisy_baseline_low.csv", 0.01);
    generate_noisy_baseline("output/noisy_baseline_high.csv", 0.1);
    generate_multitone("output/multitone.csv");

    /* Generate BPM position data */
    generate_bpm_positions("output/bpm_positions.csv", 1000);

    /* Generate timestamp data */
    generate_timestamp_data("output/timestamps.csv", 1000);

    /* Generate parameter file */
    generate_test_parameters("output/test_parameters.csv");

    printf("\n====================================\n");
    printf("All test data files generated successfully!\n");
    printf("Files are located in the 'output' directory\n");

    return 0;
}
