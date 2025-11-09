# 常见问题 (FAQ)

按主题分类的常见问题和解决方案。

## 目录
- [安装和编译](#安装和编译)
- [运行时错误](#运行时错误)
- [Channel Access](#channel-access)
- [数据库和Record](#数据库和record)
- [设备支持和驱动](#设备支持和驱动)
- [性能优化](#性能优化)
- [调试技巧](#调试技巧)
- [交叉编译和部署](#交叉编译和部署)
- [最佳实践](#最佳实践)

---

## 安装和编译

### Q: EPICS Base编译失败，提示"readline.h not found"？

**A**: 缺少readline开发库。

```bash
# Debian/Ubuntu
sudo apt-get install libreadline-dev

# CentOS/RHEL
sudo yum install readline-devel

# 重新编译
cd $EPICS_BASE
make clean uninstall
make
```

### Q: 编译时出现"undefined reference to `pthread_create`"？

**A**: 未链接pthread库。

**解决**: 在Makefile中添加：

```makefile
# YourApp/src/Makefile
YourApp_SYS_LIBS += pthread
```

### Q: 找不到EPICS Base？

**A**: 环境变量未设置。

```bash
# 添加到 ~/.bashrc
export EPICS_BASE=/opt/epics/base
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 重新加载
source ~/.bashrc
```

### Q: makeBaseApp.pl命令找不到？

**A**: 路径问题。

```bash
# 检查路径
which makeBaseApp.pl

# 应该返回类似:
# /opt/epics/base/bin/linux-x86_64/makeBaseApp.pl

# 如果没找到，手动指定完整路径
$EPICS_BASE/bin/$EPICS_HOST_ARCH/makeBaseApp.pl
```

### Q: 交叉编译时找不到arm-linux-gnueabihf-gcc？

**A**: 交叉编译工具链未安装。

```bash
# Ubuntu
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# 验证
arm-linux-gnueabihf-gcc --version
```

---

## 运行时错误

### Q: IOC启动后立即退出？

**A**: 可能原因：

**1. st.cmd权限问题**
```bash
chmod +x st.cmd
```

**2. 路径错误**
```bash
# 检查st.cmd中的路径
cd iocBoot/iocYourIOC
cat st.cmd

# 确保相对路径正确
cd "../../"
dbLoadDatabase("dbd/YourApp.dbd")
```

**3. 库文件缺失**
```bash
# 检查依赖
ldd ../../bin/linux-x86_64/YourApp

# 如果有"not found"，设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/lib:$LD_LIBRARY_PATH
```

### Q: 出现"dlopen: cannot open shared object file"？

**A**: 动态库加载失败。

**检查步骤**:

```bash
# 1. 确认库文件存在
ls -l /opt/lib/libYourDriver.so

# 2. 检查LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH

# 3. 使用绝对路径
dlopen("/opt/lib/libYourDriver.so", RTLD_LAZY);

# 4. 或配置ldconfig
echo "/opt/lib" | sudo tee /etc/ld.so.conf.d/yourapp.conf
sudo ldconfig
```

### Q: PV值一直显示UDF (Undefined)？

**A**: Record未正确处理。

**可能原因**:

**1. SCAN设置为Passive，但没有触发**
```
# 改为定期扫描
field(SCAN, "1 second")

# 或者确保有FLNK触发
```

**2. 设备支持read函数失败**
```c
// 检查返回值
static long read_ai(aiRecord *prec) {
    // ...
    if (error) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;  // ← 导致UDF
    }
    prec->val = value;
    prec->udf = 0;  // ← 必须清除UDF
    return 0;
}
```

**3. PINI未设置**
```
# IOC启动时处理
field(PINI, "YES")
```

### Q: 出现"Segmentation fault"？

**A**: 内存访问错误，常见原因：

**1. 数组越界**
```c
// 错误
float data[10];
data[10] = 5.0;  // 越界！

// 正确
if (index >= 0 && index < 10) {
    data[index] = 5.0;
}
```

**2. 空指针**
```c
// 错误
DevPvt *pPvt = (DevPvt*)prec->dpvt;
pPvt->channel = 5;  // pPvt可能为NULL

// 正确
if (pPvt != NULL) {
    pPvt->channel = 5;
}
```

**3. 释放后使用**
```c
// 错误
free(buffer);
buffer[0] = 10;  // 已释放！

// 正确
free(buffer);
buffer = NULL;
```

**调试**:
```bash
# 使用GDB
gdb ../../bin/linux-x86_64/YourApp
(gdb) run st.cmd
# 崩溃后
(gdb) bt  # backtrace
(gdb) list
```

---

## Channel Access

### Q: cagetfails with "Channel connect timed out"？

**A**: CA连接问题。

**检查步骤**:

```bash
# 1. IOC是否运行？
ps aux | grep YourApp

# 2. 网络连通性
ping <IOC_IP>

# 3. CA端口是否开放？
# UDP 5064, TCP 5065
netstat -an | grep 506

# 4. 防火墙？
sudo ufw allow 5064/udp
sudo ufw allow 5065/tcp

# 5. EPICS_CA_ADDR_LIST设置？
export EPICS_CA_ADDR_LIST=<IOC_IP>
export EPICS_CA_AUTO_ADDR_LIST=NO

caget LLRF:BPM:RFIn_01_Amp
```

### Q: camonitor显示"Not connected"？

**A**: PV名称错误或IOC未运行。

```bash
# 1. 确认PV名称（区分大小写）
caget LLRF:BPM:RFIn_01_Amp  # 正确
caget llrf:bpm:rfin_01_amp  # 错误（小写）

# 2. 列出所有PV
softIocPVA
# 在IOC控制台输入:
dbl  # database list

# 3. 使用通配符测试
caget "LLRF:BPM:*"
```

### Q: PV可以caget但caput失败？

**A**: 权限或Record类型问题。

**可能原因**:

**1. 只读Record**
```
# ai record是只读的
record(ai, "LLRF:BPM:RFIn_01_Amp") {...}

# 需要用ao
record(ao, "LLRF:BPM:SetVoltage") {...}
```

**2. CA Security配置**
```bash
# 检查 access security文件
cat accessSecurity.acf

# 临时禁用(仅测试)
# 删除st.cmd中的:
# asSetFilename("accessSecurity.acf")
```

---

## 数据库和Record

### Q: .db文件修改后不生效？

**A**: 需要重新加载数据库。

```bash
# 方法1: 重启IOC
# 停止IOC (Ctrl+C)
# 重新启动
./st.cmd

# 方法2: 动态重载（有限支持）
# 在IOC控制台
dbLoadRecords("db/YourApp.db")
```

### Q: calc record的表达式语法错误？

**A**: calc支持有限的C语法。

**支持的**:
```
# 数学运算
A+B, A-B, A*B, A/B, A^B

# 函数
SIN(A), COS(A), TAN(A), LOG(A), EXP(A)
ABS(A), SQRT(A), CEIL(A), FLOOR(A)
MIN(A,B), MAX(A,B)

# 逻辑
A&&B, A||B, !A
A==B, A!=B, A>B, A<B, A>=B, A<=B

# 三元运算符
A?B:C
```

**不支持的**:
```
# 复合赋值
A += B  # 错误

# 函数调用
myFunc(A)  # 错误

# 多语句
A=B; C=D  # 错误
```

**示例**:
```
# 正确
field(CALC, "A+B")
field(CALC, "A>10?1:0")
field(CALC, "SIN(A*3.14159/180)")

# 错误
field(CALC, "A+=B")  # 不支持
field(CALC, "return A+B;")  # 不支持
```

### Q: Waveform record怎么写入数据？

**A**: 需要特殊的处理。

```c
// 设备支持中
static long write_waveform(waveformRecord *prec) {
    // 获取数据指针
    float *data = (float*)prec->bptr;
    int nelm = prec->nelm;  // 最大长度

    // 写入数据
    for (int i = 0; i < nelm && i < actual_data_length; i++) {
        data[i] = source_data[i];
    }

    // 更新实际长度
    prec->nord = actual_data_length;

    return 0;
}
```

**Python客户端**:
```python
import epics

# 写入数组
pv = epics.PV('LLRF:BPM:Waveform')
data = [1.0, 2.0, 3.0, 4.0, 5.0]
pv.put(data)

# 读取数组
data = pv.get()
print(len(data))  # NORD (实际长度)
```

---

## 设备支持和驱动

### Q: DTYP设置错误，但record仍能加载？

**A**: 可能使用了默认设备支持。

**检查**:
```bash
# IOC控制台
dbpr "LLRF:BPM:RFIn_01_Amp", 10

# 查看DTYP字段
DTYP: Soft Channel  # 如果是这个，说明设备支持未注册
```

**解决**:
```c
// 确保注册DSET
epicsExportAddress(dset, devAiBPMMonitor);

// st.cmd中加载DBD
dbLoadDatabase("dbd/YourApp.dbd")
YourApp_registerRecordDeviceDriver(pdbbase)
```

### Q: 设备支持的init_record何时调用？

**A**: IOC启动时，dbLoadRecords之后。

**时序**:
```
1. iocInit()
   ↓
2. dbLoadRecords("db/YourApp.db")
   ↓
3. 对每个record调用init_record()
   ↓
4. IOC运行
```

**注意**: 如果硬件初始化在init_record中，确保在iocInit之前调用InitDevice()

### Q: 读写函数什么时候被调用？

**A**: 取决于SCAN字段。

```
SCAN = "Passive"      → 被其他record触发时
SCAN = "1 second"     → 每秒自动调用
SCAN = "I/O Intr"     → 硬件中断时
SCAN = "Event"        → 特定事件发生时
```

**调试**:
```c
static long read_ai(aiRecord *prec) {
    printf("read_ai called for %s\n", prec->name);
    // ...
}
```

---

## 性能优化

### Q: IOC CPU占用过高？

**A**: 检查扫描频率和处理逻辑。

**诊断**:
```bash
# 1. 查看扫描配置
dbgrep "SCAN" db/YourApp.db

# 如果有大量高频扫描（如.01 second），考虑降低

# 2. 使用top监控
top -p $(pidof YourApp)
```

**优化**:
```
# 降低非关键PV的扫描频率
# 从.1 second → 1 second
field(SCAN, "1 second")

# 使用I/O Intr替代轮询
field(SCAN, "I/O Intr")
```

### Q: 数据更新延迟大？

**A**: 可能的瓶颈。

**检查**:

**1. 网络延迟**
```bash
ping <IOC_IP>
# RTT应该< 1ms (局域网)
```

**2. CA缓冲设置**
```c
// IOC中增加缓冲
#define CA_BUFFER_SIZE 65536
```

**3. 扫描优先级**
```
# 提高优先级
field(PRIO, "HIGH")
```

### Q: 内存占用持续增长？

**A**: 可能有内存泄漏。

**检查**:
```bash
# Valgrind检测
valgrind --leak-check=full ../../bin/linux-x86_64/YourApp st.cmd

# 运行一段时间后Ctrl+C
# 查看leak summary
```

**常见泄漏**:
```c
// 错误: 每次调用都malloc
static long read_ai(aiRecord *prec) {
    float *data = malloc(100 * sizeof(float));  // 泄漏！
    // ... 忘记free
}

// 正确: 使用静态缓冲或正确free
static float data[100];  // 静态
// 或
float *data = malloc(...);
// ...
free(data);
```

---

## 调试技巧

### Q: 如何查看Record的所有字段？

**A**: 使用dbpr命令。

```bash
# IOC控制台
dbpr "LLRF:BPM:RFIn_01_Amp"

# 详细级别（0-4）
dbpr "LLRF:BPM:RFIn_01_Amp", 4

# 输出示例:
# ASG:                ASGNAME:
# DISS: 0             DISP: 0             DISA: 0
# ...
```

### Q: 如何启用调试日志？

**A**: 使用errlog或printf。

```c
// IOC中启用debug日志
#include <errlog.h>

// 设置日志级别
errlogInit(2000);  // buffer size
eltc(0);  // 显示所有级别

// 记录日志
errlogPrintf("Debug: channel=%d, value=%.3f\n", channel, value);
```

**Python端**:
```python
import epics
epics.ca.CAThread()  # 启用CA线程

# 启用debug
import os
os.environ['EPICS_CA_DEBUG'] = '1'
```

### Q: 如何追踪Record处理顺序？

**A**: 使用TPRO字段。

```bash
# IOC控制台
dbpf "LLRF:BPM:RFIn_01_Amp.TPRO", "1"

# 之后每次处理都会打印:
# process: LLRF:BPM:RFIn_01_Amp
```

### Q: 如何测试设备支持不依赖硬件？

**A**: 使用Mock驱动或Soft Channel。

**方法1**: Mock驱动
```c
// mockDriver.c
float Mock_ReadADC(int channel, int offset) {
    return 10.0 + channel * 0.5;  // 模拟数据
}
```

**方法2**: 临时用Soft Channel
```
record(ai, "LLRF:BPM:RFIn_01_Amp") {
    field(DTYP, "Soft Channel")  # 不调用设备支持
    field(INP, "10.5")           # 固定值
}
```

---

## 交叉编译和部署

### Q: 交叉编译后在ARM板上无法运行？

**A**: 架构不匹配或库缺失。

**检查**:
```bash
# 1. 确认文件架构
file ../../bin/linux-arm/YourApp
# 应该显示: ELF 32-bit LSB executable, ARM, ...

# 2. 检查依赖
arm-linux-gnueabihf-readelf -d ../../bin/linux-arm/YourApp | grep NEEDED

# 3. 在目标板上检查库
ldd /opt/YourApp/bin/linux-arm/YourApp
```

### Q: 如何在目标板上调试？

**A**: 使用gdbserver。

**目标板**:
```bash
# 安装gdbserver
sudo apt-get install gdbserver

# 运行程序通过gdbserver
gdbserver :1234 /opt/YourApp/bin/linux-arm/YourApp
```

**开发机**:
```bash
# 启动gdb
arm-linux-gnueabihf-gdb ../../bin/linux-arm/YourApp

# 连接目标板
(gdb) target remote 192.168.1.100:1234

# 调试
(gdb) break main
(gdb) continue
```

---

## 最佳实践

### Q: PV命名有什么规范？

**A**: 遵循分层命名。

**推荐**:
```
SYSTEM:SUBSYS:DEVICE:PARAM

例如:
LLRF:BPM:RFIn_01:Amp
LLRF:BPM:RFIn_01:Phase
LLRF:RF:Cavity01:SetVoltage
```

**避免**:
```
Amp1           # 太简短
LLRFBPMRFIn01Amp  # 太长无分隔
bpm_amp_1      # 使用下划线而非冒号
```

### Q: 应该把所有PV都放在一个.db文件吗？

**A**: 否，应该模块化。

**推荐**:
```
db/
├── bpm.db        # BPM相关PV
├── rf.db         # RF相关PV
├── vacuum.db     # 真空相关PV
└── system.db     # 系统状态PV
```

**st.cmd中加载**:
```bash
dbLoadRecords("db/bpm.db")
dbLoadRecords("db/rf.db")
dbLoadRecords("db/vacuum.db")
dbLoadRecords("db/system.db")
```

### Q: 何时使用calc record，何时用C代码？

**A**: 简单计算用calc，复杂逻辑用C。

**用calc**:
- 简单数学运算（加减乘除）
- 单位转换
- 阈值判断

**用C代码**:
- 复杂算法（FFT、滤波）
- 需要状态的逻辑
- 性能关键的代码

---

## 还有问题？

如果FAQ没有覆盖你的问题：

1. **查看官方文档**: https://docs.epics-controls.org/
2. **搜索邮件列表**: https://epics.anl.gov/tech-talk/
3. **提问**: tech-talk@aps.anl.gov
4. **查看GitHub Issues**: https://github.com/epics-base/epics-base/issues

**提问技巧**:
- 提供完整的错误信息
- 描述复现步骤
- 说明EPICS版本和操作系统
- 包含相关代码片段
