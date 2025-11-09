# 驱动层修改指南

> **阅读时间**: 25分钟
> **难度**: ⭐⭐⭐⭐☆

## 📋 本文目标

- 掌握安全修改驱动层的方法
- 学会添加新功能的完整流程
- 理解修改对系统的影响

## 1. 修改前准备

### 1.1 备份

```bash
# 备份整个项目
cd ~
tar -czf BPMIOC_backup_$(date +%Y%m%d).tar.gz BPMIOC/

# 或使用git
cd ~/BPMIOC
git add -A
git commit -m "Backup before modification"
git tag backup-$(date +%Y%m%d-%H%M%S)
```

### 1.2 创建测试环境

```bash
# 复制项目
cp -r ~/BPMIOC ~/BPMIOC_test

# 在测试环境工作
cd ~/BPMIOC_test
```

### 1.3 理解现有代码

```bash
# 阅读相关文档
less docs/part4-driver-layer/README.md

# 查看代码结构
wc -l BPMmonitorApp/src/driverWrapper.c

# 查找关键函数
grep -n "^long InitDevice" BPMmonitorApp/src/driverWrapper.c
grep -n "^float ReadData" BPMmonitorApp/src/driverWrapper.c
```

## 2. 常见修改场景

### 2.1 添加新PV（标量）

**目标**: 添加一个温度PV `LLRF:BPM:Temp1`

#### Step 1: 添加硬件函数

```c
// libbpm_mock.c
float GetTemperature(int sensor)
{
    return 25.0 + (rand() % 50) * 0.1;  // 25-30℃
}
```

#### Step 2: 在驱动层声明函数指针

```c
// driverWrapper.c 全局变量区域
static float (*funcGetTemperature)(int sensor);
```

#### Step 3: 在InitDevice()中加载

```c
long InitDevice()
{
    // ... 其他dlsym ...

    funcGetTemperature = (float (*)(int))dlsym(handle, "GetTemperature");
    if (funcGetTemperature == NULL) {
        fprintf(stderr, "WARNING: Cannot find GetTemperature\n");
    }

    // ...
}
```

#### Step 4: 在ReadData()中添加case

```c
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        // ... 现有case 0-28 ...

        case 29:  // 温度
            if (funcGetTemperature != NULL) {
                return funcGetTemperature(channel);
            } else {
                return 25.0;  // 默认值
            }
            break;

        default:
            return 0.0;
    }
}
```

#### Step 5: 添加数据库定义

```
# BPMMonitor.db
record(ai, "LLRF:BPM:Temp1") {
    field(DESC, "Temperature Sensor 1")
    field(DTYP, "BPMMonitor")
    field(INP,  "@29 0")      # offset=29, channel=0
    field(SCAN, "I/O Intr")
    field(EGU,  "degC")
    field(PREC, "1")
    field(HIGH, "35")
    field(HSV,  "MINOR")
    field(HIHI, "40")
    field(HHSV, "MAJOR")
}
```

#### Step 6: 编译测试

```bash
cd ~/BPMIOC_test
make

cd iocBoot/iocBPMmonitor
./st.cmd

# 测试
epics> caget LLRF:BPM:Temp1
```

### 2.2 添加新波形PV

**目标**: 添加一个FFT波形 `LLRF:BPM:RF3FFT`

#### Step 1: 添加全局缓冲区

```c
// driverWrapper.c
static float rf3fft[buf_len];
```

#### Step 2: 初始化缓冲区

```c
static void initAllBuffers(void)
{
    // ... 其他buffer ...

    memset(rf3fft, 0, sizeof(rf3fft));
}
```

#### Step 3: 在pthread中计算FFT

```c
#include <fftw3.h>

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        // 计算RF3的FFT
        computeFFT(rf3amp, rf3fft, buf_len);

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}

void computeFFT(const float *input, float *output, int len)
{
    // FFT计算实现
    // ...
}
```

#### Step 4: 在ReadWaveform()中添加case

```c
long ReadWaveform(int offset, int channel, float *buf, int *len)
{
    switch (offset) {
        // ... 现有case ...

        case 30:  // RF3FFT
            memcpy(buf, rf3fft, buf_len * sizeof(float));
            *len = buf_len;
            break;

        default:
            *len = 0;
            return -1;
    }

    return 0;
}
```

#### Step 5: 添加数据库定义

```
record(waveform, "LLRF:BPM:RF3FFT") {
    field(DESC, "RF3 FFT Spectrum")
    field(DTYP, "BPMMonitor")
    field(INP,  "@30")
    field(SCAN, "I/O Intr")
    field(NELM, "10000")
    field(FTVL, "FLOAT")
    field(EGU,  "V")
}
```

### 2.3 修改采集周期

**目标**: 将100ms改为50ms (20 Hz)

#### Step 1: 修改宏定义

```c
// driverWrapper.c
#define ACQUISITION_PERIOD_US 50000  // 50ms
```

#### Step 2: 修改pthread

```c
void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        usleep(ACQUISITION_PERIOD_US);
    }
}
```

#### Step 3: 考虑性能影响

```
原来: 10 Hz × 50 PV = 500次ReadData/s
现在: 20 Hz × 50 PV = 1000次ReadData/s

网络带宽影响:
原来: 50 PV × 4 bytes × 10 Hz = 2 KB/s
现在: 50 PV × 4 bytes × 20 Hz = 4 KB/s
```

#### Step 4: 测试验证

```c
// 添加统计
static unsigned long scan_count = 0;
static time_t last_report = 0;

void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        scan_count++;

        time_t now = time(NULL);
        if (now - last_report >= 10) {
            printf("Actual scan rate: %.1f Hz\n", scan_count / 10.0);
            scan_count = 0;
            last_report = now;
        }

        usleep(ACQUISITION_PERIOD_US);
    }
}
```

### 2.4 添加数据处理功能

**目标**: 添加数据平滑滤波

#### Step 1: 添加辅助函数

```c
static void smoothData(float *data, int len, int window_size)
{
    float *temp = (float *)malloc(len * sizeof(float));
    memcpy(temp, data, len * sizeof(float));

    int half_window = window_size / 2;

    for (int i = half_window; i < len - half_window; i++) {
        float sum = 0.0;
        for (int j = -half_window; j <= half_window; j++) {
            sum += temp[i + j];
        }
        data[i] = sum / window_size;
    }

    free(temp);
}
```

#### Step 2: 在pthread中应用

```c
void *pthread(void *arg)
{
    while (1) {
        funcTriggerAllDataReached();

        // 平滑处理
        if (Reg[50] == 1) {  // Reg[50]: 启用平滑
            int window = Reg[51];  // Reg[51]: 窗口大小
            smoothData(rf3amp, buf_len, window);
            smoothData(rf4amp, buf_len, window);
        }

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

## 3. 修改注意事项

### 3.1 保持向后兼容

```c
// ✅ 好的做法：添加新功能但保留旧接口
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        case 0:  // 现有功能，保持不变
            return funcGetRFInfo(channel, type);

        case 29:  // 新功能
            return funcGetTemperature(channel);

        default:
            return 0.0;
    }
}

// ❌ 不好的做法：修改现有行为
float ReadData(int offset, int channel, int type)
{
    switch (offset) {
        case 0:  // 修改了现有功能！
            return funcGetRFInfo(channel, type) * 2.0;  // 破坏兼容性
        // ...
    }
}
```

### 3.2 避免破坏现有功能

```c
// 测试现有功能
epics> caget LLRF:BPM:RF3Amp
LLRF:BPM:RF3Amp 105.234  # 确认现有PV仍然工作

# 测试新功能
epics> caget LLRF:BPM:Temp1
LLRF:BPM:Temp1 27.5  # 确认新PV工作
```

### 3.3 添加错误处理

```c
float ReadData(int offset, int channel, int type)
{
    // 参数验证
    if (offset < 0 || offset > 29) {
        fprintf(stderr, "ERROR: Invalid offset: %d\n", offset);
        return 0.0;
    }

    // 函数指针检查
    if (offset == 29 && funcGetTemperature == NULL) {
        fprintf(stderr, "WARNING: Temperature function not available\n");
        return 25.0;  // 返回合理默认值
    }

    // ...
}
```

## 4. 修改后验证

### 4.1 功能测试

```bash
# 测试所有现有PV
for pv in RF3Amp RF3Phase X1 Y1; do
    echo "Testing $pv:"
    caget LLRF:BPM:$pv
done

# 测试新PV
caget LLRF:BPM:Temp1
```

### 4.2 性能测试

```bash
# CPU使用率
top -p $(pidof st.cmd)

# 内存使用
ps aux | grep st.cmd

# 网络流量
iftop -i eth0
```

### 4.3 长时间稳定性测试

```bash
# 运行24小时监控
while true; do
    date >> stability.log
    caget LLRF:BPM:RF3Amp >> stability.log
    caget LLRF:BPM:Temp1 >> stability.log
    sleep 60
done

# 检查日志
less stability.log
```

## 5. 提交修改

### 5.1 代码审查清单

```
☐ 代码格式一致
☐ 添加了注释
☐ 没有warning
☐ 通过所有测试
☐ 更新了文档
☐ 向后兼容
```

### 5.2 Git提交

```bash
cd ~/BPMIOC
git add BPMmonitorApp/src/driverWrapper.c
git add BPMmonitorApp/Db/BPMMonitor.db
git commit -m "Add temperature monitoring feature

- Add GetTemperature hardware function
- Add offset=29 for temperature reading
- Add LLRF:BPM:Temp1-4 PV records
- Tested on PC with Mock library"

git push
```

## 6. 回滚修改

```bash
# 方法1: Git回滚
git log --oneline
git revert <commit-hash>

# 方法2: 恢复备份
cd ~
tar -xzf BPMIOC_backup_20250109.tar.gz
cd BPMIOC
make clean
make
```

## 📚 延伸阅读

- [15-best-practices.md](./15-best-practices.md) - 最佳实践
- Git使用指南

## 🎓 本章总结

- ✅ 修改前做好备份和测试
- ✅ 遵循一致的代码风格
- ✅ 保持向后兼容性
- ✅ 充分测试验证
- ✅ 及时提交代码

---

**建议**: 每次修改只做一件事，便于测试和回滚
