# 案例1: BPM IOC完整项目

> **项目**: 从零开发一个BPM监控IOC
> **时长**: 2周（设计1天 + 开发1周 + 测试部署3天）
> **难度**: ⭐⭐⭐
> **关键技术**: 三层架构、Mock模拟器、交叉编译

## 1. 项目背景

### 需求来源

某加速器实验室需要监控8路束流位置监测器（BPM）的RF信号，要求：

- 实时监控8路RF输入的幅度、相位、品质因子（Q值）
- 支持波形数据采集（每路2048点）
- 支持寄存器读写（配置硬件参数）
- 提供远程访问接口（EPICS Channel Access）
- 更新频率：10Hz（幅度、相位）+ 1Hz（波形）
- 目标硬件：基于Zynq-7000的自研FPGA板

### 技术约束

- 必须使用EPICS框架（实验室标准）
- 硬件驱动库已由FPGA团队提供（libBPMDriver.so）
- 需要支持PC开发（硬件板紧缺）
- 交叉编译环境已配置（arm-linux-gnueabihf）

## 2. 架构设计

### 2.1 整体架构

采用EPICS标准三层架构：

```
┌─────────────────────────────────────────────┐
│         Database Layer (PV定义)             │
│  - BPMmonitor.db (8路 × 多参数)             │
│  - st.cmd (启动脚本)                        │
└─────────────────────────────────────────────┘
                    ↕ (DSET)
┌─────────────────────────────────────────────┐
│     Device Support Layer (设备支持)         │
│  - devBPMMonitor.c                          │
│  - 处理Record初始化和读写                    │
└─────────────────────────────────────────────┘
                    ↕ (Function Calls)
┌─────────────────────────────────────────────┐
│        Driver Layer (驱动封装)              │
│  - driverWrapper.c                          │
│  - dlopen加载动态库                         │
│  - Mock/Real硬件切换                        │
└─────────────────────────────────────────────┘
                    ↕ (dlsym)
┌─────────────────────────────────────────────┐
│      Hardware Library (硬件库)              │
│  - libBPMDriver.so (真实硬件)               │
│  - libMockDriver.so (PC模拟)                │
└─────────────────────────────────────────────┘
```

### 2.2 设计决策

#### 决策1: 使用dlopen动态加载

**原因**：
- PC开发时加载Mock库，真实部署时加载硬件库
- 不修改代码即可切换环境
- 降低对硬件的依赖

**实现**：
```c
// driverWrapper.c
void* lib_handle = dlopen(lib_path, RTLD_LAZY);
if (!lib_handle) {
    // Fallback to mock
    lib_handle = dlopen("./libMockDriver.so", RTLD_LAZY);
}

// 获取函数指针
BPM_RFIn_ReadADC = dlsym(lib_handle, "BPM_RFIn_ReadADC");
```

#### 决策2: Offset抽象层

**原因**：
- 硬件提供14个数据类型（幅度、相位、Q值等）
- 避免在设备支持层重复代码

**实现**：
```c
// 统一的读取接口
float ReadData(int offset, int channel, int type) {
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        return 0.0;
    }
    return g_data_buffer[offset][channel];
}

// Database中使用INP字段传递offset
field(INP, "@0 0")  # offset=0 (幅度), channel=0
field(INP, "@2 1")  # offset=2 (相位), channel=1
```

#### 决策3: 线程模型

**原因**：
- 硬件采集需要独立线程
- 避免阻塞EPICS主线程

**实现**：
```c
void* AcquireThread(void* arg) {
    while (g_running) {
        // 采集所有通道数据
        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            for (int off = 0; off < NUM_OFFSETS; off++) {
                g_data_buffer[off][ch] = BPM_RFIn_ReadADC(ch, off);
            }
        }
        usleep(100000);  // 100ms → 10Hz
    }
}
```

## 3. 实现过程

### 3.1 第1天: 项目初始化

#### 创建EPICS应用

```bash
cd /opt/epics/modules
makeBaseApp.pl -t ioc BPMmonitor
makeBaseApp.pl -i -t ioc BPMmonitor

# 项目结构
BPMmonitor/
├── BPMmonitorApp/
│   ├── src/           # 源代码
│   └── Db/            # 数据库文件
├── iocBoot/
│   └── iocBPMmonitor/ # 启动脚本
└── configure/         # 构建配置
```

#### 编写Mock库

为了PC开发，先创建Mock库：

```c
// simulator/mockDriver.c
#include <math.h>

float BPM_RFIn_ReadADC(int channel, int type) {
    // 模拟数据
    static int counter = 0;
    counter++;

    switch (type) {
    case 0:  // 幅度
        return 10.0 + 2.0 * sin(counter * 0.01 + channel);
    case 2:  // 相位
        return 45.0 + 10.0 * cos(counter * 0.01);
    default:
        return 0.0;
    }
}

void BPM_RFIn_SetReg(unsigned int addr, unsigned int value) {
    printf("[Mock] SetReg: addr=0x%x, value=0x%x\n", addr, value);
}
```

编译：
```bash
cd simulator
gcc -shared -fPIC -o libMockDriver.so mockDriver.c -lm
```

### 3.2 第2-5天: 驱动层开发

#### driverWrapper.c 核心实现

```c
#include <dlfcn.h>
#include <pthread.h>

// 函数指针
static float (*BPM_RFIn_ReadADC)(int, int) = NULL;
static void (*BPM_RFIn_SetReg)(unsigned int, unsigned int) = NULL;

// 全局缓冲区
static float g_data_buffer[NUM_OFFSETS][MAX_RF_CHANNELS];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_running = 0;

// 初始化函数
int InitDevice(const char* lib_path) {
    // 1. 加载动态库
    void* lib_handle = dlopen(lib_path, RTLD_LAZY);
    if (!lib_handle) {
        printf("WARNING: Cannot load %s, using mock\n", lib_path);
        lib_handle = dlopen("./libMockDriver.so", RTLD_LAZY);
    }

    // 2. 获取函数指针
    BPM_RFIn_ReadADC = dlsym(lib_handle, "BPM_RFIn_ReadADC");
    BPM_RFIn_SetReg = dlsym(lib_handle, "BPM_RFIn_SetReg");

    if (!BPM_RFIn_ReadADC) {
        printf("ERROR: Cannot find BPM_RFIn_ReadADC\n");
        return -1;
    }

    // 3. 启动采集线程
    g_running = 1;
    pthread_t tid;
    pthread_create(&tid, NULL, AcquireThread, NULL);

    printf("Device initialized successfully\n");
    return 0;
}

// 采集线程
void* AcquireThread(void* arg) {
    while (g_running) {
        pthread_mutex_lock(&g_mutex);

        for (int ch = 0; ch < MAX_RF_CHANNELS; ch++) {
            for (int off = 0; off < NUM_OFFSETS; off++) {
                g_data_buffer[off][ch] = BPM_RFIn_ReadADC(ch, off);
            }
        }

        pthread_mutex_unlock(&g_mutex);
        usleep(100000);  // 10Hz
    }
    return NULL;
}

// 读取数据
float ReadData(int offset, int channel, int type) {
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        return 0.0;
    }

    pthread_mutex_lock(&g_mutex);
    float value = g_data_buffer[offset][channel];
    pthread_mutex_unlock(&g_mutex);

    return value;
}

// 寄存器写入
void SetReg(unsigned int addr, unsigned int value) {
    if (BPM_RFIn_SetReg) {
        BPM_RFIn_SetReg(addr, value);
    }
}
```

### 3.3 第6-7天: 设备支持层

#### devBPMMonitor.c 实现

```c
#include <aiRecord.h>
#include <aoRecord.h>
#include <devSup.h>

// 私有数据结构
typedef struct {
    int offset;
    int channel;
    int type;
} DevPvt;

// ai record初始化
static long init_ai_record(aiRecord *prec) {
    // 解析INP字段: "@offset channel"
    int offset, channel;
    if (sscanf(prec->inp.value.instio.string, "@%d %d",
               &offset, &channel) != 2) {
        printf("ERROR: Invalid INP format\n");
        return -1;
    }

    // 创建私有数据
    DevPvt *pPvt = malloc(sizeof(DevPvt));
    pPvt->offset = offset;
    pPvt->channel = channel;
    pPvt->type = 0;
    prec->dpvt = pPvt;

    return 0;
}

// ai record读取
static long read_ai(aiRecord *prec) {
    DevPvt *pPvt = (DevPvt*)prec->dpvt;

    // 从驱动层读取
    float value = ReadData(pPvt->offset, pPvt->channel, pPvt->type);
    prec->val = value;
    prec->udf = 0;

    return 0;
}

// DSET定义
struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devAiBPMMonitor = {
    6,
    NULL,
    NULL,
    init_ai_record,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAiBPMMonitor);
```

### 3.4 第8-9天: 数据库设计

#### BPMmonitor.db

```
# 幅度监控 (offset=0)
record(ai, "LLRF:BPM:RFIn_01_Amp") {
    field(DTYP, "BPM Monitor")
    field(INP, "@0 0")
    field(SCAN, ".1 second")
    field(EGU, "dBm")
    field(PREC, "3")
    field(HIHI, "15.0")
    field(HIGH, "12.0")
    field(LOW, "8.0")
    field(LOLO, "5.0")
    field(HHSV, "MAJOR")
    field(HSV, "MINOR")
    field(LSV, "MINOR")
    field(LLSV, "MAJOR")
}

# 相位监控 (offset=2)
record(ai, "LLRF:BPM:RFIn_01_Phase") {
    field(DTYP, "BPM Monitor")
    field(INP, "@2 0")
    field(SCAN, ".1 second")
    field(EGU, "deg")
    field(PREC, "2")
}

# ... 重复8路 ...

# 寄存器写入
record(ao, "LLRF:BPM:Reg_Write") {
    field(DTYP, "BPM Monitor")
    field(OUT, "@REG")
    field(DRVL, "0")
    field(DRVH, "65535")
}
```

### 3.5 第10天: 测试

#### 单元测试

使用Unity框架：

```c
// test/test_driver.c
#include "unity.h"

void test_InitDevice_success(void) {
    int ret = InitDevice("./libMockDriver.so");
    TEST_ASSERT_EQUAL(0, ret);
}

void test_ReadData_valid_channel(void) {
    InitDevice("./libMockDriver.so");
    float value = ReadData(0, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(20.0, 10.0, value);  // 10±20
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_InitDevice_success);
    RUN_TEST(test_ReadData_valid_channel);
    return UNITY_END();
}
```

#### 集成测试

```python
# test_ioc.py
import epics
import time
import subprocess

# 启动IOC
proc = subprocess.Popen(['./st.cmd'],
                       cwd='../../iocBoot/iocBPMmonitor')
time.sleep(3)

# 测试连接
pv_amp = epics.PV('LLRF:BPM:RFIn_01_Amp')
assert pv_amp.connected, "PV not connected"

# 测试数据范围
value = pv_amp.get()
assert 0 < value < 20, f"Value out of range: {value}"

# 测试更新
old_value = pv_amp.get()
time.sleep(0.5)
new_value = pv_amp.get()
assert old_value != new_value, "PV not updating"

proc.terminate()
print("All tests passed!")
```

### 3.6 第11-12天: 部署

#### 交叉编译

```bash
# configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS = linux-arm

# configure/os/CONFIG_SITE.Common.linux-arm
GNU_TARGET = arm-linux-gnueabihf

# 编译
make clean uninstall
make
```

#### 部署到目标板

```bash
# 1. 复制文件
scp -r bin/linux-arm root@192.168.1.100:/opt/BPMmonitor/
scp -r db root@192.168.1.100:/opt/BPMmonitor/
scp -r iocBoot/iocBPMmonitor root@192.168.1.100:/opt/BPMmonitor/

# 2. 配置st.cmd
ssh root@192.168.1.100
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor
vi st.cmd
# 修改库路径为真实硬件库
# BPM_DRIVER_LIB=/opt/BPMDriver/lib/libBPMDriver.so

# 3. 测试运行
./st.cmd

# 4. 配置systemd服务
cat > /etc/systemd/system/bpmioc.service <<EOF
[Unit]
Description=BPM Monitor IOC

[Service]
Type=simple
WorkingDirectory=/opt/BPMmonitor/iocBoot/iocBPMmonitor
ExecStart=/opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd
Restart=always

[Install]
WantedBy=multi-user.target
EOF

systemctl enable bpmioc
systemctl start bpmioc
```

### 3.7 第13-14天: 文档和交付

#### 项目文档

```markdown
# BPM Monitor IOC

## 概述
监控8路BPM的RF信号

## 安装
1. PC开发: `make` (使用Mock库)
2. 硬件部署: 参见 DEPLOYMENT.md

## PV列表
| PV名称 | 描述 | 单位 |
|--------|------|------|
| LLRF:BPM:RFIn_01_Amp | 通道1幅度 | dBm |
| LLRF:BPM:RFIn_01_Phase | 通道1相位 | deg |
...

## 维护
- 日志位置: /var/log/bpmioc.log
- 重启: `systemctl restart bpmioc`
- 监控: `camonitor LLRF:BPM:*`
```

## 4. 遇到的问题和解决

### 问题1: dlopen失败

**现象**：
```
ERROR: libBPMDriver.so: cannot open shared object file
```

**原因**：
- 动态库搜索路径不包含自定义路径

**解决**：
```bash
# 方法1: 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/BPMDriver/lib:$LD_LIBRARY_PATH

# 方法2: 使用绝对路径
dlopen("/opt/BPMDriver/lib/libBPMDriver.so", RTLD_LAZY);

# 方法3: 配置ldconfig
echo "/opt/BPMDriver/lib" > /etc/ld.so.conf.d/bpm.conf
ldconfig
```

### 问题2: 线程竞争导致数据错乱

**现象**：
- PV值偶尔出现异常跳变
- 不同PV值相同

**原因**：
- 采集线程和EPICS读取线程同时访问`g_data_buffer`
- 没有正确加锁

**解决**：
```c
// 读取时加锁
float ReadData(int offset, int channel, int type) {
    pthread_mutex_lock(&g_mutex);
    float value = g_data_buffer[offset][channel];
    pthread_mutex_unlock(&g_mutex);
    return value;
}

// 采集时加锁
void* AcquireThread(void* arg) {
    while (g_running) {
        pthread_mutex_lock(&g_mutex);
        // ... 采集数据 ...
        pthread_mutex_unlock(&g_mutex);
        usleep(100000);
    }
}
```

### 问题3: 交叉编译的pthread库问题

**现象**：
```
undefined reference to `pthread_create'
```

**原因**：
- Makefile未正确链接pthread库

**解决**：
```makefile
# BPMmonitorApp/src/Makefile
BPMmonitor_SYS_LIBS += pthread
BPMmonitor_SYS_LIBS += dl
```

## 5. 经验教训

### ✅ 做得好的

1. **Mock库设计**
   - PC开发效率高，无需等待硬件
   - Mock数据模拟真实场景（正弦波、噪声）

2. **Offset抽象**
   - 设备支持层代码简洁
   - 易于扩展新的数据类型

3. **测试覆盖**
   - 单元测试 + 集成测试
   - 发现了3个边界条件bug

### ❌ 可以改进的

1. **错误处理不完善**
   - dlopen失败时应有更好的fallback
   - 应该记录日志而不是printf

2. **性能未优化**
   - 每次读取都加锁，开销大
   - 可以使用Ring Buffer减少锁竞争

3. **文档不够详细**
   - 缺少架构图和数据流图
   - 缺少常见问题FAQ

### 💡 如果重做

1. **使用配置文件**
   ```bash
   # config/bpm.conf
   DRIVER_LIB=/opt/BPMDriver/lib/libBPMDriver.so
   NUM_CHANNELS=8
   UPDATE_RATE=10
   ```

2. **添加性能监控PV**
   ```
   record(ai, "LLRF:BPM:Stats_UpdateRate") {
       field(DESC, "实际更新率")
       field(EGU, "Hz")
   }
   ```

3. **使用errlog替代printf**
   ```c
   #include <errlog.h>
   errlogPrintf("Device initialized\n");
   errlogSevPrintf(errlogFatal, "Cannot load driver\n");
   ```

## 6. 项目成果

### 交付物

- ✅ 源代码（~2000行C代码）
- ✅ 数据库文件（64个PV）
- ✅ 测试用例（单元测试 + 集成测试）
- ✅ 部署脚本和systemd服务
- ✅ 用户文档和维护手册

### 性能指标

| 指标 | 要求 | 实际 | 状态 |
|------|------|------|------|
| 更新频率 | 10Hz | 10Hz | ✅ |
| 通道数 | 8 | 8 | ✅ |
| CPU占用 | <10% | ~5% | ✅ |
| 内存占用 | <50MB | ~30MB | ✅ |
| 启动时间 | <5s | ~3s | ✅ |

### 用户反馈

- ✅ 满足监控需求
- ✅ PC开发方便
- ⚠️ 希望增加历史数据归档
- ⚠️ 希望增加Web界面

## 7. 后续迭代

### v1.1 (1周后)
- 添加Archiver集成
- 增加SNR计算

### v1.2 (1个月后)
- 添加Web监控界面（使用EPICS Web Tools）
- 增加报警邮件通知

### v2.0 (3个月后)
- 支持16路通道
- 优化性能（Ring Buffer、无锁编程）

## 🔗 相关资源

- [Part 3: 架构设计](../part3-bpmioc-architecture/)
- [Part 4-6: 三层实现](../part4-driver-layer/)
- [Part 19: Mock库编写](../part19-simulator-tutorial/)
- [Part 13: 部署指南](../part13-deployment/)
