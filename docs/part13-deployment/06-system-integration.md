# 系统集成

> **目标**: 集成真实硬件和验证系统功能
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 1-2天

## 📋 硬件集成

### 连接FPGA

#### 检查硬件连接

```bash
# 检查PCIe设备（如果使用PCIe）
lspci | grep -i xilinx

# 检查内存映射设备
ls -l /dev/mem

# 检查设备树（ARM平台）
ls /sys/firmware/devicetree/base/

# 检查GPIO
ls /sys/class/gpio/
```

### 加载硬件驱动

```bash
# 检查驱动是否已加载
lsmod | grep bpm

# 如果是内核模块
modprobe bpm_driver

# 如果是用户态驱动
export BPM_DRIVER_LIB=/opt/BPMDriver/lib/libBPMDriver.so
```

### 测试硬件通信

创建测试程序 `test_hardware.c`:

```c
#include <stdio.h>
#include <dlfcn.h>

typedef int (*DeviceInit_t)(void);
typedef float (*ReadADC_t)(int channel, int type);

int main() {
    void *handle = dlopen("/opt/BPMDriver/lib/libBPMDriver.so", RTLD_LAZY);
    if (!handle) {
        printf("ERROR: Cannot load driver: %s\n", dlerror());
        return 1;
    }

    DeviceInit_t DeviceInit = (DeviceInit_t)dlsym(handle, "BPM_DeviceInit");
    ReadADC_t ReadADC = (ReadADC_t)dlsym(handle, "BPM_RFIn_ReadADC");

    if (!DeviceInit || !ReadADC) {
        printf("ERROR: Cannot find functions\n");
        return 1;
    }

    // 初始化硬件
    printf("Initializing hardware...\n");
    if (DeviceInit() != 0) {
        printf("ERROR: Hardware init failed\n");
        return 1;
    }

    // 读取数据
    printf("Reading ADC channels...\n");
    for (int ch = 0; ch < 8; ch++) {
        float value = ReadADC(ch, 0);
        printf("  Channel %d: %.3f\n", ch, value);
    }

    dlclose(handle);
    return 0;
}
```

编译运行：

```bash
gcc test_hardware.c -ldl -o test_hardware
./test_hardware
```

## 🔍 IOC集成测试

### 启动IOC

```bash
cd /opt/BPMmonitor/iocBoot/iocBPMmonitor
./st.cmd
```

期望输出：

```
dbLoadDatabase("../../dbd/BPMmonitor.dbd")
BPMmonitor_registerRecordDeviceDriver(pdbbase)
dbLoadRecords("../../db/BPMmonitor.db")
iocInit()
Starting iocInit
############################################################################
## EPICS R3.15.6
## EPICS Base built Nov  9 2025
############################################################################
iocRun: All initialization complete
```

### 验证PV访问

在PC上：

```bash
# 设置CA地址
export EPICS_CA_ADDR_LIST=192.168.1.100

# 测试连接
caget LLRF:BPM:RFIn_01_Amp
caget LLRF:BPM:RFIn_01_Pha

# 监控数据
camonitor LLRF:BPM:RFIn_01_Amp

# 写入测试
caput LLRF:BPM:RF3RegAddr 0x1000
caput LLRF:BPM:RF3RegValue 0xABCD
```

## 📊 性能验证

### 测试数据采集速率

Python测试脚本：

```python
#!/usr/bin/env python3
import epics
import time

pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

# 测试读取速率
count = 0
start_time = time.time()

while time.time() - start_time < 10:
    value = pv.get()
    count += 1

rate = count / 10.0
print(f"Read rate: {rate:.1f} reads/sec")
```

### 测试CA响应延迟

```bash
#!/bin/bash
# 测试caget延迟

for i in {1..100}; do
    time caget LLRF:BPM:RFIn_01_Amp > /dev/null
done 2>&1 | grep real | awk '{sum+=$2; count++} END {print "Average:", sum/count, "s"}'
```

## 🔗 相关文档

- [05-startup-config.md](./05-startup-config.md)
- [07-monitoring.md](./07-monitoring.md)
- [08-troubleshooting.md](./08-troubleshooting.md)
