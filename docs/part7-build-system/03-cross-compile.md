# Part 7.3: 交叉编译配置

> **目标**: 掌握EPICS的交叉编译配置
> **难度**: ⭐⭐⭐⭐☆
> **时间**: 60分钟

## 📖 什么是交叉编译？

**交叉编译** = 在一个平台（主机）上编译，运行在另一个平台（目标）上

```
主机（Host）                     目标（Target）
─────────────                   ─────────────
Linux x86_64       编译          Linux ARM
(开发机器)         ───→          (嵌入式设备)
                  运行在
```

**为什么需要？**
- 嵌入式设备资源有限（RAM、CPU）
- 交叉编译速度快（主机性能强）
- 工具链完整（主机有完整开发环境）

---

## 🎯 EPICS支持的架构

### 常见目标架构

| 架构 | 说明 | 示例设备 |
|------|------|----------|
| `linux-x86_64` | 64位Linux PC | 开发机 |
| `linux-x86` | 32位Linux PC | 老PC |
| `linux-arm` | ARM Linux | Raspberry Pi, BeagleBone |
| `linux-aarch64` | 64位ARM | Raspberry Pi 4 |
| `vxWorks-ppc32` | VxWorks PowerPC | 工业控制器 |
| `RTEMS-*` | RTEMS实时系统 | 嵌入式 |
| `win32-x86` | Windows 32位 | Windows PC |

---

## 🔧 配置交叉编译

### 步骤1: 安装交叉编译工具链

**Linux ARM示例**:

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-linux-gnueabihf
sudo apt-get install g++-arm-linux-gnueabihf

# 验证安装
arm-linux-gnueabihf-gcc --version
```

---

### 步骤2: 配置EPICS Base

**编辑EPICS Base的configure/os/CONFIG_SITE.Common.linux-arm**:

```bash
cd /opt/epics/base/configure/os

# 如果文件不存在，创建它
vim CONFIG_SITE.Common.linux-arm
```

**内容**:
```makefile
# 交叉编译器前缀
GNU_TARGET = arm-linux-gnueabihf

# 编译器
GNU_DIR = /usr
CC  = $(GNU_DIR)/bin/$(GNU_TARGET)-gcc
CXX = $(GNU_DIR)/bin/$(GNU_TARGET)-g++
AR  = $(GNU_DIR)/bin/$(GNU_TARGET)-ar
LD  = $(GNU_DIR)/bin/$(GNU_TARGET)-ld
```

**重新编译EPICS Base**:
```bash
cd /opt/epics/base
make clean
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

---

### 步骤3: 配置BPMIOC交叉编译

**编辑configure/CONFIG_SITE**:

```makefile
# 启用ARM交叉编译
CROSS_COMPILER_TARGET_ARCHS = linux-arm
```

**或命令行指定**:
```bash
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

---

### 步骤4: 编译BPMIOC

```bash
cd /home/user/BPMIOC

# 清理
make clean

# 交叉编译
make

# 或显式指定
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

**输出**:
```
bin/
├── linux-x86_64/
│   └── BPMmonitor          # PC版本
└── linux-arm/
    └── BPMmonitor          # ARM版本 ← 新生成
```

---

## 📊 架构检测机制

### EPICS_HOST_ARCH

**自动检测主机架构**:

```bash
# 查看主机架构
make EPICS_HOST_ARCH
# 输出：linux-x86_64

# 或
echo $EPICS_HOST_ARCH
```

**EPICS如何检测**:
```bash
# EPICS Base提供的脚本
$EPICS_BASE/startup/EpicsHostArch

# 内部逻辑（简化）:
uname -s  # Linux
uname -m  # x86_64
→ 组合为 "linux-x86_64"
```

---

### T_A变量

**T_A** = **T**arget **A**rchitecture

在Makefile中：
```makefile
# 主机编译时
T_A = linux-x86_64

# 交叉编译时
T_A = linux-arm
```

**用途**:
```makefile
# 根据目标架构选择源文件
ifeq ($(T_A),linux-arm)
    BPMmonitor_SRCS += arm_specific.c
else
    BPMmonitor_SRCS += x86_specific.c
endif
```

---

## 🎨 实际应用

### 示例1: PC和ARM使用不同的驱动

**Makefile**:
```makefile
ifeq ($(T_A),linux-x86_64)
    # PC: 使用Mock库
    BPMmonitor_SRCS += mock_hardware.c
    BPMmonitor_SYS_LIBS += BPMmock
else ifeq ($(T_A),linux-arm)
    # ARM: 使用真实硬件
    BPMmonitor_SRCS += real_hardware.c
    BPMmonitor_SYS_LIBS += fpga
endif
```

---

### 示例2: 条件链接库

```makefile
# 所有平台
BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

# ARM平台额外链接
ifeq ($(T_A),linux-arm)
    BPMmonitor_SYS_LIBS += wiringPi  # GPIO库
endif
```

---

### 示例3: 编译选项优化

```makefile
# ARM平台优化
ifeq ($(T_A),linux-arm)
    USR_CFLAGS += -march=armv7-a
    USR_CFLAGS += -mfpu=neon
endif
```

---

## 🚀 部署到目标设备

### 方法1: NFS挂载

**主机**:
```bash
# 导出BPMIOC目录
sudo vim /etc/exports
# 添加：
/home/user/BPMIOC *(rw,sync,no_subtree_check)

# 重启NFS
sudo exportfs -ra
```

**目标设备**:
```bash
# 挂载
mount -t nfs 192.168.1.100:/home/user/BPMIOC /mnt/BPMIOC

# 运行
cd /mnt/BPMIOC/iocBoot/iocBPMmonitor
./st.cmd
```

---

### 方法2: SCP复制

```bash
# 复制可执行文件和依赖
scp -r bin/linux-arm root@192.168.1.10:/opt/BPMIOC/bin/
scp -r lib/linux-arm root@192.168.1.10:/opt/BPMIOC/lib/
scp -r db root@192.168.1.10:/opt/BPMIOC/
scp -r dbd root@192.168.1.10:/opt/BPMIOC/

# 复制启动脚本
scp -r iocBoot root@192.168.1.10:/opt/BPMIOC/
```

**目标设备**:
```bash
cd /opt/BPMIOC/iocBoot/iocBPMmonitor
chmod +x st.cmd
./st.cmd
```

---

### 方法3: SD卡/U盘

```bash
# 打包
tar czf BPMIOC-arm.tar.gz bin/linux-arm lib/linux-arm db dbd iocBoot

# 复制到SD卡
cp BPMIOC-arm.tar.gz /media/sdcard/

# 目标设备解压
tar xzf /media/sdcard/BPMIOC-arm.tar.gz -C /opt/
```

---

## ⚠️ 常见问题

### 问题1: 找不到交叉编译器

**错误**:
```
arm-linux-gnueabihf-gcc: command not found
```

**解决**:
```bash
# 安装交叉编译器
sudo apt-get install gcc-arm-linux-gnueabihf

# 或指定路径
export PATH=$PATH:/opt/cross/bin
```

---

### 问题2: 库文件架构不匹配

**错误**:
```
libCom.so: cannot open shared object file
```

**原因**: 链接了主机版本的EPICS库

**解决**: 确保EPICS Base也为ARM架构编译
```bash
cd /opt/epics/base
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

---

### 问题3: 运行时找不到共享库

**错误**（在目标设备上）:
```
./BPMmonitor: error while loading shared libraries: libCom.so.3.15
```

**解决**:
```bash
# 方法1: 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/BPMIOC/lib/linux-arm

# 方法2: 添加到st.cmd
#!/opt/BPMIOC/bin/linux-arm/BPMmonitor
export LD_LIBRARY_PATH=/opt/BPMIOC/lib/linux-arm
```

---

## 🔍 交叉编译调试

### 检查编译器

```bash
# 查看使用的编译器
make VERBOSE=1 | grep "gcc"

# 应该看到：
arm-linux-gnueabihf-gcc ...
```

---

### 检查生成文件的架构

```bash
# 查看可执行文件架构
file bin/linux-arm/BPMmonitor

# 输出：
bin/linux-arm/BPMmonitor: ELF 32-bit LSB executable, ARM, ...
```

---

### 查看链接的库

```bash
# 查看动态库依赖
arm-linux-gnueabihf-readelf -d bin/linux-arm/BPMmonitor | grep NEEDED

# 输出：
0x00000001 (NEEDED) Shared library: [libCom.so.3.15]
0x00000001 (NEEDED) Shared library: [libc.so.6]
```

---

## ✅ 学习检查点

- [ ] 理解交叉编译的概念和用途
- [ ] 能够安装交叉编译工具链
- [ ] 能够配置EPICS Base交叉编译
- [ ] 能够配置BPMIOC交叉编译
- [ ] 理解T_A变量的作用
- [ ] 能够根据架构条件编译
- [ ] 能够部署到目标设备
- [ ] 能够解决交叉编译问题

---

## 🎯 总结

### 交叉编译的关键

**三个层次的配置**:
1. **工具链**: 安装arm-linux-gnueabihf-gcc
2. **EPICS Base**: 配置和编译ARM版本
3. **应用**: 设置CROSS_COMPILER_TARGET_ARCHS

### 架构检测流程

```
make
  ↓
检测EPICS_HOST_ARCH (linux-x86_64)
  ↓
检查CROSS_COMPILER_TARGET_ARCHS
  ↓
如果定义了 → 添加目标架构（linux-arm）
  ↓
为每个架构分别编译
  ├─ linux-x86_64 (主机)
  └─ linux-arm (目标)
```

### BPMIOC的交叉编译策略

**开发时**: PC编译，使用Mock库调试
**部署时**: ARM交叉编译，使用真实硬件

**下一步**: 学习Part 7.4 - 依赖管理，理解EPICS模块之间的依赖关系！

---

**关键理解**: 交叉编译让你在强大的PC上开发，部署到资源受限的嵌入式设备！💡
