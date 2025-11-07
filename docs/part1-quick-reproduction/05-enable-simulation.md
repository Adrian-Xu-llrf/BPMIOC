# 启用模拟模式

> **预计时间**: 10-15分钟
> **难度**: ⭐⭐⭐☆☆
> **前置条件**: 已完成 04-clone-and-compile.md

## 为什么需要模拟模式?

BPMIOC 原本设计为与真实的 FPGA 硬件通信,通过 `liblowlevel.so` 库访问硬件。在学习和开发阶段:

- ❌ 没有 FPGA 硬件
- ❌ 没有 liblowlevel.so 库
- ❌ IOC 会因为加载库失败而无法启动

**解决方案**: 修改代码,检测到库加载失败时自动切换到**模拟模式**,生成模拟数据。

## 修改策略

修改 `driverWrapper.c`,在以下位置添加模拟逻辑:

1. ✅ 添加模拟模式标志
2. ✅ 修改 `InitDevice()` - 检测库加载失败
3. ✅ 修改 `my_thread()` - 生成模拟数据
4. ✅ 添加必要的头文件

## 详细步骤

### 步骤 1: 备份原文件

```bash
cd ~/BPMIOC
cp BPMmonitorApp/src/driverWrapper.c BPMmonitorApp/src/driverWrapper.c.bak
```

### 步骤 2: 编辑 driverWrapper.c

```bash
vim BPMmonitorApp/src/driverWrapper.c
```

### 步骤 3: 添加头文件

在文件顶部的 `#include` 部分添加:

```c
#include <time.h>
#include <math.h>
```

### 步骤 4: 添加模拟模式标志

在全局变量区域(文件开头,函数定义之前)添加:

```c
// 模拟模式标志
static int use_simulation = 0;
```

**位置**: 放在其他 `static` 全局变量附近,例如在 `static double Amp[8];` 前后。

### 步骤 5: 修改 InitDevice() 函数

找到 `int InitDevice()` 函数,修改为:

```c
int InitDevice()
{
    printf("=== BPM Monitor Driver Initialization ===\n");

    // 尝试加载硬件库
    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    if (!handle) {
        // 库加载失败,使用模拟模式
        printf("WARNING: Cannot load liblowlevel.so: %s\n", dlerror());
        printf("WARNING: Using SIMULATION mode\n");
        printf("         All data will be simulated\n");

        use_simulation = 1;

        // 初始化 I/O 中断
        scanIoInit(&ioScanPvt);

        // 创建后台扫描线程
        epicsThreadCreate("BPMMonitor",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)my_thread,
                          NULL);

        return 0;  // 成功(模拟模式)
    }

    // 以下是原有的真实硬件初始化代码
    printf("INFO: Loaded liblowlevel.so successfully\n");
    printf("INFO: Using HARDWARE mode\n");

    // 获取函数指针
    *(void **)(&systemInitFunc) = dlsym(handle, "SystemInit");
    *(void **)(&getRfInfoFunc) = dlsym(handle, "GetRfInfo");
    // ... (保留原有的所有 dlsym 调用)

    // 初始化硬件
    (*systemInitFunc)();

    // 加载校准参数
    Amp2PowerInit();
    LoadParam();

    // 创建扫描线程
    scanIoInit(&ioScanPvt);
    epicsThreadCreate("BPMMonitor",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC)my_thread,
                      NULL);

    return 0;
}
```

### 步骤 6: 修改 my_thread() 函数

找到 `static void my_thread(void *arg)` 函数,修改为:

```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;  // 模拟时间
    struct timespec ts;

    printf("Background thread started\n");

    while (1) {
        if (use_simulation) {
            // ========== 模拟模式 ==========

            // 1. 模拟 RF 振幅和相位
            for (int i = 0; i < 8; i++) {
                // 振幅: 1.0V ± 0.5V 正弦波,不同通道有相位差
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);

                // 相位: 线性增加,加上通道偏移
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }

            // 2. 模拟触发波形
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 10000; j++) {
                    // 正弦波 + 高斯噪声
                    float signal = sin(j * 0.001 + i * 0.1);
                    float noise = ((rand() % 100) - 50) / 1000.0;  // ±0.05
                    TrigWaveform[i][j] = Amp[i] * (signal + noise);
                }
            }

            // 3. 模拟 BPM 位置数据 (两个 BPM)
            // BPM1 (channel 0)
            BPM_X[0] = 0.5 * sin(sim_time * 0.5);       // ±0.5mm
            BPM_Y[0] = 0.3 * cos(sim_time * 0.5);       // ±0.3mm
            BPM_Sum[0] = 100.0 + 10.0 * sin(sim_time);  // 90-110

            // BPM2 (channel 1)
            BPM_X[1] = 0.4 * sin(sim_time * 0.5 + 1.0);
            BPM_Y[1] = 0.35 * cos(sim_time * 0.5 + 1.0);
            BPM_Sum[1] = 100.0 + 10.0 * sin(sim_time + 0.5);

            // 4. 更新时间戳 (使用系统时间)
            clock_gettime(CLOCK_REALTIME, &ts);
            TAIsec = ts.tv_sec;
            TAInsec = ts.tv_nsec;

            // 5. 递增模拟时间
            sim_time += 0.1;  // 每次增加 0.1秒

        } else {
            // ========== 真实硬件模式 ==========

            // 读取 RF 信息
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }

            // 读取 BPM 位置
            BPM_X[0] = (*getBPMXFunc)(0);
            BPM_Y[0] = (*getBPMYFunc)(0);
            BPM_Sum[0] = (*getBPMSumFunc)(0);

            BPM_X[1] = (*getBPMXFunc)(1);
            BPM_Y[1] = (*getBPMYFunc)(1);
            BPM_Sum[1] = (*getBPMSumFunc)(1);

            // 读取时间戳
            (*getTimeStampFunc)(&TAIsec, &TAInsec);

            // 读取波形数据 (如果需要)
            // ... (保留原有代码)
        }

        // 触发 I/O 中断扫描 (模拟和真实模式都需要)
        scanIoRequest(ioScanPvt);

        // 休眠 100ms
        epicsThreadSleep(0.1);
    }
}
```

### 步骤 7: 保存文件

保存并退出编辑器。

### 步骤 8: 验证修改

```bash
# 检查语法 (不编译,只检查)
gcc -c -I${EPICS_BASE}/include \
    -I${EPICS_BASE}/include/os/Linux \
    BPMmonitorApp/src/driverWrapper.c \
    -o /tmp/test.o

# 如果有错误,仔细检查修改
```

### 步骤 9: 重新编译

```bash
cd ~/BPMIOC
make clean
make -j$(nproc)
```

**预期输出**:
```
...
make[1]: Leaving directory '/home/yourname/BPMIOC'
```

如果编译成功,你应该看到没有错误。

### 步骤 10: 测试模拟模式

```bash
cd iocBoot/iocBPMmonitor
./st.cmd
```

**预期输出**:
```
=== BPM Monitor Driver Initialization ===
WARNING: Cannot load liblowlevel.so: /usr/lib/liblowlevel.so: cannot open shared object file: No such file or directory
WARNING: Using SIMULATION mode
         All data will be simulated
Background thread started
Starting iocInit
...
iocRun: All initialization complete
epics>
```

关键标志:
- ✅ "Using SIMULATION mode"
- ✅ "Background thread started"
- ✅ "All initialization complete"
- ✅ 看到 `epics>` 提示符

## 验证模拟数据

在 IOC Shell 中:

```bash
# 读取 RF 振幅
epics> dbgf "iLinac_007:BPM14And15:RF3Amp"
DBR_DOUBLE:          1.234567

# 读取 RF 相位
epics> dbgf "iLinac_007:BPM14And15:RF3Phase"
DBR_DOUBLE:          45.678

# 读取多次,观察值的变化
epics> dbgf "iLinac_007:BPM14And15:RF3Amp"
DBR_DOUBLE:          1.345678  # 值在变化!
```

在新终端使用 CA 工具:

```bash
# 监控振幅变化
camonitor iLinac_007:BPM14And15:RF3Amp

# 应该看到值持续变化:
# iLinac_007:BPM14And15:RF3Amp 2025-11-07 10:30:45.123456 1.234567
# iLinac_007:BPM14And15:RF3Amp 2025-11-07 10:30:45.223456 1.245678
# ...
```

## 模拟数据特征

### RF 振幅 (Amp)
- **范围**: 0.5V ~ 1.5V
- **模式**: 正弦波
- **周期**: 约 6.3秒
- **通道差异**: 每个通道有不同的相位偏移

### RF 相位 (Phase)
- **范围**: 0° ~ 360°
- **模式**: 线性增加(循环)
- **速度**: 每秒增加约 10°
- **通道差异**: 每个通道偏移 45°

### BPM 位置
- **X位置**: ±0.5mm (BPM1), ±0.4mm (BPM2)
- **Y位置**: ±0.3mm (BPM1), ±0.35mm (BPM2)
- **Sum值**: 90 ~ 110
- **模式**: 正弦波运动

### 波形数据
- **长度**: 10000点/通道
- **内容**: 正弦波 + 高斯噪声
- **噪声**: ±0.05 (5%)

## 高级: 自定义模拟数据

### 修改振幅范围

在 `my_thread()` 中修改:

```c
// 原来: 0.5 ~ 1.5V
Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);

// 改为: 2.0 ~ 3.0V
Amp[i] = 2.5 + 0.5 * sin(sim_time + i * 0.5);
```

### 添加趋势变化

```c
// 振幅随时间缓慢下降
double trend = 1.0 - 0.0001 * sim_time;  // 每100秒下降1%
Amp[i] = trend * (1.0 + 0.5 * sin(sim_time + i * 0.5));
```

### 模拟告警条件

```c
// 每隔一段时间产生一个高值
if ((int)sim_time % 30 == 0) {  // 每30秒
    Amp[i] = 2.0;  // 超过正常范围,触发告警
}
```

## 故障排查

### 问题 1: 编译错误 "time.h: No such file"

**解决**: 系统缺少标准库头文件
```bash
# Ubuntu
sudo apt install libc6-dev

# CentOS
sudo yum install glibc-devel
```

### 问题 2: 编译错误 "math.h: undefined reference to 'sin'"

**解决**: 需要链接数学库

编辑 `BPMmonitorApp/src/Makefile`,确保有:
```makefile
BPMmonitor_SYS_LIBS += m
```

### 问题 3: IOC 启动但所有值都是 0

**检查**: 后台线程是否启动

在 IOC Shell 中:
```bash
epics> var BPMMonitorDebug 1
```

如果看不到 "Background thread started",检查 `my_thread()` 中的 `printf`。

### 问题 4: 值不变化

**检查**:
1. `scanIoRequest(ioScanPvt)` 是否被调用
2. 记录的 SCAN 字段是否为 "I/O Intr"

```bash
epics> dbpr "iLinac_007:BPM14And15:RF3Amp" 2
# 查找 SCAN 字段,应该是 "I/O Intr"
```

## 恢复原文件

如果需要恢复到原始版本:

```bash
cd ~/BPMIOC
mv BPMmonitorApp/src/driverWrapper.c.bak BPMmonitorApp/src/driverWrapper.c
make clean
make
```

## 下一步

✅ 模拟模式已启用!

现在你可以:
1. **运行 IOC** → [06-first-run.md](./06-first-run.md)
2. **验证 PV** → [07-verify-pvs.md](./07-verify-pvs.md)
3. **开始实验** → [Part 8: 实验室](../part8-hands-on-labs/)

---

**提示**: 保存好修改后的代码,以后可以作为模板使用！
