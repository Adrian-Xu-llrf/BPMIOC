/**
 * fileReplay.c - File Replay Module Implementation
 *
 * 从CSV文件回放真实FPGA数据
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "fileReplay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*******************************************************************************
 * 全局状态
 ******************************************************************************/

static ReplayState g_replay_state = {0};
static int g_initialized = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/*******************************************************************************
 * 内部辅助函数
 ******************************************************************************/

/**
 * 解析CSV行
 */
static int parse_csv_line(const char *line, RfDataFrame *frame)
{
    // 格式: timestamp, ch0_amp, ch0_phase, ch1_amp, ch1_phase, ..., ch7_amp, ch7_phase
    const char *ptr = line;
    char *endptr;

    // 解析时间戳
    frame->timestamp = strtod(ptr, &endptr);
    if (ptr == endptr) {
        return -1;
    }
    ptr = endptr;

    // 解析8个通道
    for (int ch = 0; ch < 8; ch++) {
        // 跳过逗号
        while (*ptr == ',' || *ptr == ' ') ptr++;

        // 解析幅度
        frame->amp[ch] = strtof(ptr, &endptr);
        if (ptr == endptr) {
            return -1;
        }
        ptr = endptr;

        // 跳过逗号
        while (*ptr == ',' || *ptr == ' ') ptr++;

        // 解析相位
        frame->phase[ch] = strtof(ptr, &endptr);
        if (ptr == endptr) {
            return -1;
        }
        ptr = endptr;

        // 计算功率 (P = A^2 * 50Ω)
        frame->power[ch] = frame->amp[ch] * frame->amp[ch] * 50.0f;
    }

    return 0;
}

/*******************************************************************************
 * 公共API实现
 ******************************************************************************/

int FileReplay_Init(void)
{
    if (g_initialized) {
        return 0;
    }

    memset(&g_replay_state, 0, sizeof(ReplayState));
    g_replay_state.playback_speed = 1.0;
    g_replay_state.loop_mode = 1;  // 默认循环播放
    g_replay_state.paused = 0;

    g_initialized = 1;
    return 0;
}

int FileReplay_LoadFile(const char *filename)
{
    if (!g_initialized || filename == NULL) {
        return -1;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[FileReplay] Failed to open file: %s\n", filename);
        return -1;
    }

    // 先卸载旧数据
    FileReplay_Unload();

    pthread_mutex_lock(&g_mutex);

    // 保存文件名
    snprintf(g_replay_state.filename, sizeof(g_replay_state.filename), "%s", filename);

    // 第一遍：计算行数
    char line[1024];
    int line_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        line_count++;
    }

    if (line_count == 0) {
        fprintf(stderr, "[FileReplay] No data in file: %s\n", filename);
        fclose(fp);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    printf("[FileReplay] Found %d data frames in file\n", line_count);

    // 分配内存
    g_replay_state.frames = (RfDataFrame *)malloc(line_count * sizeof(RfDataFrame));
    if (g_replay_state.frames == NULL) {
        fprintf(stderr, "[FileReplay] Memory allocation failed\n");
        fclose(fp);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    // 第二遍：读取数据
    rewind(fp);
    int frame_index = 0;

    while (fgets(line, sizeof(line), fp) != NULL && frame_index < line_count) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // 解析行
        if (parse_csv_line(line, &g_replay_state.frames[frame_index]) == 0) {
            frame_index++;
        } else {
            fprintf(stderr, "[FileReplay] Failed to parse line %d\n", frame_index + 1);
        }
    }

    fclose(fp);

    g_replay_state.num_frames = frame_index;
    g_replay_state.current_frame = 0;

    pthread_mutex_unlock(&g_mutex);

    printf("[FileReplay] Loaded %d frames from %s\n", frame_index, filename);
    printf("[FileReplay] Duration: %.1f seconds\n",
           g_replay_state.frames[frame_index - 1].timestamp);

    return 0;
}

void FileReplay_Unload(void)
{
    pthread_mutex_lock(&g_mutex);

    if (g_replay_state.frames != NULL) {
        free(g_replay_state.frames);
        g_replay_state.frames = NULL;
    }

    g_replay_state.num_frames = 0;
    g_replay_state.current_frame = 0;
    memset(g_replay_state.filename, 0, sizeof(g_replay_state.filename));

    pthread_mutex_unlock(&g_mutex);
}

int FileReplay_GetNextFrame(float *Amp, float *Phase, float *Power)
{
    if (!g_initialized || Amp == NULL || Phase == NULL || Power == NULL) {
        return -1;
    }

    pthread_mutex_lock(&g_mutex);

    // 检查是否有数据
    if (g_replay_state.frames == NULL || g_replay_state.num_frames == 0) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    // 检查暂停状态
    if (g_replay_state.paused) {
        // 暂停时返回当前帧
        RfDataFrame *frame = &g_replay_state.frames[g_replay_state.current_frame];
        memcpy(Amp, frame->amp, 8 * sizeof(float));
        memcpy(Phase, frame->phase, 8 * sizeof(float));
        memcpy(Power, frame->power, 8 * sizeof(float));
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    // 获取当前帧
    RfDataFrame *frame = &g_replay_state.frames[g_replay_state.current_frame];

    // 复制数据
    memcpy(Amp, frame->amp, 8 * sizeof(float));
    memcpy(Phase, frame->phase, 8 * sizeof(float));
    memcpy(Power, frame->power, 8 * sizeof(float));

    // 推进到下一帧
    g_replay_state.current_frame++;

    // 检查是否到达结尾
    if (g_replay_state.current_frame >= g_replay_state.num_frames) {
        if (g_replay_state.loop_mode) {
            // 循环模式：回到开头
            g_replay_state.current_frame = 0;
        } else {
            // 非循环模式：停留在最后一帧
            g_replay_state.current_frame = g_replay_state.num_frames - 1;
        }
    }

    pthread_mutex_unlock(&g_mutex);
    return 0;
}

void FileReplay_Rewind(void)
{
    pthread_mutex_lock(&g_mutex);
    g_replay_state.current_frame = 0;
    pthread_mutex_unlock(&g_mutex);
}

void FileReplay_SetSpeed(double speed)
{
    if (speed < 0.1) speed = 0.1;
    if (speed > 10.0) speed = 10.0;

    pthread_mutex_lock(&g_mutex);
    g_replay_state.playback_speed = speed;
    pthread_mutex_unlock(&g_mutex);

    printf("[FileReplay] Playback speed set to %.2fx\n", speed);
}

double FileReplay_GetSpeed(void)
{
    return g_replay_state.playback_speed;
}

void FileReplay_SetLoopMode(int loop)
{
    pthread_mutex_lock(&g_mutex);
    g_replay_state.loop_mode = loop ? 1 : 0;
    pthread_mutex_unlock(&g_mutex);

    printf("[FileReplay] Loop mode: %s\n", loop ? "ON" : "OFF");
}

void FileReplay_SetPause(int pause)
{
    pthread_mutex_lock(&g_mutex);
    g_replay_state.paused = pause ? 1 : 0;
    pthread_mutex_unlock(&g_mutex);

    printf("[FileReplay] Playback %s\n", pause ? "PAUSED" : "RESUMED");
}

ReplayState* FileReplay_GetState(void)
{
    return &g_replay_state;
}

double FileReplay_GetProgress(void)
{
    if (g_replay_state.num_frames == 0) {
        return 0.0;
    }

    return (double)g_replay_state.current_frame / g_replay_state.num_frames;
}

void FileReplay_Seek(double position)
{
    if (position < 0.0) position = 0.0;
    if (position > 1.0) position = 1.0;

    pthread_mutex_lock(&g_mutex);

    if (g_replay_state.num_frames > 0) {
        g_replay_state.current_frame = (int)(position * g_replay_state.num_frames);
        if (g_replay_state.current_frame >= g_replay_state.num_frames) {
            g_replay_state.current_frame = g_replay_state.num_frames - 1;
        }
    }

    pthread_mutex_unlock(&g_mutex);

    printf("[FileReplay] Seeked to %.1f%%\n", position * 100.0);
}

/*******************************************************************************
 * 示例数据生成
 ******************************************************************************/

int FileReplay_GenerateSampleFile(const char *filename, double duration, double interval)
{
    if (filename == NULL || duration <= 0.0 || interval <= 0.0) {
        return -1;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "[FileReplay] Failed to create file: %s\n", filename);
        return -1;
    }

    // 写入文件头
    fprintf(fp, "# BPMIOC Sample RF Data\n");
    fprintf(fp, "# Format: timestamp(s), ch0_amp, ch0_phase, ch1_amp, ch1_phase, ..., ch7_amp, ch7_phase\n");
    fprintf(fp, "# Generated sample data for testing\n");
    fprintf(fp, "#\n");

    // 生成数据点
    int num_points = (int)(duration / interval);

    printf("[FileReplay] Generating %d data points...\n", num_points);

    for (int i = 0; i < num_points; i++) {
        double t = i * interval;

        fprintf(fp, "%.3f", t);

        // 生成8个通道的数据
        for (int ch = 0; ch < 8; ch++) {
            // 幅度：正弦波 + 噪声
            double amp = 4.0 + 1.0 * sin(2.0 * M_PI * 0.5 * t + ch * M_PI / 4.0);
            amp += ((rand() % 100) / 1000.0 - 0.05);  // ±5% 噪声

            // 相位：慢速漂移
            double phase = 90.0 * sin(2.0 * M_PI * 0.1 * t + ch * M_PI / 8.0);
            phase += ((rand() % 100) / 10.0 - 5.0);  // ±5度抖动

            fprintf(fp, ", %.3f, %.1f", amp, phase);
        }

        fprintf(fp, "\n");
    }

    fclose(fp);

    printf("[FileReplay] Generated sample file: %s\n", filename);
    printf("[FileReplay]   Duration: %.1f seconds\n", duration);
    printf("[FileReplay]   Interval: %.3f seconds\n", interval);
    printf("[FileReplay]   Points: %d\n", num_points);

    return 0;
}
