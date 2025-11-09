# 硬件函数接口详解

> **阅读时间**: 50分钟
> **难度**: ⭐⭐⭐⭐☆
> **前置知识**: dlopen/dlsym、函数指针、硬件抽象

## 📋 本文目标

- 理解50+硬件函数的分类和作用
- 掌握Mock库和Real库的实现差异
- 学会如何设计新的硬件函数
- 理解硬件抽象层的最佳实践

## 🎯 硬件函数接口概览

### 整体架构

```
driverWrapper.c (驱动层)
       ↓ 调用
[50+硬件函数指针]
       ↓ dlsym绑定
┌──────┴──────┐
│             │
↓             ↓
Mock库      Real库
(PC模拟)   (ZYNQ硬件)
```

**设计目标**:
- **统一接口**: Mock和Real库提供相同的API
- **硬件无关**: 驱动层代码不依赖具体硬件
- **灵活切换**: 运行时决定加载哪个库

## 1. 函数分类

### 1.1 7大类50+函数

```c
// ===== 类别1: 系统管理 (5个) =====
int   SystemInit(void);             // 初始化系统
void  SystemClose(void);            // 关闭系统
int   GetSystemStatus(void);        // 获取系统状态
void  ResetSystem(void);            // 重置系统
const char* GetVersion(void);       // 获取版本

// ===== 类别2: 数据采集 (10个) =====
int   TriggerAllDataReached(void);  // 触发数据采集
void  GetAllWaveData(void);         // 获取所有波形数据
void  StartAcquisition(void);       // 开始采集
void  StopAcquisition(void);        // 停止采集
int   IsDataReady(void);            // 数据是否就绪
void  SetAcquisitionMode(int mode); // 设置采集模式
int   GetAcquisitionMode(void);     // 获取采集模式
void  SetSamplingRate(int rate);    // 设置采样率
int   GetSamplingRate(void);        // 获取采样率
void  ForceTrigger(void);           // 强制触发

// ===== 类别3: RF数据 (15个) =====
float GetRFInfo(int channel, int type);         // 通用RF接口
float GetRF3Amp(void);                          // RF3幅度
float GetRF3Phase(void);                        // RF3相位
float GetRF4Amp(void);                          // RF4幅度
float GetRF4Phase(void);                        // RF4相位
float GetRF5Amp(void);                          // RF5幅度
float GetRF5Phase(void);                        // RF5相位
float GetRF6Amp(void);                          // RF6幅度
float GetRF6Phase(void);                        // RF6相位
float GetCenterFrequency(void);                 // 中心频率
void  SetCenterFrequency(float freq);           // 设置中心频率
float GetRFPower(int channel);                  // RF功率
void  SetRFAttenuation(int channel, int atten); // 设置衰减
int   GetRFAttenuation(int channel);            // 获取衰减
void  CalibrateRF(int channel);                 // RF校准

// ===== 类别4: 位置数据 (12个) =====
float GetXYPosition(int channel);   // 通用XY接口 (0=X1, 1=Y1, 2=X2, 3=Y2)
float GetX1(void);                  // X1位置
float GetY1(void);                  // Y1位置
float GetX2(void);                  // X2位置
float GetY2(void);                  // Y2位置
float GetQ1(void);                  // Q1电荷
float GetQ2(void);                  // Q2电荷
void  GetXYPair(int pair, float *x, float *y);  // 同时获取X和Y
void  SetXOffset(float offset);     // 设置X偏移校准
float GetXOffset(void);             // 获取X偏移
void  SetYOffset(float offset);     // 设置Y偏移校准
float GetYOffset(void);             // 获取Y偏移

// ===== 类别5: Button信号 (10个) =====
float GetButtonSignal(int index);   // 通用Button接口 (0-7)
float GetButton1(void);             // Button1
float GetButton2(void);             // Button2
// ... Button3-8
float GetButtonSum(void);           // Button总和
float GetButtonI(int index);        // Button I分量
float GetButtonQ(int index);        // Button Q分量

// ===== 类别6: 波形数据 (10个) =====
float GetRFWaveData(int channel, int index, int type);   // RF波形单点
float GetXYWaveData(int channel, int index);             // XY波形单点
float GetButtonWaveData(int button, int index);          // Button波形单点
void  GetRFWaveArray(int channel, int type, float *buf); // RF波形数组
void  GetXYWaveArray(int channel, float *buf);           // XY波形数组
void  GetButtonWaveArray(int button, float *buf);        // Button波形数组
void  GetHistoryWave(int channel, float *buf, int *len); // 历史波形
void  ClearHistoryBuffer(void);                          // 清空历史缓冲
int   GetHistoryLength(void);                            // 获取历史长度
void  SetHistoryLength(int len);                         // 设置历史长度

// ===== 类别7: 寄存器操作 (8个) =====
void  SetReg(int addr, int value);  // 写寄存器
int   GetReg(int addr);             // 读寄存器
void  SetRegBit(int addr, int bit, int value);  // 设置bit
int   GetRegBit(int addr, int bit);             // 获取bit
void  ResetReg(int addr);           // 重置寄存器
void  DumpAllRegs(void);            // 打印所有寄存器
void  LoadRegsFromFile(const char *filename);   // 从文件加载
void  SaveRegsToFile(const char *filename);     // 保存到文件
```

**总计**: ~70个函数

## 2. 重点函数详解

### 2.1 SystemInit() - 系统初始化

```c
// 函数签名
int SystemInit(void);

// 返回值
// 0: 成功
// -1: 失败
```

#### Mock实现

```c
// libbpm_mock.c

static int mock_initialized = 0;

int SystemInit(void)
{
    printf("[Mock] SystemInit called\n");

    if (mock_initialized) {
        printf("[Mock] Already initialized\n");
        return 0;
    }

    // 初始化随机数生成器
    srand(time(NULL));

    // 初始化模拟寄存器
    memset(mock_registers, 0, sizeof(mock_registers));
    mock_registers[0] = 1;   // 系统运行
    mock_registers[1] = 100; // 采样率100kHz

    // 创建模拟数据
    initMockData();

    mock_initialized = 1;

    printf("[Mock] System initialized successfully\n");

    return 0;
}
```

#### Real实现 (ZYNQ)

```c
// libbpm_zynq.c

#include "xil_io.h"
#include "xdma.h"

static int real_initialized = 0;
static XDma dma_instance;

int SystemInit(void)
{
    printf("[ZYNQ] SystemInit called\n");

    if (real_initialized) {
        printf("[ZYNQ] Already initialized\n");
        return 0;
    }

    // 1. 初始化FPGA寄存器
    printf("[ZYNQ] Initializing FPGA...\n");
    Xil_Out32(FPGA_CTRL_REG, 0x00000001);  // 启动FPGA

    // 等待FPGA就绪
    int timeout = 1000;
    while (timeout-- > 0) {
        uint32_t status = Xil_In32(FPGA_STATUS_REG);
        if (status & 0x1) {
            printf("[ZYNQ] FPGA ready\n");
            break;
        }
        usleep(1000);  // 1ms
    }

    if (timeout == 0) {
        printf("[ZYNQ] ERROR: FPGA initialization timeout\n");
        return -1;
    }

    // 2. 初始化DMA
    printf("[ZYNQ] Initializing DMA...\n");
    XDma_Config *cfg = XDma_LookupConfig(DMA_DEVICE_ID);
    if (cfg == NULL) {
        printf("[ZYNQ] ERROR: DMA config not found\n");
        return -1;
    }

    int status = XDma_CfgInitialize(&dma_instance, cfg);
    if (status != XST_SUCCESS) {
        printf("[ZYNQ] ERROR: DMA initialization failed\n");
        return -1;
    }

    // 3. 配置ADC
    printf("[ZYNQ] Configuring ADC...\n");
    configureADC(100000);  // 100kHz采样率

    // 4. 设置默认寄存器
    SetReg(0, 1);    // 系统运行
    SetReg(1, 100);  // 采样率
    SetReg(2, 0);    // 软件触发

    real_initialized = 1;

    printf("[ZYNQ] System initialized successfully\n");

    return 0;
}
```

**关键差异**:
- Mock: 简单的标志位设置
- Real: 复杂的硬件初始化（FPGA、DMA、ADC）

### 2.2 TriggerAllDataReached() - 触发数据采集

```c
// 函数签名
int TriggerAllDataReached(void);

// 返回值
// 0: 成功
// -1: 失败
```

#### Mock实现

```c
int TriggerAllDataReached(void)
{
    // 生成模拟数据
    static float t = 0.0;
    t += 0.01;

    // RF数据 (正弦波)
    for (int i = 0; i < buf_len; i++) {
        float phase = t + i * 0.001;

        mock_rf3amp[i] = 100.0 + 10.0 * sin(phase);
        mock_rf3phase[i] = 180.0 * cos(phase * 2.0);

        mock_rf4amp[i] = 95.0 + 5.0 * sin(phase * 1.5);
        mock_rf4phase[i] = 150.0 * cos(phase * 1.2);

        // ... RF5, RF6 类似
    }

    // XY位置 (束流轨迹模拟)
    for (int i = 0; i < buf_len; i++) {
        float phase = t + i * 0.002;

        mock_wave_X1[i] = 2.0 * sin(phase) + 0.1 * sin(phase * 10.0);  // 主轨迹 + 抖动
        mock_wave_Y1[i] = 1.5 * cos(phase) + 0.1 * cos(phase * 8.0);

        mock_wave_X2[i] = 2.1 * sin(phase + 0.1);
        mock_wave_Y2[i] = 1.6 * cos(phase + 0.1);
    }

    // Button信号
    for (int i = 0; i < buf_len; i++) {
        float phase = t + i * 0.001;

        mock_wave_button1[i] = 50.0 + 5.0 * sin(phase);
        mock_wave_button2[i] = 48.0 + 4.0 * sin(phase + M_PI/4);
        mock_wave_button3[i] = 52.0 + 6.0 * sin(phase + M_PI/2);
        mock_wave_button4[i] = 49.0 + 5.5 * sin(phase + 3*M_PI/4);
        // ... button5-8
    }

    return 0;
}
```

#### Real实现 (ZYNQ)

```c
int TriggerAllDataReached(void)
{
    // 1. 发送触发命令到FPGA
    Xil_Out32(FPGA_TRIGGER_REG, 0x1);

    // 2. 等待数据就绪 (轮询或中断)
    int timeout = 1000;
    while (timeout-- > 0) {
        uint32_t status = Xil_In32(FPGA_STATUS_REG);
        if (status & 0x2) {  // bit 1: data ready
            break;
        }
        usleep(100);  // 100μs
    }

    if (timeout == 0) {
        printf("[ZYNQ] ERROR: Data acquisition timeout\n");
        return -1;
    }

    // 3. 通过DMA读取数据
    // RF数据
    dmaRead(FPGA_RF3_AMP_ADDR,  real_rf3amp,  buf_len * sizeof(float));
    dmaRead(FPGA_RF3_PHASE_ADDR, real_rf3phase, buf_len * sizeof(float));
    dmaRead(FPGA_RF4_AMP_ADDR,  real_rf4amp,  buf_len * sizeof(float));
    // ... 其他RF通道

    // XY位置数据
    dmaRead(FPGA_X1_ADDR, real_wave_X1, buf_len * sizeof(float));
    dmaRead(FPGA_Y1_ADDR, real_wave_Y1, buf_len * sizeof(float));
    // ... X2, Y2

    // Button数据
    dmaRead(FPGA_BUTTON1_ADDR, real_wave_button1, buf_len * sizeof(float));
    // ... button2-8

    // 4. 数据后处理 (可选)
    // 例如: 单位转换、校准等
    for (int i = 0; i < buf_len; i++) {
        real_rf3amp[i] *= ADC_TO_VOLT_SCALE;      // ADC计数 → 电压
        real_rf3phase[i] *= ADC_TO_DEGREE_SCALE;  // ADC计数 → 角度
    }

    // 5. 清除触发标志
    Xil_Out32(FPGA_TRIGGER_REG, 0x0);

    return 0;
}

// DMA读取辅助函数
void dmaRead(uint32_t fpga_addr, float *buf, size_t size)
{
    XDma_BdRing *ring = XDma_GetRxRing(&dma_instance);

    // 配置DMA传输
    XDma_BdSetBufAddr(bd, fpga_addr);
    XDma_BdSetLength(bd, size);

    // 启动DMA
    XDma_BdRingToHw(ring, 1, bd);

    // 等待完成
    while (XDma_BdRingFromHw(ring, 1, &bd_ptr) == 0) {
        usleep(10);
    }

    // 拷贝到目标缓冲区
    memcpy(buf, dma_buffer, size);
}
```

**关键差异**:
- Mock: 纯软件生成数据（数学公式）
- Real: 复杂的硬件交互（FPGA触发、DMA传输、数据转换）

### 2.3 GetRFInfo() - 获取RF信息

```c
// 函数签名
float GetRFInfo(int channel, int type);

// 参数
// channel: 3-6 (RF3-RF6)
// type: 0=幅度, 1=相位

// 返回值
// RF信息值
```

#### Mock实现

```c
float GetRFInfo(int channel, int type)
{
    // 参数验证
    if (channel < 3 || channel > 6) {
        printf("[Mock] Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    // 返回最新值 (从buffer的最后一个点)
    float value;

    if (type == 0) {  // 幅度
        switch (channel) {
            case 3: value = mock_rf3amp[buf_len - 1]; break;
            case 4: value = mock_rf4amp[buf_len - 1]; break;
            case 5: value = mock_rf5amp[buf_len - 1]; break;
            case 6: value = mock_rf6amp[buf_len - 1]; break;
        }
    } else if (type == 1) {  // 相位
        switch (channel) {
            case 3: value = mock_rf3phase[buf_len - 1]; break;
            case 4: value = mock_rf4phase[buf_len - 1]; break;
            case 5: value = mock_rf5phase[buf_len - 1]; break;
            case 6: value = mock_rf6phase[buf_len - 1]; break;
        }
    } else {
        value = 0.0;
    }

    return value;
}
```

#### Real实现 (ZYNQ)

```c
float GetRFInfo(int channel, int type)
{
    // 参数验证
    if (channel < 3 || channel > 6) {
        printf("[ZYNQ] Invalid RF channel: %d\n", channel);
        return 0.0;
    }

    // 计算寄存器地址
    uint32_t reg_addr;

    if (type == 0) {  // 幅度
        reg_addr = FPGA_RF_BASE + (channel - 3) * 0x100 + 0x00;
    } else if (type == 1) {  // 相位
        reg_addr = FPGA_RF_BASE + (channel - 3) * 0x100 + 0x04;
    } else {
        return 0.0;
    }

    // 读取FPGA寄存器
    uint32_t raw_value = Xil_In32(reg_addr);

    // 转换为物理单位
    float value;
    if (type == 0) {  // 幅度
        value = raw_value * ADC_TO_VOLT_SCALE;  // 例如: * 0.001
    } else {  // 相位
        value = (int32_t)raw_value * ADC_TO_DEGREE_SCALE;  // 例如: * 0.01
    }

    return value;
}
```

### 2.4 SetReg() / GetReg() - 寄存器操作

```c
// 函数签名
void SetReg(int addr, int value);
int  GetReg(int addr);
```

#### Mock实现

```c
static int mock_registers[100];

void SetReg(int addr, int value)
{
    if (addr < 0 || addr >= 100) {
        printf("[Mock] Invalid register address: %d\n", addr);
        return;
    }

    printf("[Mock] SetReg(%d, %d)\n", addr, value);

    mock_registers[addr] = value;

    // 模拟某些寄存器的副作用
    if (addr == 1) {  // 采样率
        printf("[Mock] Sampling rate changed to %d kHz\n", value);
    } else if (addr == 2) {  // 触发模式
        printf("[Mock] Trigger mode changed to %d\n", value);
    }
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= 100) {
        printf("[Mock] Invalid register address: %d\n", addr);
        return 0;
    }

    return mock_registers[addr];
}
```

#### Real实现 (ZYNQ)

```c
void SetReg(int addr, int value)
{
    if (addr < 0 || addr >= 100) {
        printf("[ZYNQ] Invalid register address: %d\n", addr);
        return;
    }

    // 计算FPGA寄存器地址
    uint32_t fpga_addr = FPGA_REG_BASE + addr * 4;

    // 写入FPGA
    Xil_Out32(fpga_addr, (uint32_t)value);

    // 读回验证
    uint32_t readback = Xil_In32(fpga_addr);
    if (readback != (uint32_t)value) {
        printf("[ZYNQ] WARNING: SetReg verification failed\n");
        printf("  Wrote: %d, Read: %d\n", value, (int)readback);
    }

    printf("[ZYNQ] SetReg(%d, %d) successful\n", addr, value);
}

int GetReg(int addr)
{
    if (addr < 0 || addr >= 100) {
        printf("[ZYNQ] Invalid register address: %d\n", addr);
        return 0;
    }

    uint32_t fpga_addr = FPGA_REG_BASE + addr * 4;
    uint32_t value = Xil_In32(fpga_addr);

    return (int)value;
}
```

## 3. 设计新硬件函数的最佳实践

### 3.1 函数命名规范

```c
// ✅ 好的命名
float GetRFInfo(int channel, int type);      // 清晰、简洁
void  SetCenterFrequency(float freq);        // 动词开头
int   IsDataReady(void);                     // 布尔函数用Is/Has

// ❌ 不好的命名
float rf_info(int ch, int t);                // 不清晰
void  freq(float f);                         // 太简短
int   data_ready();                          // 不符合规范
```

### 3.2 参数设计

```c
// ✅ 好的参数设计
float GetXYPosition(int channel);            // channel: 0-3
void  GetXYPair(float *x, float *y);         // 输出参数用指针

// ❌ 不好的参数设计
float GetPosition(int x_or_y, int which);    // 不清晰
float GetX1AndY1(void);                      // 无法返回两个值
```

### 3.3 错误处理

```c
// ✅ 好的错误处理
float GetRFInfo(int channel, int type)
{
    if (channel < 3 || channel > 6) {
        fprintf(stderr, "ERROR: Invalid channel %d (range: 3-6)\n", channel);
        return 0.0;  // 返回合理默认值
    }

    if (type != 0 && type != 1) {
        fprintf(stderr, "ERROR: Invalid type %d (0=amp, 1=phase)\n", type);
        return 0.0;
    }

    // ... 正常处理 ...
}

// ❌ 不好的错误处理
float GetRFInfo(int channel, int type)
{
    // 没有参数验证，可能崩溃
    return rf_data[channel][type];
}
```

### 3.4 Mock和Real的一致性

```c
// Mock和Real必须有完全相同的函数签名

// ✅ 一致
// libbpm_mock.c
float GetRFInfo(int channel, int type) { ... }

// libbpm_zynq.c
float GetRFInfo(int channel, int type) { ... }

// ❌ 不一致
// libbpm_mock.c
float GetRFInfo(int channel, int type) { ... }

// libbpm_zynq.c
double GetRFInfo(int ch, int t) { ... }  // 返回类型、参数名不同
```

## 4. 添加新硬件函数的完整流程

### 示例：添加GetTemperature()

#### Step 1: 在硬件库中实现

```c
// libbpm_mock.c
float GetTemperature(int sensor)
{
    if (sensor < 0 || sensor > 3) {
        return 0.0;
    }

    // 模拟温度 25℃ ± 5℃
    return 25.0 + (rand() % 100) * 0.1;
}

// libbpm_zynq.c
float GetTemperature(int sensor)
{
    if (sensor < 0 || sensor > 3) {
        return 0.0;
    }

    uint32_t reg_addr = FPGA_TEMP_BASE + sensor * 4;
    uint32_t raw = Xil_In32(reg_addr);

    // 转换为℃ (假设0.01℃/LSB)
    return (float)raw * 0.01;
}
```

#### Step 2: 在driverWrapper.c中声明函数指针

```c
// driverWrapper.c 全局变量区域
static float (*funcGetTemperature)(int sensor);
```

#### Step 3: 在InitDevice()中加载

```c
long InitDevice()
{
    // ... dlopen ...

    // ... 其他dlsym ...

    // 加载GetTemperature
    funcGetTemperature = (float (*)(int))dlsym(handle, "GetTemperature");
    if (funcGetTemperature == NULL) {
        fprintf(stderr, "WARNING: Cannot find GetTemperature: %s\n", dlerror());
        // 继续运行，但功能不可用
    }

    // ...
}
```

#### Step 4: 在ReadData()中使用

```c
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        // ... 现有case ...

        case 29:  // 温度
            if (funcGetTemperature != NULL) {
                return funcGetTemperature(channel);
            } else {
                return 25.0;  // 默认25℃
            }
            break;

        default:
            return 0.0;
    }
}
```

#### Step 5: 编译测试

```bash
# 编译Mock库
gcc -shared -fPIC libbpm_mock.c -o libbpm_mock.so

# 编译Real库
arm-linux-gnueabihf-gcc -shared -fPIC libbpm_zynq.c -o libbpm_zynq.so

# 编译IOC
cd ~/BPMIOC
make

# 测试
./st.cmd
epics> caget LLRF:BPM:Temp1
```

## 5. 调试硬件函数

### 5.1 打印所有函数指针

```c
void dumpFunctionPointers(void)
{
    printf("=== Hardware Function Pointers ===\n");
    printf("funcSystemInit:       %p\n", (void *)funcSystemInit);
    printf("funcTriggerAllDataReached: %p\n", (void *)funcTriggerAllDataReached);
    printf("funcGetRFInfo:        %p\n", (void *)funcGetRFInfo);
    printf("funcGetXYPosition:    %p\n", (void *)funcGetXYPosition);
    // ... 其他函数
    printf("==================================\n");
}
```

### 5.2 记录函数调用

```c
// 包装函数
float GetRFInfo_wrapper(int channel, int type)
{
    printf("[TRACE] GetRFInfo(%d, %d) called\n", channel, type);

    float value = GetRFInfo(channel, type);

    printf("[TRACE] GetRFInfo returned %.3f\n", value);

    return value;
}
```

### 5.3 性能计时

```c
#include <sys/time.h>

float GetRFInfo_timed(int channel, int type)
{
    struct timeval start, end;
    gettimeofday(&start, NULL);

    float value = GetRFInfo(channel, type);

    gettimeofday(&end, NULL);
    long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    if (us > 1000) {  // >1ms
        printf("[PERF] GetRFInfo took %ld us\n", us);
    }

    return value;
}
```

## ❓ 常见问题

### Q1: 如果硬件库中缺少某个函数怎么办？
**A**:
```c
if (funcGetRFInfo == NULL) {
    fprintf(stderr, "WARNING: funcGetRFInfo not available\n");
    // 使用默认值或跳过功能
    return 0.0;
}
```

### Q2: Mock库和Real库可以混用吗？
**A**: 不可以，必须加载完整的库，否则会有符号缺失。

### Q3: 如何在不修改驱动层的情况下添加硬件函数？
**A**: 在设备支持层直接调用（不推荐）：
```c
// devBPMMonitor.c
void *handle = dlopen("libbpm_zynq.so", RTLD_LAZY);
float (*GetTemp)(int) = dlsym(handle, "GetTemperature");
float temp = GetTemp(0);
```

## 📚 延伸阅读

- [05-dlopen-dlsym.md](./05-dlopen-dlsym.md) - 动态加载详解
- [Part 19: Simulator](../part19-simulator-tutorial/) - Mock库完整实现
- EPICS Device Support Reference Manual

## 🎓 本章总结

- ✅ 50+硬件函数分为7大类
- ✅ Mock库提供软件模拟，Real库访问硬件
- ✅ 函数接口必须在两个库中保持一致
- ✅ 通过dlsym动态加载，实现硬件无关
- ✅ 添加新函数需要5个步骤

**核心设计**: 统一接口 + 运行时绑定 = 硬件抽象

**下一步**: 阅读 [11-helper-functions.md](./11-helper-functions.md) 学习辅助函数

---

**实验任务**: 添加GetVoltage()函数读取电源电压，同时实现Mock和Real版本
