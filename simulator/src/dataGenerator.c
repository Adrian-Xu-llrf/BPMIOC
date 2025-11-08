/**
 * dataGenerator.c - Configurable Data Generator Implementation
 *
 * 提供灵活的RF数据生成功能
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "dataGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*******************************************************************************
 * 全局状态
 ******************************************************************************/

static SimulatorConfig g_config;
static int g_initialized = 0;

/*******************************************************************************
 * 内部辅助函数
 ******************************************************************************/

/**
 * Box-Muller变换生成高斯噪声
 */
double DataGen_GaussianNoise(double mean, double stddev)
{
    static int has_spare = 0;
    static double spare;

    if (has_spare) {
        has_spare = 0;
        return mean + stddev * spare;
    }

    has_spare = 1;

    double u, v, s;
    do {
        u = (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0;
        v = (rand() / (RAND_MAX + 1.0)) * 2.0 - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);

    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    return mean + stddev * u * s;
}

/**
 * 生成均匀分布噪声
 */
double DataGen_UniformNoise(double min, double max)
{
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

/**
 * 设置默认配置
 */
static void set_default_config(void)
{
    // 采样参数
    g_config.sample_rate = 100e6;      // 100 MHz
    g_config.waveform_points = 10000;

    // 全局效果
    g_config.global_drift = 0.1;       // 0.1%/min
    g_config.temperature_coeff = 0.05; // 0.05%/C

    // 触发
    g_config.trigger_mode = 0;
    g_config.trigger_level = 0.0;
    g_config.trigger_slope = 0;

    // 时间
    g_config.start_time = 0.0;
    g_config.current_time = 0.0;

    // 通道配置
    for (int ch = 0; ch < 8; ch++) {
        ChannelConfig *cfg = &g_config.channels[ch];

        // 基本参数
        cfg->frequency = 0.5 + ch * 0.05;  // 0.5-0.85 Hz
        cfg->amplitude = 1.0;
        cfg->offset = 4.0;
        cfg->phase_offset = ch * M_PI / 4.0;

        // 噪声
        cfg->noise_level = 0.02;   // 2%
        cfg->noise_type = 0;       // 高斯

        // 谐波
        cfg->enable_harmonics = 1;
        cfg->harmonic2_amp = 0.1;  // 10%
        cfg->harmonic3_amp = 0.05; // 5%

        // 调制
        cfg->enable_modulation = 0;
        cfg->mod_frequency = 1.0;
        cfg->mod_depth = 0.5;

        // 相位
        cfg->phase_drift_rate = 0.1;  // 0.1 deg/s
        cfg->phase_jitter = 0.5;      // 0.5 deg RMS
    }
}

/*******************************************************************************
 * 公共API实现
 ******************************************************************************/

int DataGen_Init(void)
{
    if (g_initialized) {
        return 0;
    }

    // 初始化随机数
    srand(time(NULL));

    // 设置默认配置
    set_default_config();

    g_initialized = 1;
    return 0;
}

SimulatorConfig* DataGen_GetConfig(void)
{
    return &g_config;
}

void DataGen_ResetTime(void)
{
    g_config.current_time = g_config.start_time;
}

void DataGen_SetTime(double time)
{
    g_config.current_time = time;
}

void DataGen_AdvanceTime(double delta_time)
{
    g_config.current_time += delta_time;
}

double DataGen_GetTime(void)
{
    return g_config.current_time;
}

/*******************************************************************************
 * 数据生成核心函数
 ******************************************************************************/

int DataGen_GenerateSample(int channel, double time, float *value)
{
    if (!g_initialized) {
        return -1;
    }

    if (channel < 0 || channel >= 8 || value == NULL) {
        return -1;
    }

    ChannelConfig *cfg = &g_config.channels[channel];

    // 1. 基础正弦波
    double base = cfg->offset +
                  cfg->amplitude * sin(2.0 * M_PI * cfg->frequency * time +
                                      cfg->phase_offset);

    // 2. 添加谐波
    if (cfg->enable_harmonics) {
        // 2次谐波
        base += cfg->amplitude * cfg->harmonic2_amp *
                sin(2.0 * 2.0 * M_PI * cfg->frequency * time +
                    cfg->phase_offset * 2.0);

        // 3次谐波
        base += cfg->amplitude * cfg->harmonic3_amp *
                sin(3.0 * 2.0 * M_PI * cfg->frequency * time +
                    cfg->phase_offset * 3.0);
    }

    // 3. 调制
    if (cfg->enable_modulation) {
        double mod = 1.0 + cfg->mod_depth *
                     sin(2.0 * M_PI * cfg->mod_frequency * time);
        base *= mod;
    }

    // 4. 添加噪声
    if (cfg->noise_level > 0.0) {
        double noise;
        if (cfg->noise_type == 0) {
            // 高斯噪声
            noise = DataGen_GaussianNoise(0.0, cfg->amplitude * cfg->noise_level);
        } else {
            // 均匀噪声
            double range = cfg->amplitude * cfg->noise_level;
            noise = DataGen_UniformNoise(-range, range);
        }
        base += noise;
    }

    // 5. 全局漂移（温度效应等）
    double drift_factor = 1.0 + (g_config.global_drift / 100.0) * (time / 60.0);
    base *= drift_factor;

    *value = (float)base;
    return 0;
}

int DataGen_GenerateRfSnapshot(float *Amp, float *Phase, float *Power)
{
    if (!g_initialized) {
        return -1;
    }

    if (Amp == NULL || Phase == NULL || Power == NULL) {
        return -1;
    }

    double t = g_config.current_time;

    for (int ch = 0; ch < 8; ch++) {
        ChannelConfig *cfg = &g_config.channels[ch];

        // 幅度
        DataGen_GenerateSample(ch, t, &Amp[ch]);

        // 相位：基础相位 + 漂移 + 抖动
        double base_phase = 90.0 * sin(2.0 * M_PI * 0.1 * t + ch * M_PI / 8.0);
        double drift = cfg->phase_drift_rate * t;
        double jitter = DataGen_GaussianNoise(0.0, cfg->phase_jitter);

        Phase[ch] = (float)(base_phase + drift + jitter);

        // 限制到-180~+180
        while (Phase[ch] > 180.0) Phase[ch] -= 360.0;
        while (Phase[ch] < -180.0) Phase[ch] += 360.0;

        // 功率 = A^2 * 50Ω
        Power[ch] = Amp[ch] * Amp[ch] * 50.0;
    }

    return 0;
}

int DataGen_GenerateWaveform(int channel, float *buffer, int *npts, int max_pts)
{
    if (!g_initialized) {
        return -1;
    }

    if (channel < 0 || channel >= 8 || buffer == NULL || npts == NULL) {
        return -1;
    }

    int num_points = g_config.waveform_points;
    if (num_points > max_pts) {
        num_points = max_pts;
    }

    double fs = g_config.sample_rate;  // 采样率
    double fc = 500e6;                 // 载波频率 500 MHz

    ChannelConfig *cfg = &g_config.channels[channel];

    for (int i = 0; i < num_points; i++) {
        double t = i / fs;

        // 载波
        double carrier = cfg->amplitude * sin(2.0 * M_PI * fc * t + cfg->phase_offset);

        // 噪声
        double noise = 0.0;
        if (cfg->noise_level > 0.0) {
            noise = DataGen_GaussianNoise(0.0, cfg->amplitude * cfg->noise_level * 0.1);
        }

        // 谐波
        double harmonics = 0.0;
        if (cfg->enable_harmonics) {
            harmonics = cfg->harmonic2_amp * cfg->amplitude *
                        sin(2.0 * 2.0 * M_PI * fc * t);
        }

        buffer[i] = (float)(carrier + noise + harmonics);
    }

    *npts = num_points;
    return 0;
}

/*******************************************************************************
 * 配置文件I/O
 ******************************************************************************/

int DataGen_LoadConfig(const char *filename)
{
    if (!g_initialized || filename == NULL) {
        return -1;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[DataGen] Failed to open config file: %s\n", filename);
        return -1;
    }

    char line[256];
    int current_channel = -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // 检查节名 [Channel_0]
        if (line[0] == '[') {
            if (sscanf(line, "[Channel_%d]", &current_channel) == 1) {
                if (current_channel < 0 || current_channel >= 8) {
                    current_channel = -1;
                }
            } else if (strstr(line, "[Global]") != NULL) {
                current_channel = -1;
            }
            continue;
        }

        // 解析键值对
        char key[64], value[64];
        if (sscanf(line, "%63s = %63s", key, value) == 2) {
            if (current_channel >= 0 && current_channel < 8) {
                // 通道配置
                ChannelConfig *cfg = &g_config.channels[current_channel];

                if (strcmp(key, "frequency") == 0) {
                    cfg->frequency = atof(value);
                } else if (strcmp(key, "amplitude") == 0) {
                    cfg->amplitude = atof(value);
                } else if (strcmp(key, "offset") == 0) {
                    cfg->offset = atof(value);
                } else if (strcmp(key, "phase_offset") == 0) {
                    cfg->phase_offset = atof(value) * M_PI / 180.0;  // deg to rad
                } else if (strcmp(key, "noise_level") == 0) {
                    cfg->noise_level = atof(value);
                } else if (strcmp(key, "harmonics") == 0) {
                    cfg->enable_harmonics = atoi(value);
                } else if (strcmp(key, "harmonic2_amp") == 0) {
                    cfg->harmonic2_amp = atof(value);
                }
            } else {
                // 全局配置
                if (strcmp(key, "sample_rate") == 0) {
                    g_config.sample_rate = atof(value);
                } else if (strcmp(key, "waveform_points") == 0) {
                    g_config.waveform_points = atoi(value);
                } else if (strcmp(key, "global_drift") == 0) {
                    g_config.global_drift = atof(value);
                }
            }
        }
    }

    fclose(fp);
    printf("[DataGen] Configuration loaded from %s\n", filename);
    return 0;
}

int DataGen_SaveConfig(const char *filename)
{
    if (!g_initialized || filename == NULL) {
        return -1;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "# BPMIOC Simulator Configuration\n");
    fprintf(fp, "# Auto-generated\n\n");

    fprintf(fp, "[Global]\n");
    fprintf(fp, "sample_rate = %.0f\n", g_config.sample_rate);
    fprintf(fp, "waveform_points = %d\n", g_config.waveform_points);
    fprintf(fp, "global_drift = %.3f\n\n", g_config.global_drift);

    for (int ch = 0; ch < 8; ch++) {
        ChannelConfig *cfg = &g_config.channels[ch];

        fprintf(fp, "[Channel_%d]\n", ch);
        fprintf(fp, "frequency = %.3f\n", cfg->frequency);
        fprintf(fp, "amplitude = %.3f\n", cfg->amplitude);
        fprintf(fp, "offset = %.3f\n", cfg->offset);
        fprintf(fp, "phase_offset = %.1f\n", cfg->phase_offset * 180.0 / M_PI);
        fprintf(fp, "noise_level = %.3f\n", cfg->noise_level);
        fprintf(fp, "harmonics = %d\n", cfg->enable_harmonics);
        fprintf(fp, "harmonic2_amp = %.3f\n\n", cfg->harmonic2_amp);
    }

    fclose(fp);
    return 0;
}
