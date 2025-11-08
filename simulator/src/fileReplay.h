/**
 * fileReplay.h - File Replay Module Header
 *
 * 从CSV文件回放真实FPGA数据
 * 用于算法验证和回归测试
 *
 * Author: BPMIOC Simulator
 * Date: 2025-11-08
 */

#ifndef FILE_REPLAY_H
#define FILE_REPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 数据结构
 ******************************************************************************/

/**
 * 单个采样点的数据
 */
typedef struct {
    double timestamp;       // 时间戳 (s)
    float amp[8];          // 8通道幅度 (V)
    float phase[8];        // 8通道相位 (deg)
    float power[8];        // 8通道功率 (W)
} RfDataFrame;

/**
 * 回放状态
 */
typedef struct {
    RfDataFrame *frames;    // 数据帧数组
    int num_frames;         // 总帧数
    int current_frame;      // 当前帧索引
    double playback_speed;  // 播放速度 (1.0=正常)
    int loop_mode;          // 循环模式 (1=循环, 0=播放一次)
    int paused;             // 暂停状态
    char filename[256];     // 数据文件名
} ReplayState;

/*******************************************************************************
 * API函数
 ******************************************************************************/

/**
 * 初始化回放模块
 * @return 0=成功, -1=失败
 */
int FileReplay_Init(void);

/**
 * 加载数据文件
 * @param filename 数据文件路径 (CSV格式)
 * @return 0=成功, -1=失败
 */
int FileReplay_LoadFile(const char *filename);

/**
 * 卸载当前数据
 */
void FileReplay_Unload(void);

/**
 * 获取下一帧数据
 * @param Amp [out] 8通道幅度
 * @param Phase [out] 8通道相位
 * @param Power [out] 8通道功率
 * @return 0=成功, -1=失败
 */
int FileReplay_GetNextFrame(float *Amp, float *Phase, float *Power);

/**
 * 重置到文件开头
 */
void FileReplay_Rewind(void);

/**
 * 设置播放速度
 * @param speed 速度倍数 (0.1 - 10.0)
 */
void FileReplay_SetSpeed(double speed);

/**
 * 获取播放速度
 * @return 当前速度
 */
double FileReplay_GetSpeed(void);

/**
 * 设置循环模式
 * @param loop 1=循环播放, 0=播放一次
 */
void FileReplay_SetLoopMode(int loop);

/**
 * 暂停/继续播放
 * @param pause 1=暂停, 0=继续
 */
void FileReplay_SetPause(int pause);

/**
 * 获取当前状态
 * @return 回放状态结构指针
 */
ReplayState* FileReplay_GetState(void);

/**
 * 获取进度
 * @return 进度 (0.0 - 1.0)
 */
double FileReplay_GetProgress(void);

/**
 * 跳转到指定位置
 * @param position 位置 (0.0 - 1.0)
 */
void FileReplay_Seek(double position);

/**
 * 生成示例数据文件
 * @param filename 输出文件路径
 * @param duration 时长 (s)
 * @param interval 采样间隔 (s)
 * @return 0=成功, -1=失败
 */
int FileReplay_GenerateSampleFile(const char *filename, double duration, double interval);

#ifdef __cplusplus
}
#endif

#endif /* FILE_REPLAY_H */
