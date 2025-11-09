# 快速参考 (Quick Reference)

常用命令、代码模板和配置的速查手册。

## 目录
- [环境变量](#环境变量)
- [EPICS命令](#epics命令)
- [CA工具](#ca工具)
- [IOC Shell命令](#ioc-shell命令)
- [代码模板](#代码模板)
- [Makefile模板](#makefile模板)
- [数据库模板](#数据库模板)
- [调试命令](#调试命令)

---

## 环境变量

### 必需的环境变量

```bash
# EPICS Base路径
export EPICS_BASE=/opt/epics/base

# 主机架构
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)

# PATH
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 库路径（可选）
export LD_LIBRARY_PATH=${EPICS_BASE}/lib/${EPICS_HOST_ARCH}:${LD_LIBRARY_PATH}
```

### CA配置变量

```bash
# CA服务器地址列表（手动指定IOC）
export EPICS_CA_ADDR_LIST="192.168.1.100 192.168.1.101"
export EPICS_CA_AUTO_ADDR_LIST=NO

# CA超时（秒）
export EPICS_CA_CONN_TMO=30.0

# CA最大数组大小
export EPICS_CA_MAX_ARRAY_BYTES=16384

# CA网络接口
export EPICS_CA_SERVER_PORT=5064
export EPICS_CA_REPEATER_PORT=5065
```

### 快速设置脚本

```bash
# 保存为 setup_epics.sh
#!/bin/bash
export EPICS_BASE=/opt/epics/base
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}
export LD_LIBRARY_PATH=${EPICS_BASE}/lib/${EPICS_HOST_ARCH}:${LD_LIBRARY_PATH}

echo "EPICS environment configured:"
echo "  EPICS_BASE=$EPICS_BASE"
echo "  EPICS_HOST_ARCH=$EPICS_HOST_ARCH"

# 使用:
# source setup_epics.sh
```

---

## EPICS命令

### 创建新应用

```bash
# 创建IOC应用
cd /path/to/workspace
makeBaseApp.pl -t ioc MyIOC
makeBaseApp.pl -i -t ioc MyIOC

# 编译
cd MyIOC
make
```

### 编译命令

```bash
# 编译
make

# 清理
make clean

# 完全清理（包括安装文件）
make clean uninstall

# 仅编译特定架构
make EPICS_HOST_ARCH=linux-x86_64
```

---

## CA工具

### caget - 读取PV

```bash
# 基本用法
caget PV_NAME

# 多个PV
caget PV1 PV2 PV3

# 显示时间戳
caget -t PV_NAME

# 以特定格式显示
caget -f 4 PV_NAME          # 4位小数
caget -e PV_NAME            # 科学计数法

# 数组PV
caget -# 100 WAVEFORM_PV    # 显示100个元素

# 超时设置
caget -w 5.0 PV_NAME        # 5秒超时
```

### caput - 写入PV

```bash
# 基本用法
caput PV_NAME VALUE

# 字符串PV
caput STRING_PV "Hello World"

# 等待callback完成
caput -c PV_NAME VALUE

# 数组PV
caput WAVEFORM_PV 10 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0
```

### camonitor - 监控PV

```bash
# 基本用法
camonitor PV_NAME

# 多个PV
camonitor PV1 PV2 PV3

# 通配符
camonitor "LLRF:BPM:*:Amp"

# 显示时间戳
camonitor -t PV_NAME

# 计数（监控N次后退出）
camonitor -c 100 PV_NAME

# 输出到文件
camonitor PV_NAME > data.txt
```

### cainfo - PV信息

```bash
# 查看PV详细信息
cainfo PV_NAME

# 输出示例:
# LLRF:BPM:RFIn_01_Amp
#     State:            connected
#     Host:             192.168.1.100:5064
#     Data type:        DBR_DOUBLE
#     Element count:    1
#     ...
```

---

## IOC Shell命令

### 启动IOC后可用的命令

```bash
# 列出所有PV
dbl

# 按record类型过滤
dbl("ai")

# 按pattern过滤
dbl("LLRF:BPM:*")

# 打印record信息
dbpr "PV_NAME"
dbpr "PV_NAME", 4   # 详细级别0-4

# 修改PV字段
dbpf "PV_NAME.SCAN", "1 second"
dbpf "PV_NAME.VAL", "10.5"

# 查看record类型信息
dbgrep "SCAN"       # 查找包含SCAN的字段

# 列出所有record类型
dbtypepf

# IOC信息
epicsEnvShow        # 显示所有环境变量
pwd                 # 当前目录
cd "path"           # 切换目录

# 启用调试
var recGblInpDebug 1   # 输入调试
var recGblOutDebug 1   # 输出调试
```

---

## 代码模板

### AI Record设备支持

```c
// devMyDevice.c
#include <aiRecord.h>
#include <devSup.h>
#include <epicsExport.h>

// 私有数据结构
typedef struct {
    int channel;
    int offset;
} MyDevPvt;

// 初始化函数
static long init_ai_record(aiRecord *prec) {
    // 解析INP字段
    int channel, offset;
    if (sscanf(prec->inp.value.instio.string, "@%d %d",
               &offset, &channel) != 2) {
        printf("ERROR: Invalid INP format for %s\n", prec->name);
        return -1;
    }

    // 分配私有数据
    MyDevPvt *pPvt = (MyDevPvt*)malloc(sizeof(MyDevPvt));
    pPvt->channel = channel;
    pPvt->offset = offset;
    prec->dpvt = pPvt;

    return 0;
}

// 读取函数
static long read_ai(aiRecord *prec) {
    MyDevPvt *pPvt = (MyDevPvt*)prec->dpvt;

    // 从驱动读取数据
    float value = ReadDataFromDriver(pPvt->offset, pPvt->channel);

    // 更新record
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
} devAiMyDevice = {
    6,
    NULL,
    NULL,
    init_ai_record,
    NULL,
    read_ai,
    NULL
};
epicsExportAddress(dset, devAiMyDevice);
```

### AO Record设备支持

```c
// devMyDeviceAo.c
#include <aoRecord.h>
#include <devSup.h>
#include <epicsExport.h>

static long init_ao_record(aoRecord *prec) {
    // ... 类似ai
    return 0;
}

static long write_ao(aoRecord *prec) {
    MyDevPvt *pPvt = (MyDevPvt*)prec->dpvt;

    // 写入驱动
    int ret = WriteDataToDriver(pPvt->offset, pPvt->channel, prec->val);

    if (ret != 0) {
        recGblSetSevr(prec, WRITE_ALARM, MAJOR_ALARM);
        return -1;
    }

    prec->udf = 0;
    return 0;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write;
    DEVSUPFUN special_linconv;
} devAoMyDevice = {
    6,
    NULL,
    NULL,
    init_ao_record,
    NULL,
    write_ao,
    NULL
};
epicsExportAddress(dset, devAoMyDevice);
```

### 驱动初始化函数

```c
// driver.c
#include <dlfcn.h>
#include <epicsExport.h>

static void *g_lib_handle = NULL;
static float (*ReadADC)(int, int) = NULL;

int InitDriver(const char *lib_path) {
    // 加载动态库
    g_lib_handle = dlopen(lib_path, RTLD_LAZY);
    if (!g_lib_handle) {
        printf("ERROR: Cannot load %s: %s\n", lib_path, dlerror());
        return -1;
    }

    // 获取函数指针
    ReadADC = (float (*)(int, int))dlsym(g_lib_handle, "ReadADC");
    if (!ReadADC) {
        printf("ERROR: Cannot find ReadADC: %s\n", dlerror());
        dlclose(g_lib_handle);
        return -1;
    }

    printf("Driver initialized successfully\n");
    return 0;
}

// 导出给IOC Shell
static const iocshArg initArg0 = {"lib_path", iocshArgString};
static const iocshArg *initArgs[] = {&initArg0};
static const iocshFuncDef initFuncDef = {"InitDriver", 1, initArgs};

static void initCallFunc(const iocshArgBuf *args) {
    InitDriver(args[0].sval);
}

static void driverRegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
}
epicsExportRegistrar(driverRegister);
```

---

## Makefile模板

### src/Makefile

```makefile
TOP=../..

include $(TOP)/configure/CONFIG

# 应用名称
PROD_IOC = MyIOC

# DBD文件
DBD += MyIOC.dbd

# 源文件
MyIOC_SRCS += MyIOC_registerRecordDeviceDriver.cpp
MyIOC_SRCS += driver.c
MyIOC_SRCS += devMyDevice.c

# 主函数
MyIOC_SRCS_DEFAULT += MyIOCMain.cpp
MyIOC_SRCS_vxWorks += -nil-

# 链接库
MyIOC_LIBS += $(EPICS_BASE_IOC_LIBS)
MyIOC_SYS_LIBS += pthread
MyIOC_SYS_LIBS += dl
MyIOC_SYS_LIBS += m

include $(TOP)/configure/RULES
```

### Db/Makefile

```makefile
TOP=../..
include $(TOP)/configure/CONFIG

# 数据库文件
DB += MyIOC.db
DB += autosave.req

include $(TOP)/configure/RULES
```

---

## 数据库模板

### AI Record模板

```
record(ai, "$(P)$(R)Amplitude") {
    field(DESC, "RF Amplitude")
    field(DTYP, "My Device")
    field(INP,  "@$(OFFSET) $(CHANNEL)")
    field(SCAN, "$(SCAN=.1 second)")
    field(EGU,  "dBm")
    field(PREC, "3")
    field(HOPR, "30")
    field(LOPR, "0")
    field(HIHI, "25")
    field(HIGH, "20")
    field(LOW,  "5")
    field(LOLO, "2")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
}
```

### AO Record模板

```
record(ao, "$(P)$(R)SetVoltage") {
    field(DESC, "Set Voltage")
    field(DTYP, "My Device")
    field(OUT,  "@$(OFFSET) $(CHANNEL)")
    field(EGU,  "V")
    field(PREC, "2")
    field(DRVH, "30")
    field(DRVL, "0")
    field(HOPR, "30")
    field(LOPR, "0")
}
```

### Calc Record模板

```
record(calc, "$(P)$(R)SNR") {
    field(DESC, "Signal-to-Noise Ratio")
    field(INPA, "$(P)$(R)Signal CPP")
    field(INPB, "$(P)$(R)Noise CPP")
    field(CALC, "20*LOG(A/B)")
    field(EGU,  "dB")
    field(PREC, "2")
}
```

### Waveform Record模板

```
record(waveform, "$(P)$(R)Waveform") {
    field(DESC, "Waveform Data")
    field(DTYP, "My Device")
    field(INP,  "@$(OFFSET) $(CHANNEL)")
    field(SCAN, "$(SCAN=1 second)")
    field(NELM, "2048")
    field(FTVL, "FLOAT")
}
```

---

## 调试命令

### GDB调试

```bash
# 启动GDB
gdb ../../bin/linux-x86_64/MyIOC

# GDB命令
(gdb) run st.cmd                # 运行
(gdb) break main                # 断点在main
(gdb) break driver.c:100        # 断点在特定行
(gdb) break InitDriver          # 断点在函数

(gdb) continue                  # 继续运行
(gdb) step                      # 单步进入
(gdb) next                      # 单步跳过
(gdb) finish                    # 运行到函数返回

(gdb) print variable            # 打印变量
(gdb) print *pointer            # 解引用指针
(gdb) print array[5]            # 数组元素

(gdb) backtrace                 # 调用栈
(gdb) frame 2                   # 切换到frame 2
(gdb) info locals               # 局部变量
(gdb) info threads              # 所有线程

(gdb) watch variable            # 监视点
(gdb) condition 1 i==10         # 条件断点

(gdb) quit                      # 退出
```

### Valgrind内存检测

```bash
# 基本用法
valgrind ../../bin/linux-x86_64/MyIOC st.cmd

# 详细检测
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes \
         ../../bin/linux-x86_64/MyIOC st.cmd

# 输出到文件
valgrind --log-file=valgrind.log \
         ../../bin/linux-x86_64/MyIOC st.cmd
```

### 性能分析

```bash
# perf record
perf record -g -p $(pidof MyIOC) sleep 10

# perf report
perf report

# perf top (实时)
perf top -p $(pidof MyIOC)
```

### 网络调试

```bash
# 抓包CA协议
sudo tcpdump -i eth0 port 5064 or port 5065 -w ca.pcap

# 查看CA连接
netstat -an | grep 5064

# Wireshark过滤器
ca || epics
```

---

## 有用的脚本

### 批量caget

```bash
#!/bin/bash
# 批量读取PV并保存
PV_LIST=(
    "LLRF:BPM:RFIn_01_Amp"
    "LLRF:BPM:RFIn_01_Phase"
    "LLRF:BPM:RFIn_02_Amp"
    "LLRF:BPM:RFIn_02_Phase"
)

for pv in "${PV_LIST[@]}"; do
    echo -n "$pv: "
    caget -t $pv
done
```

### PV监控脚本

```python
#!/usr/bin/env python3
import epics
import time

pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

print("Monitoring PV:")
while True:
    try:
        value = pv.get()
        timestamp = pv.timestamp
        print(f"{time.ctime(timestamp)}: {value:.3f}")
        time.sleep(1)
    except KeyboardInterrupt:
        break
```

---

**使用提示**:
- 收藏此页面以便快速查阅
- 复制代码模板时注意修改名称
- 环境变量建议加入`.bashrc`
- GDB命令可以缩写（如`b`代替`break`）
