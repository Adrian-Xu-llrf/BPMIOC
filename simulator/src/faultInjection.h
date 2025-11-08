/**
 * faultInjection.h - Fault Injection System Header
 *
 * 模拟各种硬件故障和异常情况，用于稳定性测试
 *
 * Author: BPMIOC Simulator
 * Date: 2025-11-08
 */

#ifndef FAULT_INJECTION_H
#define FAULT_INJECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 故障类型定义
 ******************************************************************************/

typedef enum {
    FAULT_NONE                  = 0,   // 无故障

    // 通道故障（1-10）
    FAULT_CHANNEL_DEAD          = 1,   // 通道死区（无信号）
    FAULT_CHANNEL_NOISY         = 2,   // 通道噪声过大
    FAULT_CHANNEL_SATURATED     = 3,   // 通道饱和
    FAULT_CHANNEL_STUCK         = 4,   // 通道卡死（值不变）
    FAULT_CHANNEL_DRIFT         = 5,   // 通道漂移

    // 间歇性故障（11-20）
    FAULT_INTERMITTENT_DROPOUT  = 11,  // 间歇性数据丢失
    FAULT_INTERMITTENT_SPIKE    = 12,  // 间歇性尖峰
    FAULT_INTERMITTENT_GLITCH   = 13,  // 间歇性毛刺

    // 系统故障（21-30）
    FAULT_SLOW_RESPONSE         = 21,  // 慢响应（延迟）
    FAULT_INIT_FAILURE          = 22,  // 初始化失败
    FAULT_PERIODIC_TIMEOUT      = 23,  // 周期性超时
    FAULT_MEMORY_CORRUPTION     = 24,  // 内存损坏模拟

    // 数据异常（31-40）
    FAULT_PHASE_JUMP            = 31,  // 相位突变
    FAULT_AMPLITUDE_DRIFT       = 32,  // 幅度漂移
    FAULT_FREQUENCY_SHIFT       = 33,  // 频率偏移
    FAULT_NOISE_BURST           = 34,  // 噪声突发

    // 极端情况（41-50）
    FAULT_OVER_RANGE            = 41,  // 超量程
    FAULT_UNDER_RANGE           = 42,  // 欠量程
    FAULT_INVALID_DATA          = 43,  // 无效数据（NaN/Inf）
    FAULT_CHECKSUM_ERROR        = 44,  // 校验和错误

    FAULT_MAX                   = 50
} FaultType;

/*******************************************************************************
 * 故障配置结构
 ******************************************************************************/

/**
 * 故障注入配置
 */
typedef struct {
    FaultType type;             // 故障类型
    int enabled;                // 是否启用
    int affected_channels[8];   // 受影响的通道（1=受影响）

    // 概率和频率
    double probability;         // 触发概率 (0.0-1.0)
    double frequency;           // 触发频率 (Hz, 仅用于间歇性故障)
    double duration;            // 持续时间 (s, 仅用于间歇性故障)

    // 参数
    double param1;              // 参数1（具体含义取决于故障类型）
    double param2;              // 参数2
    double param3;              // 参数3

    // 统计
    int trigger_count;          // 触发次数
    double last_trigger_time;   // 最后触发时间
} FaultConfig;

/**
 * 故障注入状态
 */
typedef struct {
    int initialized;
    int num_faults;
    FaultConfig faults[FAULT_MAX];
    double current_time;
    char config_file[256];
} FaultState;

/*******************************************************************************
 * API函数
 ******************************************************************************/

/**
 * 初始化故障注入系统
 * @return 0=成功, -1=失败
 */
int FaultInjection_Init(void);

/**
 * 加载故障配置文件
 * @param filename 配置文件路径
 * @return 0=成功, -1=失败
 */
int FaultInjection_LoadConfig(const char *filename);

/**
 * 保存配置到文件
 * @param filename 配置文件路径
 * @return 0=成功, -1=失败
 */
int FaultInjection_SaveConfig(const char *filename);

/**
 * 启用/禁用故障
 * @param type 故障类型
 * @param enable 1=启用, 0=禁用
 * @return 0=成功, -1=失败
 */
int FaultInjection_SetFault(FaultType type, int enable);

/**
 * 配置故障参数
 * @param type 故障类型
 * @param config 配置结构
 * @return 0=成功, -1=失败
 */
int FaultInjection_ConfigureFault(FaultType type, const FaultConfig *config);

/**
 * 应用故障到数据
 * @param Amp [in/out] 幅度数据
 * @param Phase [in/out] 相位数据
 * @param Power [in/out] 功率数据
 * @param num_channels 通道数
 * @return 0=正常, >0=触发的故障数
 */
int FaultInjection_Apply(float *Amp, float *Phase, float *Power, int num_channels);

/**
 * 更新时间（用于时间相关的故障）
 * @param delta_time 时间增量 (s)
 */
void FaultInjection_UpdateTime(double delta_time);

/**
 * 重置所有故障统计
 */
void FaultInjection_ResetStatistics(void);

/**
 * 获取故障统计信息
 * @param buffer [out] 统计信息缓冲区
 * @param maxlen 最大长度
 * @return 0=成功, -1=失败
 */
int FaultInjection_GetStatistics(char *buffer, int maxlen);

/**
 * 获取故障状态
 * @return 故障状态结构指针
 */
FaultState* FaultInjection_GetState(void);

/*******************************************************************************
 * 辅助函数
 ******************************************************************************/

/**
 * 获取故障类型名称
 * @param type 故障类型
 * @return 名称字符串
 */
const char* FaultInjection_GetTypeName(FaultType type);

/**
 * 从名称获取故障类型
 * @param name 名称字符串
 * @return 故障类型，未知返回FAULT_NONE
 */
FaultType FaultInjection_GetTypeFromName(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_INJECTION_H */
