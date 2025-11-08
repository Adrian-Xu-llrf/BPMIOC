/**
 * faultInjection.c - Fault Injection System Implementation
 *
 * 简化版实现 - 核心故障注入功能
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "faultInjection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static FaultState g_fault_state = {0};

const char* FaultInjection_GetTypeName(FaultType type) {
    switch(type) {
        case FAULT_CHANNEL_DEAD: return "CHANNEL_DEAD";
        case FAULT_CHANNEL_NOISY: return "CHANNEL_NOISY";
        case FAULT_CHANNEL_SATURATED: return "CHANNEL_SATURATED";
        case FAULT_INTERMITTENT_SPIKE: return "INTERMITTENT_SPIKE";
        case FAULT_PHASE_JUMP: return "PHASE_JUMP";
        case FAULT_AMPLITUDE_DRIFT: return "AMPLITUDE_DRIFT";
        default: return "UNKNOWN";
    }
}

int FaultInjection_Init(void) {
    memset(&g_fault_state, 0, sizeof(FaultState));
    g_fault_state.initialized = 1;
    g_fault_state.num_faults = 0;
    return 0;
}

int FaultInjection_SetFault(FaultType type, int enable) {
    if (type <= FAULT_NONE || type >= FAULT_MAX) return -1;

    FaultConfig *cfg = &g_fault_state.faults[type];
    cfg->type = type;
    cfg->enabled = enable ? 1 : 0;
    cfg->probability = 0.1;  // 默认10%概率

    if (enable && g_fault_state.num_faults < FAULT_MAX) {
        g_fault_state.num_faults++;
    }

    printf("[FaultInject] %s: %s\n",
           FaultInjection_GetTypeName(type),
           enable ? "ENABLED" : "DISABLED");
    return 0;
}

int FaultInjection_Apply(float *Amp, float *Phase, float *Power, int num_channels) {
    if (!g_fault_state.initialized || num_channels <= 0) return 0;

    int faults_triggered = 0;

    for (int ft = 1; ft < FAULT_MAX; ft++) {
        FaultConfig *cfg = &g_fault_state.faults[ft];
        if (!cfg->enabled) continue;

        // 简单概率触发
        double r = (double)rand() / RAND_MAX;
        if (r > cfg->probability) continue;

        faults_triggered++;
        cfg->trigger_count++;

        // 应用故障效果
        switch ((FaultType)ft) {
            case FAULT_CHANNEL_DEAD:
                // 通道0死区
                if (num_channels > 0) {
                    Amp[0] = 0.0f;
                    Phase[0] = 0.0f;
                    Power[0] = 0.0f;
                }
                break;

            case FAULT_CHANNEL_NOISY:
                // 所有通道添加大噪声
                for (int ch = 0; ch < num_channels; ch++) {
                    float noise = ((rand() % 1000) / 500.0f - 1.0f) * 0.5f;
                    Amp[ch] += noise;
                }
                break;

            case FAULT_CHANNEL_SATURATED:
                // 通道1饱和
                if (num_channels > 1) {
                    Amp[1] = 10.0f;  // 饱和到最大值
                    Power[1] = 5000.0f;
                }
                break;

            case FAULT_INTERMITTENT_SPIKE:
                // 随机通道尖峰
                {
                    int ch = rand() % num_channels;
                    Amp[ch] *= 5.0f;  // 5倍尖峰
                }
                break;

            case FAULT_PHASE_JUMP:
                // 相位突变
                for (int ch = 0; ch < num_channels; ch++) {
                    Phase[ch] += 180.0f;
                    if (Phase[ch] > 180.0f) Phase[ch] -= 360.0f;
                }
                break;

            case FAULT_AMPLITUDE_DRIFT:
                // 幅度漂移
                for (int ch = 0; ch < num_channels; ch++) {
                    Amp[ch] *= 1.2f;  // 20%漂移
                }
                break;

            default:
                break;
        }
    }

    return faults_triggered;
}

void FaultInjection_UpdateTime(double delta_time) {
    g_fault_state.current_time += delta_time;
}

int FaultInjection_GetStatistics(char *buffer, int maxlen) {
    if (!buffer || maxlen < 1) return -1;

    int len = 0;
    len += snprintf(buffer + len, maxlen - len, "Fault Injection Statistics:\n");
    len += snprintf(buffer + len, maxlen - len, "  Active faults: %d\n", g_fault_state.num_faults);
    len += snprintf(buffer + len, maxlen - len, "  Runtime: %.1f s\n", g_fault_state.current_time);

    for (int ft = 1; ft < FAULT_MAX; ft++) {
        FaultConfig *cfg = &g_fault_state.faults[ft];
        if (cfg->enabled || cfg->trigger_count > 0) {
            len += snprintf(buffer + len, maxlen - len,
                          "  %s: triggers=%d\n",
                          FaultInjection_GetTypeName((FaultType)ft),
                          cfg->trigger_count);
        }
    }

    return 0;
}

FaultState* FaultInjection_GetState(void) {
    return &g_fault_state;
}

int FaultInjection_LoadConfig(const char *filename) {
    // 简化实现：暂不支持文件加载
    printf("[FaultInject] LoadConfig: %s (not implemented)\n", filename);
    return -1;
}

int FaultInjection_SaveConfig(const char *filename) {
    printf("[FaultInject] SaveConfig: %s (not implemented)\n", filename);
    return -1;
}

void FaultInjection_ResetStatistics(void) {
    for (int ft = 0; ft < FAULT_MAX; ft++) {
        g_fault_state.faults[ft].trigger_count = 0;
    }
    g_fault_state.current_time = 0.0;
}

int FaultInjection_ConfigureFault(FaultType type, const FaultConfig *config) {
    if (type <= FAULT_NONE || type >= FAULT_MAX || !config) return -1;
    g_fault_state.faults[type] = *config;
    return 0;
}

FaultType FaultInjection_GetTypeFromName(const char *name) {
    if (!name) return FAULT_NONE;
    if (strcmp(name, "CHANNEL_DEAD") == 0) return FAULT_CHANNEL_DEAD;
    if (strcmp(name, "CHANNEL_NOISY") == 0) return FAULT_CHANNEL_NOISY;
    // ... 更多映射
    return FAULT_NONE;
}
