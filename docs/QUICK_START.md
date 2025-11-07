# ⚡ BPMIOC 快速开始指南

> **目标**: 30分钟内让BPMIOC运行起来
> **难度**: ⭐⭐☆☆☆
> **前置条件**: Linux系统（Ubuntu/CentOS）+ 基本命令行知识

## 🎯 你将完成什么

完成本指南后，你将：
- ✅ 安装好EPICS Base
- ✅ 编译BPMIOC
- ✅ 在模拟模式下运行IOC
- ✅ 访问PV（Process Variables）
- ✅ 理解基本工作流程

## 📋 检查清单

在开始之前，确保：
- [ ] Linux系统（Ubuntu 20.04+ 或 CentOS 7+）
- [ ] 至少 5GB 可用磁盘空间
- [ ] 2GB+ 内存
- [ ] 网络连接（下载EPICS Base）
- [ ] 基本命令行操作能力

## 🚀 快速步骤

### 步骤1: 安装依赖（2分钟）

```bash
# Ubuntu
sudo apt update
sudo apt install -y build-essential git wget libreadline-dev perl

# CentOS
sudo yum groupinstall -y "Development Tools"
sudo yum install -y git wget readline-devel perl
```

### 步骤2: 安装EPICS Base（10分钟）

```bash
# 下载
cd ~
wget https://epics.anl.gov/download/base/base-3.15.6.tar.gz
tar -xzf base-3.15.6.tar.gz
cd base-3.15.6

# 编译
make -j$(nproc)

# 配置环境变量
cat >> ~/.bashrc << 'EOF'
export EPICS_BASE=$HOME/base-3.15.6
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}
EOF

source ~/.bashrc
```

**验证安装**:
```bash
which softIoc
# 应该显示: /home/yourname/base-3.15.6/bin/linux-x86_64/softIoc
```

### 步骤3: 克隆并配置BPMIOC（2分钟）

```bash
cd ~
git clone <your-repo-url> BPMIOC
cd BPMIOC

# 配置EPICS_BASE路径
vim configure/RELEASE
```

修改为（将yourname改为你的用户名）:
```makefile
EPICS_BASE=/home/yourname/base-3.15.6
```

### 步骤4: 启用模拟模式（5分钟）

编辑驱动层源码以支持无硬件运行:

```bash
vim BPMmonitorApp/src/driverWrapper.c
```

找到 `InitDevice()` 函数，在文件开头添加:
```c
static int use_simulation = 0;
```

修改 `InitDevice()`:
```c
int InitDevice()
{
    printf("=== BPM Monitor Driver Initialization ===\n");

    handle = dlopen("/usr/lib/liblowlevel.so", RTLD_LAZY);

    if (!handle) {
        printf("WARNING: Cannot load liblowlevel.so\n");
        printf("WARNING: Using SIMULATION mode\n");
        use_simulation = 1;

        scanIoInit(&ioScanPvt);
        epicsThreadCreate("BPMMonitor", 50, 20000,
                          (EPICSTHREADFUNC)my_thread, NULL);
        return 0;
    }

    // ... 原有代码 ...
}
```

修改 `my_thread()` 添加模拟数据:
```c
static void my_thread(void *arg)
{
    static double sim_time = 0.0;

    while (1) {
        if (use_simulation) {
            // 模拟8个RF通道的数据
            for (int i = 0; i < 8; i++) {
                Amp[i] = 1.0 + 0.5 * sin(sim_time + i * 0.5);
                Phase[i] = fmod(sim_time * 10.0 + i * 45.0, 360.0);
            }
            sim_time += 0.1;
        } else {
            // 真实硬件模式
            for (int i = 0; i < 8; i++) {
                (*getRfInfoFunc)(i, &Amp[i], &Phase[i]);
            }
        }

        scanIoRequest(ioScanPvt);
        epicsThreadSleep(0.1);
    }
}
```

**注**: 完整的模拟模式补丁见 [part1-quick-reproduction/05-enable-simulation.md](./part1-quick-reproduction/05-enable-simulation.md)

### 步骤5: 编译（3分钟）

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

**验证**:
```bash
ls -lh bin/*/BPMmonitor
ls -lh db/*.db
```

应该看到:
- `bin/linux-x86_64/BPMmonitor` - 可执行文件
- `db/BPMMonitor.db` - 数据库文件
- `db/BPMCal.db` - 校准数据库

### 步骤6: 运行IOC（2分钟）

```bash
cd iocBoot/iocBPMmonitor
./st.cmd
```

**预期输出**:
```
=== BPM Monitor Driver Initialization ===
WARNING: Cannot load liblowlevel.so
WARNING: Using SIMULATION mode
Starting iocInit
...
iocRun: All initialization complete
epics>
```

看到 `epics>` 提示符表示IOC已成功启动！

### 步骤7: 验证PV访问（5分钟）

**在IOC Shell中验证**:
```bash
epics> dbl
# 应该列出所有PV

epics> dbgf "iLinac_007:BPM14And15:RF3Amp"
# 应该显示一个数值

epics> dbpr "iLinac_007:BPM14And15:RF3Amp"
# 显示记录详细信息
```

**打开新终端，使用CA工具验证**:
```bash
# 读取RF振幅
caget iLinac_007:BPM14And15:RF3Amp
caget iLinac_007:BPM14And15:RF4Amp

# 监控变化
camonitor iLinac_007:BPM14And15:RF3Amp

# 读取波形
caget -# 100 iLinac_007:BPM14And15:RF3TrigWaveform
```

**用Python验证**:
```bash
pip install pyepics

python3 << 'EOF'
import epics
import time

# 读取PV
amp = epics.caget('iLinac_007:BPM14And15:RF3Amp')
print(f"RF3 Amplitude: {amp:.3f} V")

# 监控10次
def callback(pvname=None, value=None, **kws):
    print(f"{pvname} = {value:.3f}")

pv = epics.PV('iLinac_007:BPM14And15:RF3Amp', callback=callback)

for i in range(10):
    time.sleep(1)
EOF
```

## ✅ 成功标准

如果你看到：
- ✅ IOC启动没有错误
- ✅ `caget` 能读取到数值
- ✅ `camonitor` 显示数值在变化
- ✅ Python能访问PV

**恭喜！你已经成功运行BPMIOC了！** 🎉

## 🔍 理解你做了什么

### 数据流（简化版）

```
模拟数据 (my_thread)
    ↓ [每100ms更新]
全局缓冲区 (Amp[], Phase[])
    ↓ [I/O中断触发]
设备支持层 (devBPMMonitor.c)
    ↓ [调用ReadData()]
驱动层 (driverWrapper.c)
    ↓ [Channel Access]
客户端 (caget, Python)
```

### 关键组件

1. **驱动层** (`driverWrapper.c`): 模拟硬件数据
2. **设备支持层** (`devBPMMonitor.c`): 连接EPICS记录和驱动
3. **数据库** (`BPMMonitor.db`): 定义PV
4. **IOC**: 运行所有组件的容器

## 🐛 遇到问题？

### 问题1: `make` 失败

```bash
# 检查EPICS_BASE是否正确
echo $EPICS_BASE
# 应该显示: /home/yourname/base-3.15.6

# 重新source
source ~/.bashrc
```

### 问题2: IOC启动但PV访问不到

```bash
# 检查网络
ping localhost

# 检查PV名称是否正确
./st.cmd
epics> dbl | grep RF3
```

### 问题3: `caget` 找不到命令

```bash
# 确保PATH正确
echo $PATH | grep EPICS

# 重新配置
source ~/.bashrc
which caget
```

**更多问题** → 查看 [part1-quick-reproduction/08-troubleshooting.md](./part1-quick-reproduction/08-troubleshooting.md)

## 📚 下一步

现在你已经成功运行BPMIOC，接下来可以：

### 选项A: 系统学习（推荐）
跟随 [8周学习计划](./part11-weekly-plan/)

### 选项B: 快速实践
完成 [基础实验](./part8-hands-on-labs/labs-basic/)：
1. [lab01-trace-rf-amp.md](./part8-hands-on-labs/labs-basic/lab01-trace-rf-amp.md) - 追踪数据流
2. [lab02-modify-scan.md](./part8-hands-on-labs/labs-basic/lab02-modify-scan.md) - 修改扫描周期
3. [lab03-add-debug.md](./part8-hands-on-labs/labs-basic/lab03-add-debug.md) - 添加调试输出

### 选项C: 深入理解
阅读架构文档：
- [Part 3: BPMIOC架构](./part3-bpmioc-architecture/)
- [Part 4: 驱动层详解](./part4-driver-layer/)

### 选项D: 查看完整路线
阅读 [ROADMAP.md](./ROADMAP.md) 了解三个学习层次

## 💡 学习建议

1. **不要跳过实践**: 看文档的同时一定要动手
2. **做好笔记**: 记录遇到的问题和解决方法
3. **理解原理**: 不仅要知道怎么做，还要知道为什么
4. **循序渐进**: 不要试图一次理解所有内容

## 🎓 学习路径建议

```
你现在在这里 ↓

[✓] 快速开始      ← 30分钟
[ ] EPICS基础     ← 1-2天 (Part 2)
[ ] 理解架构      ← 3-5天 (Part 3)
[ ] 驱动层        ← 1-2周 (Part 4)
[ ] 设备支持层    ← 1周 (Part 5)
[ ] 数据库层      ← 1周 (Part 6)
[ ] 独立开发      ← 1-2周 (Part 9)
```

## 📞 获取帮助

- 📖 查看 [FAQ](./part18-appendix/faq.md)
- 🔧 查看 [故障排查指南](./part18-appendix/troubleshooting-guide.md)
- 💬 在GitHub提Issue
- 📧 联系维护者

---

**🎉 恭喜完成快速开始！**

现在开始你的EPICS学习之旅吧！建议下一步查看 [ROADMAP.md](./ROADMAP.md) 了解完整学习路径。
