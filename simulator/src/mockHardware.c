/**
 * mockHardware.c - Mock Hardware Library Implementation
 *
 * 模拟FPGA硬件行为，提供可配置的数据源
 *
 * 支持的模拟模式：
 *   0 - 正弦波：平滑的正弦波变化，适合基础测试
 *   1 - 随机噪声：随机波动，适合稳定性测试
 *   2 - 文件回放：从真实数据文件回放，适合算法验证
 *
 * 环境变量配置：
 *   BPMIOC_SIM_MODE      - 模拟模式 (0/1/2)
 *   BPMIOC_SIM_CONFIG    - 配置文件路径
 *   BPMIOC_DATA_FILE     - 回放数据文件路径
 *   BPMIOC_DEBUG_LEVEL   - 调试级别 (0-4)
 *   BPMIOC_FAULT_CONFIG  - 故障注入配置
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "mockHardware.h"
#include "dataGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/*******************************************************************************
 * 常量定义
 ******************************************************************************/

#define NUM_CHANNELS 8
#define MAX_WAVEFORM_POINTS 100000
#define DEFAULT_WAVEFORM_POINTS 10000

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*******************************************************************************
 * 全局状态变量
 ******************************************************************************/

// 初始化状态
static int g_initialized = 0;

// 模拟模式
static int g_simulation_mode = 0;  // 0=正弦, 1=随机, 2=回放

// 调试级别
static int g_debug_level = 1;  // 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG, 4=TRACE

// 模拟时间（秒）
static double g_sim_time = 0.0;

// 线程安全锁
static pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;

// 最后一次错误信息
static char g_last_error[256] = {0};

/*******************************************************************************
 * 模拟数据缓冲区
 ******************************************************************************/

// RF数据
static float g_amp[NUM_CHANNELS] = {0};
static float g_phase[NUM_CHANNELS] = {0};
static float g_power[NUM_CHANNELS] = {0};

// BPM数据
static float g_vbpm[NUM_CHANNELS] = {0};
static float g_ibpm[NUM_CHANNELS] = {0};

// CRI数据
static float g_vcri[NUM_CHANNELS] = {0};
static float g_icri[NUM_CHANNELS] = {0};

// 相位设置
static int g_pset[NUM_CHANNELS] = {0};

// 波形缓冲区
static float g_waveform[NUM_CHANNELS][MAX_WAVEFORM_POINTS];
static int g_waveform_npts = DEFAULT_WAVEFORM_POINTS;

// 温度
static float g_temperature[4] = {35.0, 37.0, 40.0, 42.0};  // 4个传感器

// 电源
static float g_voltage[3] = {5.0, 3.3, 12.0};  // +5V, +3.3V, +12V
static float g_current[3] = {2.0, 1.5, 0.8};   // 电流

/*******************************************************************************
 * 配置参数
 ******************************************************************************/

/*******************************************************************************
 * 调试宏
 ******************************************************************************/

#define LOG_ERROR(fmt, ...) \
    do { \
        if (g_debug_level >= 1) { \
            printf("[MOCK ERROR] " fmt "\n", ##__VA_ARGS__); \
        } \
        snprintf(g_last_error, sizeof(g_last_error), fmt, ##__VA_ARGS__); \
    } while(0)

#define LOG_INFO(fmt, ...) \
    do { \
        if (g_debug_level >= 2) { \
            printf("[MOCK INFO] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_DEBUG(fmt, ...) \
    do { \
        if (g_debug_level >= 3) { \
            printf("[MOCK DEBUG] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_TRACE(fmt, ...) \
    do { \
        if (g_debug_level >= 4) { \
            printf("[MOCK TRACE] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

/*******************************************************************************
 * 内部辅助函数
 ******************************************************************************/

/**
 * 生成正弦波数据（使用dataGenerator）
 */
static void generate_sine_data(void)
{
    // 使用dataGenerator生成RF数据
    DataGen_SetTime(g_sim_time);
    DataGen_GenerateRfSnapshot(g_amp, g_phase, g_power);

    // BPM和CRI数据（使用简单模型）
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        // BPM电压/电流
        g_vbpm[ch] = g_amp[ch] * 0.8;
        g_ibpm[ch] = g_amp[ch] * 0.05;

        // CRI电压/电流
        g_vcri[ch] = 5.0 + 0.5 * sin(2.0 * M_PI * 0.2 * g_sim_time);
        g_icri[ch] = 1.2 + 0.1 * sin(2.0 * M_PI * 0.3 * g_sim_time);

        // 相位设置值
        g_pset[ch] = (int)(g_phase[ch]);
    }
}

/**
 * 生成随机噪声数据
 */
static void generate_random_data(void)
{
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        // 幅度: 4.0 ± 1.0V随机
        g_amp[ch] = 4.0 + ((rand() % 2000) / 1000.0 - 1.0);

        // 相位: -180 ~ +180随机
        g_phase[ch] = ((rand() % 36000) / 100.0) - 180.0;

        // 功率
        g_power[ch] = g_amp[ch] * g_amp[ch] * 50.0;

        // BPM
        g_vbpm[ch] = g_amp[ch] * 0.8 + ((rand() % 100) / 1000.0);
        g_ibpm[ch] = g_amp[ch] * 0.05 + ((rand() % 10) / 10000.0);

        // CRI
        g_vcri[ch] = 5.0 + ((rand() % 1000) / 1000.0 - 0.5);
        g_icri[ch] = 1.2 + ((rand() % 200) / 1000.0 - 0.1);

        // 相位设置
        g_pset[ch] = (int)(g_phase[ch]);
    }
}

/**
 * 生成波形数据（使用dataGenerator）
 */
static void generate_waveform_data(int channel)
{
    if (channel < 0 || channel >= NUM_CHANNELS) {
        return;
    }

    int npts = 0;
    DataGen_GenerateWaveform(channel, g_waveform[channel], &npts, MAX_WAVEFORM_POINTS);

    if (npts > 0) {
        g_waveform_npts = npts;
    }
}

/**
 * 更新模拟数据
 */
static void update_simulation_data(void)
{
    pthread_mutex_lock(&g_data_mutex);

    switch (g_simulation_mode) {
        case 0:  // 正弦波
            generate_sine_data();
            break;

        case 1:  // 随机噪声
            generate_random_data();
            break;

        case 2:  // 文件回放（TODO: 在fileReplay.c中实现）
            // 暂时使用正弦波
            generate_sine_data();
            break;

        default:
            generate_sine_data();
            break;
    }

    pthread_mutex_unlock(&g_data_mutex);
}

/*******************************************************************************
 * 公共API实现 - 系统初始化
 ******************************************************************************/

int SystemInit(void)
{
    LOG_INFO("SystemInit called");

    if (g_initialized) {
        LOG_INFO("Already initialized");
        return 0;
    }

    // 读取环境变量配置
    char *env_mode = getenv("BPMIOC_SIM_MODE");
    if (env_mode != NULL) {
        g_simulation_mode = atoi(env_mode);
        LOG_INFO("Simulation mode from env: %d", g_simulation_mode);
    }

    char *env_debug = getenv("BPMIOC_DEBUG_LEVEL");
    if (env_debug != NULL) {
        g_debug_level = atoi(env_debug);
        LOG_INFO("Debug level from env: %d", g_debug_level);
    }

    // 显示模式
    const char *mode_str[] = {"Sine Wave", "Random Noise", "File Replay"};
    if (g_simulation_mode >= 0 && g_simulation_mode <= 2) {
        LOG_INFO("Simulation mode: %s", mode_str[g_simulation_mode]);
    } else {
        LOG_ERROR("Invalid simulation mode: %d, using Sine Wave", g_simulation_mode);
        g_simulation_mode = 0;
    }

    // 初始化随机数种子
    srand(time(NULL));

    // 初始化数据生成器
    DataGen_Init();

    // 尝试从配置文件加载
    char *config_file = getenv("BPMIOC_SIM_CONFIG");
    if (config_file != NULL) {
        if (DataGen_LoadConfig(config_file) == 0) {
            LOG_INFO("Loaded configuration from: %s", config_file);
        } else {
            LOG_INFO("Failed to load config, using defaults");
        }
    }

    // 重置模拟时间
    g_sim_time = 0.0;
    DataGen_ResetTime();

    // 初始化数据
    update_simulation_data();

    // 为所有通道生成初始波形
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        generate_waveform_data(ch);
    }

    g_initialized = 1;

    LOG_INFO("Mock hardware initialized successfully");
    LOG_INFO("  Channels: %d", NUM_CHANNELS);
    LOG_INFO("  Waveform points: %d", g_waveform_npts);
    LOG_INFO("  Simulation mode: %s", mode_str[g_simulation_mode]);

    return 0;
}

int SystemReset(void)
{
    LOG_INFO("SystemReset called");

    g_sim_time = 0.0;
    update_simulation_data();

    return 0;
}

int GetBoardStatus(void)
{
    return g_initialized ? 1 : 0;
}

int GetFirmwareVersion(char *version, int maxlen)
{
    if (version == NULL || maxlen < 1) {
        return -1;
    }

    snprintf(version, maxlen, "MOCK-v1.0.0-beta");
    return 0;
}

/*******************************************************************************
 * RF数据采集
 ******************************************************************************/

int GetRfInfo(float *Amp, float *Phase, float *Power,
              float *VBPM, float *IBPM,
              float *VCRI, float *ICRI,
              int *PSET)
{
    LOG_TRACE("GetRfInfo called");

    if (!g_initialized) {
        LOG_ERROR("Not initialized");
        return -1;
    }

    if (Amp == NULL || Phase == NULL || Power == NULL ||
        VBPM == NULL || IBPM == NULL ||
        VCRI == NULL || ICRI == NULL || PSET == NULL) {
        LOG_ERROR("NULL pointer in GetRfInfo");
        return -1;
    }

    // 模拟硬件采集延迟（真实硬件约100ms）
    // PC上用较短延迟加快测试速度
    usleep(5000);  // 5ms

    // 更新模拟数据
    update_simulation_data();

    // 复制数据到输出缓冲区
    pthread_mutex_lock(&g_data_mutex);

    memcpy(Amp, g_amp, NUM_CHANNELS * sizeof(float));
    memcpy(Phase, g_phase, NUM_CHANNELS * sizeof(float));
    memcpy(Power, g_power, NUM_CHANNELS * sizeof(float));
    memcpy(VBPM, g_vbpm, NUM_CHANNELS * sizeof(float));
    memcpy(IBPM, g_ibpm, NUM_CHANNELS * sizeof(float));
    memcpy(VCRI, g_vcri, NUM_CHANNELS * sizeof(float));
    memcpy(ICRI, g_icri, NUM_CHANNELS * sizeof(float));
    memcpy(PSET, g_pset, NUM_CHANNELS * sizeof(int));

    pthread_mutex_unlock(&g_data_mutex);

    // 增加模拟时间
    g_sim_time += 0.1;  // 100ms per call

    LOG_TRACE("  Ch0: Amp=%.3f, Phase=%.1f, Power=%.1f",
              Amp[0], Phase[0], Power[0]);

    return 0;
}

int GetChannelAmplitude(int channel, float *value)
{
    if (!g_initialized || channel < 0 || channel >= NUM_CHANNELS || value == NULL) {
        return -1;
    }

    pthread_mutex_lock(&g_data_mutex);
    *value = g_amp[channel];
    pthread_mutex_unlock(&g_data_mutex);

    return 0;
}

int GetChannelPhase(int channel, float *value)
{
    if (!g_initialized || channel < 0 || channel >= NUM_CHANNELS || value == NULL) {
        return -1;
    }

    pthread_mutex_lock(&g_data_mutex);
    *value = g_phase[channel];
    pthread_mutex_unlock(&g_data_mutex);

    return 0;
}

int GetChannelPower(int channel, float *value)
{
    if (!g_initialized || channel < 0 || channel >= NUM_CHANNELS || value == NULL) {
        return -1;
    }

    pthread_mutex_lock(&g_data_mutex);
    *value = g_power[channel];
    pthread_mutex_unlock(&g_data_mutex);

    return 0;
}

/*******************************************************************************
 * 波形数据采集
 ******************************************************************************/

int GetTrigWaveform(int channel, float *buffer, int *npts)
{
    LOG_DEBUG("GetTrigWaveform: channel=%d", channel);

    if (!g_initialized) {
        LOG_ERROR("Not initialized");
        return -1;
    }

    if (channel < 0 || channel >= NUM_CHANNELS) {
        LOG_ERROR("Invalid channel: %d", channel);
        return -1;
    }

    if (buffer == NULL || npts == NULL) {
        LOG_ERROR("NULL pointer in GetTrigWaveform");
        return -1;
    }

    // 重新生成波形数据（模拟触发采集）
    generate_waveform_data(channel);

    // 模拟触发延迟
    usleep(10000);  // 10ms

    // 复制波形数据
    pthread_mutex_lock(&g_data_mutex);
    memcpy(buffer, g_waveform[channel], g_waveform_npts * sizeof(float));
    *npts = g_waveform_npts;
    pthread_mutex_unlock(&g_data_mutex);

    LOG_DEBUG("  Returned %d points", *npts);

    return 0;
}

int GetContinuousWaveform(int channel, float *buffer, int *npts)
{
    // 连续波形与触发波形相同实现
    return GetTrigWaveform(channel, buffer, npts);
}

/*******************************************************************************
 * 参数设置 - 写操作
 ******************************************************************************/

int SetPhaseOffset(int channel, float value)
{
    LOG_INFO("SetPhaseOffset: channel=%d, value=%.2f", channel, value);

    if (!g_initialized) {
        return -1;
    }

    if (channel < 0 || channel >= NUM_CHANNELS) {
        LOG_ERROR("Invalid channel: %d", channel);
        return -1;
    }

    if (value < -180.0 || value > 180.0) {
        LOG_ERROR("Invalid phase offset: %.2f (must be -180~+180)", value);
        return -1;
    }

    // Mock实现：只记录，不实际改变模拟数据
    LOG_INFO("  Phase offset set successfully (mock)");

    return 0;
}

int SetAmpOffset(int channel, float value)
{
    LOG_INFO("SetAmpOffset: channel=%d, value=%.2f", channel, value);

    if (!g_initialized || channel < 0 || channel >= NUM_CHANNELS) {
        return -1;
    }

    LOG_INFO("  Amplitude offset set successfully (mock)");
    return 0;
}

int SetPowerThreshold(int channel, float value)
{
    LOG_INFO("SetPowerThreshold: channel=%d, value=%.2f", channel, value);

    if (!g_initialized || channel < 0 || channel >= NUM_CHANNELS) {
        return -1;
    }

    LOG_INFO("  Power threshold set successfully (mock)");
    return 0;
}

int SetSampleRate(int rate)
{
    LOG_INFO("SetSampleRate: rate=%d Hz", rate);

    if (!g_initialized) {
        return -1;
    }

    if (rate < 1000 || rate > 200000000) {
        LOG_ERROR("Invalid sample rate: %d", rate);
        return -1;
    }

    LOG_INFO("  Sample rate set successfully (mock)");
    return 0;
}

int SetTriggerMode(int mode)
{
    LOG_INFO("SetTriggerMode: mode=%d", mode);

    if (!g_initialized) {
        return -1;
    }

    if (mode != 0 && mode != 1) {
        LOG_ERROR("Invalid trigger mode: %d", mode);
        return -1;
    }

    LOG_INFO("  Trigger mode set to %s (mock)", mode ? "External" : "Internal");
    return 0;
}

int SetTriggerLevel(float level)
{
    LOG_INFO("SetTriggerLevel: level=%.2f", level);

    if (!g_initialized) {
        return -1;
    }

    LOG_INFO("  Trigger level set successfully (mock)");
    return 0;
}

/*******************************************************************************
 * BPM特定功能
 ******************************************************************************/

int GetBeamPosition(float *x, float *y)
{
    if (!g_initialized || x == NULL || y == NULL) {
        return -1;
    }

    // 模拟束流位置：轻微抖动
    *x = 0.5 * sin(2.0 * M_PI * 0.3 * g_sim_time);
    *y = 0.3 * sin(2.0 * M_PI * 0.4 * g_sim_time);

    return 0;
}

int GetBeamIntensity(float *intensity)
{
    if (!g_initialized || intensity == NULL) {
        return -1;
    }

    // 模拟束流强度
    *intensity = 100.0 + 10.0 * sin(2.0 * M_PI * 0.2 * g_sim_time);

    return 0;
}

int SetBPMGain(float gain)
{
    LOG_INFO("SetBPMGain: gain=%.2f", gain);

    if (!g_initialized) {
        return -1;
    }

    if (gain < 0.1 || gain > 10.0) {
        LOG_ERROR("Invalid BPM gain: %.2f", gain);
        return -1;
    }

    LOG_INFO("  BPM gain set successfully (mock)");
    return 0;
}

/*******************************************************************************
 * 温度和电源监控
 ******************************************************************************/

int GetTemperature(int sensor, float *temp)
{
    if (!g_initialized || sensor < 0 || sensor >= 4 || temp == NULL) {
        return -1;
    }

    // 温度轻微波动
    *temp = g_temperature[sensor] + 2.0 * sin(2.0 * M_PI * 0.05 * g_sim_time);

    return 0;
}

int GetPowerVoltage(int rail, float *voltage)
{
    if (!g_initialized || rail < 0 || rail >= 3 || voltage == NULL) {
        return -1;
    }

    // 电压轻微波动（±1%）
    *voltage = g_voltage[rail] * (1.0 + 0.01 * sin(2.0 * M_PI * 0.1 * g_sim_time));

    return 0;
}

int GetPowerCurrent(int rail, float *current)
{
    if (!g_initialized || rail < 0 || rail >= 3 || current == NULL) {
        return -1;
    }

    // 电流轻微波动
    *current = g_current[rail] * (1.0 + 0.05 * sin(2.0 * M_PI * 0.15 * g_sim_time));

    return 0;
}

/*******************************************************************************
 * 诊断和调试
 ******************************************************************************/

int SetDebugLevel(int level)
{
    if (level < 0 || level > 4) {
        return -1;
    }

    g_debug_level = level;
    LOG_INFO("Debug level set to %d", level);

    return 0;
}

int GetLastError(char *buffer, int maxlen)
{
    if (buffer == NULL || maxlen < 1) {
        return -1;
    }

    snprintf(buffer, maxlen, "%s", g_last_error);
    return 0;
}

int SelfTest(void)
{
    LOG_INFO("SelfTest: Running mock self-test");

    if (!g_initialized) {
        LOG_ERROR("Not initialized");
        return -1;
    }

    // 模拟自检：总是成功
    LOG_INFO("  All tests passed");
    return 0;
}

/*******************************************************************************
 * 校准功能
 ******************************************************************************/

int Calibrate(int channel)
{
    LOG_INFO("Calibrate: channel=%d", channel);

    if (!g_initialized) {
        return -1;
    }

    if (channel != -1 && (channel < 0 || channel >= NUM_CHANNELS)) {
        LOG_ERROR("Invalid channel: %d", channel);
        return -1;
    }

    if (channel == -1) {
        LOG_INFO("  Calibrating all channels (mock)");
    } else {
        LOG_INFO("  Calibrating channel %d (mock)", channel);
    }

    // 模拟校准延迟
    usleep(100000);  // 100ms

    LOG_INFO("  Calibration completed");
    return 0;
}

int LoadCalibration(const char *filename)
{
    LOG_INFO("LoadCalibration: file=%s", filename ? filename : "NULL");

    if (!g_initialized || filename == NULL) {
        return -1;
    }

    LOG_INFO("  Calibration loaded successfully (mock)");
    return 0;
}

int SaveCalibration(const char *filename)
{
    LOG_INFO("SaveCalibration: file=%s", filename ? filename : "NULL");

    if (!g_initialized || filename == NULL) {
        return -1;
    }

    LOG_INFO("  Calibration saved successfully (mock)");
    return 0;
}

/*******************************************************************************
 * 模拟器特有功能
 ******************************************************************************/

int Mock_SetSimulationMode(int mode)
{
    if (mode < 0 || mode > 2) {
        LOG_ERROR("Invalid simulation mode: %d", mode);
        return -1;
    }

    g_simulation_mode = mode;

    const char *mode_str[] = {"Sine Wave", "Random Noise", "File Replay"};
    LOG_INFO("Simulation mode changed to: %s", mode_str[mode]);

    return 0;
}

int Mock_LoadDataFile(const char *filename)
{
    LOG_INFO("Mock_LoadDataFile: file=%s", filename ? filename : "NULL");

    if (filename == NULL) {
        return -1;
    }

    // TODO: 在fileReplay.c中实现
    LOG_INFO("  File replay not yet implemented");
    return -1;
}

int Mock_SetFault(int fault_type, int enable)
{
    LOG_INFO("Mock_SetFault: type=%d, enable=%d", fault_type, enable);

    // TODO: 实现故障注入
    LOG_INFO("  Fault injection not yet implemented");
    return 0;
}

int Mock_GetStatistics(char *buffer, int maxlen)
{
    if (buffer == NULL || maxlen < 1) {
        return -1;
    }

    snprintf(buffer, maxlen,
             "Mock Hardware Statistics:\n"
             "  Initialized: %s\n"
             "  Simulation Mode: %d\n"
             "  Simulation Time: %.1f s\n"
             "  Debug Level: %d\n"
             "  Channels: %d\n"
             "  Waveform Points: %d\n",
             g_initialized ? "Yes" : "No",
             g_simulation_mode,
             g_sim_time,
             g_debug_level,
             NUM_CHANNELS,
             g_waveform_npts);

    return 0;
}
