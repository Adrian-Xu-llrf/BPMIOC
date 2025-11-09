# 交叉编译详解

> **目标**: 掌握EPICS IOC交叉编译配置和流程
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 1-2天
> **前置知识**: Linux编译、Makefile基础

## 📋 本文档内容

1. 交叉编译概念
2. 工具链安装
3. EPICS Base交叉编译
4. BPMIOC交叉编译
5. 常见问题排查
6. 优化技巧

## 1️⃣ 交叉编译概念

### 什么是交叉编译

**交叉编译 (Cross Compilation)**：在一个平台上（主机，如PC x86_64）编译出另一个平台（目标，如ARM）的可执行程序。

```
主机平台 (Host)              目标平台 (Target)
   ↓                            ↓
x86_64 Linux PC      →      ARM Linux 嵌入式板

使用工具:
arm-linux-gnueabihf-gcc      在ARM板上运行
(在PC上执行)
```

### 为什么需要交叉编译

**原因**：
1. **性能**：目标板CPU较慢，编译时间长
2. **资源**：目标板内存/存储有限，无法安装完整开发环境
3. **便利**：在PC上开发更方便（编辑器、调试工具）

### 编译术语

| 术语 | 说明 | 示例 |
|------|------|------|
| **Build** | 执行编译的平台 | x86_64 PC |
| **Host** | 运行编译器的平台 | x86_64 PC |
| **Target** | 生成代码运行的平台 | ARM板 |
| **本地编译** | Build = Host = Target | 全是x86_64 |
| **交叉编译** | Build = Host ≠ Target | Build/Host=x86_64, Target=ARM |

## 2️⃣ 工具链安装

### ARM工具链

#### 方法1: 使用发行版包管理器（推荐用于开发）

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf

# 验证安装
arm-linux-gnueabihf-gcc --version
```

输出示例：
```
arm-linux-gnueabihf-gcc (Ubuntu 9.4.0-1ubuntu1~20.04) 9.4.0
```

#### 方法2: 下载官方工具链（推荐用于生产）

```bash
# 下载ARM官方工具链
cd /opt
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf.tar.xz

# 解压
tar xf gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf.tar.xz

# 添加到PATH
export PATH=/opt/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin:$PATH

# 永久添加（添加到~/.bashrc）
echo 'export PATH=/opt/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin:$PATH' >> ~/.bashrc
```

#### 方法3: Zynq/Petalinux SDK（用于Xilinx平台）

```bash
# 安装Petalinux
# 假设已下载petalinux-v2020.2-final-installer.run

chmod +x petalinux-v2020.2-final-installer.run
./petalinux-v2020.2-final-installer.run

# Source SDK环境
source /opt/petalinux/2020.2/settings.sh
```

### 验证工具链

```bash
# 检查编译器
arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-g++ --version

# 检查工具链组件
which arm-linux-gnueabihf-gcc
which arm-linux-gnueabihf-ld
which arm-linux-gnueabihf-ar
which arm-linux-gnueabihf-objdump

# 测试编译
cat > test.c <<'EOF'
#include <stdio.h>
int main() {
    printf("Hello ARM!\n");
    return 0;
}
EOF

arm-linux-gnueabihf-gcc test.c -o test_arm

# 查看文件类型
file test_arm
# 输出: test_arm: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)...
```

## 3️⃣ EPICS Base交叉编译

### 步骤1: 下载EPICS Base

```bash
cd /opt
sudo git clone --recursive https://github.com/epics-base/epics-base.git
cd epics-base
sudo git checkout R3.15.6  # 或其他稳定版本
```

### 步骤2: 配置交叉编译

编辑 `configure/CONFIG_SITE`:

```makefile
# configure/CONFIG_SITE

# 指定目标架构
CROSS_COMPILER_TARGET_ARCHS = linux-arm

# 可选：设置安装路径
#INSTALL_LOCATION = /opt/epics/base-arm
```

### 步骤3: 配置ARM工具链

创建或编辑 `configure/os/CONFIG_SITE.Common.linux-arm`:

```makefile
# configure/os/CONFIG_SITE.Common.linux-arm

# GNU编译器配置
GNU_TARGET = arm-linux-gnueabihf
GNU_DIR = /usr

# 如果使用自定义工具链路径：
#GNU_DIR = /opt/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf

# 编译器和链接器
CC = $(GNU_DIR)/bin/$(GNU_TARGET)-gcc
CCC = $(GNU_DIR)/bin/$(GNU_TARGET)-g++
CPP = $(GNU_DIR)/bin/$(GNU_TARGET)-cpp
AR = $(GNU_DIR)/bin/$(GNU_TARGET)-ar
LD = $(GNU_DIR)/bin/$(GNU_TARGET)-ld
RANLIB = $(GNU_DIR)/bin/$(GNU_TARGET)-ranlib

# 编译选项
OPT_CFLAGS_YES = -O2
OPT_CXXFLAGS_YES = -O2

# 可选：针对特定ARM CPU优化
#OPT_CFLAGS_YES += -mcpu=cortex-a9 -mfpu=neon
#OPT_CXXFLAGS_YES += -mcpu=cortex-a9 -mfpu=neon
```

### 步骤4: 编译EPICS Base

```bash
cd /opt/epics-base

# 清理之前的编译
make clean

# 交叉编译
make CROSS_COMPILER_TARGET_ARCHS=linux-arm

# 或者直接make（如果已在CONFIG_SITE中配置）
make
```

**编译时间**：约10-30分钟，取决于PC性能

### 步骤5: 验证编译结果

```bash
# 检查ARM可执行文件
ls -lh bin/linux-arm/

# 应该看到：
# softIoc
# caget
# caput
# camonitor
# ...

# 检查文件类型
file bin/linux-arm/softIoc
# 输出: bin/linux-arm/softIoc: ELF 32-bit LSB executable, ARM, EABI5...

# 检查库文件
ls -lh lib/linux-arm/
# 应该看到：
# libCom.so
# libca.so
# libdbCore.so
# ...
```

## 4️⃣ BPMIOC交叉编译

### 步骤1: 配置BPMIOC

编辑 `configure/CONFIG_SITE`:

```makefile
# BPMmonitor/configure/CONFIG_SITE

# 启用交叉编译
CROSS_COMPILER_TARGET_ARCHS = linux-arm
```

编辑 `configure/RELEASE`:

```makefile
# BPMmonitor/configure/RELEASE

# EPICS Base路径（包含ARM编译结果）
EPICS_BASE = /opt/epics-base

# 其他依赖...
```

### 步骤2: 配置平台特定的源文件

编辑 `BPMmonitorApp/src/Makefile`:

```makefile
# BPMmonitorApp/src/Makefile

TOP=../..
include $(TOP)/configure/CONFIG

PROD_IOC = BPMmonitor
DBD += BPMmonitor.dbd

BPMmonitor_DBD += base.dbd
BPMmonitor_DBD += devBPMMonitor.dbd

# 公共源文件
BPMmonitor_SRCS += BPMmonitor_registerRecordDeviceDriver.cpp
BPMmonitor_SRCS += devBPMMonitor.c
BPMmonitor_SRCS += driverWrapper.c

# 平台特定源文件
ifeq ($(T_A),linux-x86_64)
    # PC开发环境 - 使用Mock库
    # （不包含硬件相关代码，运行时动态加载Mock库）
else ifeq ($(T_A),linux-arm)
    # ARM目标板 - 使用真实驱动
    # （不包含硬件相关代码，运行时动态加载真实驱动）
endif

BPMmonitor_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
```

### 步骤3: 交叉编译BPMIOC

```bash
cd /opt/BPMmonitor

# 清理
make clean

# 编译（同时编译x86_64和ARM版本）
make

# 或只编译ARM版本
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

### 步骤4: 验证编译结果

```bash
# 检查可执行文件
ls -lh bin/linux-arm/
# BPMmonitor

file bin/linux-arm/BPMmonitor
# 输出: bin/linux-arm/BPMmonitor: ELF 32-bit LSB executable, ARM...

# 检查依赖库
arm-linux-gnueabihf-ldd bin/linux-arm/BPMmonitor
# 或使用 readelf
arm-linux-gnueabihf-readelf -d bin/linux-arm/BPMmonitor | grep NEEDED
```

输出示例：
```
 0x00000001 (NEEDED)                     Shared library: [libdbCore.so.3.15]
 0x00000001 (NEEDED)                     Shared library: [libca.so.3.15]
 0x00000001 (NEEDED)                     Shared library: [libCom.so.3.15]
 0x00000001 (NEEDED)                     Shared library: [libdl.so.2]
 0x00000001 (NEEDED)                     Shared library: [libm.so.6]
 0x00000001 (NEEDED)                     Shared library: [libpthread.so.0]
 0x00000001 (NEEDED)                     Shared library: [libc.so.6]
```

## 5️⃣ 交叉编译硬件驱动库

### 编译真实硬件驱动

假设硬件驱动源码在 `/opt/BPMDriver`:

```bash
cd /opt/BPMDriver

# Makefile for cross-compilation
cat > Makefile <<'EOF'
# Target architecture
ARCH ?= arm
ifeq ($(ARCH),arm)
    CC = arm-linux-gnueabihf-gcc
    CFLAGS = -Wall -fPIC -O2
else
    CC = gcc
    CFLAGS = -Wall -fPIC -O2
endif

# Library name
LIB = libBPMDriver.so

# Source files
SRCS = BPMDriver.c hardware_io.c

# Object files
OBJS = $(SRCS:.c=.o)

all: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
EOF

# 编译ARM版本
make clean
make ARCH=arm

# 验证
file libBPMDriver.so
# 输出: libBPMDriver.so: ELF 32-bit LSB shared object, ARM...
```

## 6️⃣ 优化技巧

### 编译优化选项

```makefile
# configure/CONFIG_SITE.Common.linux-arm

# 优化级别
OPT_CFLAGS_YES = -O3        # 最高优化（体积和速度）
#OPT_CFLAGS_YES = -O2       # 平衡优化（推荐）
#OPT_CFLAGS_YES = -Os       # 最小体积优化

# CPU特定优化
# For Cortex-A9 (Zynq-7000)
OPT_CFLAGS_YES += -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard

# For Cortex-A53 (Raspberry Pi 3/4)
#OPT_CFLAGS_YES += -mcpu=cortex-a53 -mfpu=neon-fp-armv8

# 调试符号
OPT_CFLAGS_YES += -g        # 包含调试符号（开发时）
#OPT_CFLAGS_YES +=          # 不包含调试符号（生产时）
```

### 减小可执行文件大小

```bash
# 剥离调试符号（生产环境）
arm-linux-gnueabihf-strip bin/linux-arm/BPMmonitor

# 比较大小
ls -lh bin/linux-arm/BPMmonitor
# Before strip: 2.5MB
# After strip:  800KB
```

### 静态链接 vs 动态链接

```makefile
# 动态链接（默认，推荐）
# - 可执行文件小
# - 共享库节省内存
# - 更新库无需重新编译IOC

# 静态链接（特殊情况）
# - 可执行文件大
# - 部署简单（无需传输库）
# - 用于无法安装库的环境

# 启用静态链接：
#STATIC_BUILD = YES
```

## 7️⃣ 常见问题排查

### 问题1: 找不到工具链

**错误**：
```
make: arm-linux-gnueabihf-gcc: Command not found
```

**解决方案**：
```bash
# 检查工具链是否安装
which arm-linux-gnueabihf-gcc

# 如果没有，安装工具链
sudo apt-get install gcc-arm-linux-gnueabihf

# 或添加到PATH
export PATH=/opt/gcc-arm-xxx/bin:$PATH
```

### 问题2: 找不到头文件

**错误**：
```
fatal error: stdio.h: No such file or directory
```

**解决方案**：
```bash
# 检查sysroot
arm-linux-gnueabihf-gcc -print-sysroot

# 如果sysroot不存在，需要：
# 1. 安装libc-dev
sudo apt-get install libc6-dev-armhf-cross

# 2. 或指定sysroot
CFLAGS += --sysroot=/path/to/sysroot
```

### 问题3: 链接错误

**错误**：
```
undefined reference to `pthread_create'
```

**解决方案**：
```makefile
# 添加链接库
BPMmonitor_SYS_LIBS += pthread
BPMmonitor_SYS_LIBS += dl
BPMmonitor_SYS_LIBS += m
```

### 问题4: EPICS库路径错误

**错误**：
```
cannot find -lCom
```

**解决方案**：
```bash
# 检查EPICS Base是否已为ARM编译
ls $EPICS_BASE/lib/linux-arm/

# 如果没有，重新编译EPICS Base
cd $EPICS_BASE
make CROSS_COMPILER_TARGET_ARCHS=linux-arm
```

### 问题5: 运行时库加载错误

**错误（在ARM板上运行时）**：
```
error while loading shared libraries: libCom.so.3.15: cannot open shared object file
```

**解决方案**：
```bash
# 在目标板上设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/epics-base/lib/linux-arm:$LD_LIBRARY_PATH

# 或添加到 /etc/ld.so.conf.d/
echo "/opt/epics-base/lib/linux-arm" > /etc/ld.so.conf.d/epics.conf
ldconfig
```

## 8️⃣ 完整编译脚本

### 自动化编译脚本

创建 `build_arm.sh`:

```bash
#!/bin/bash
# build_arm.sh - BPMIOC ARM交叉编译脚本

set -e  # 遇到错误立即退出

echo "=== BPMIOC ARM Cross-Compilation Script ==="

# 配置
EPICS_BASE="/opt/epics-base"
BPMIOC_ROOT="/opt/BPMmonitor"
ARCH="linux-arm"

# 1. 检查工具链
echo ""
echo "[1/5] Checking toolchain..."
if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
    echo "ERROR: arm-linux-gnueabihf-gcc not found!"
    echo "Please install: sudo apt-get install gcc-arm-linux-gnueabihf"
    exit 1
fi
echo "Toolchain OK: $(arm-linux-gnueabihf-gcc --version | head -1)"

# 2. 编译EPICS Base
echo ""
echo "[2/5] Building EPICS Base for ARM..."
cd "$EPICS_BASE"

# 检查是否已编译
if [ ! -f "bin/$ARCH/softIoc" ]; then
    echo "  Compiling EPICS Base (this may take 10-30 minutes)..."
    make clean
    make CROSS_COMPILER_TARGET_ARCHS=$ARCH
    echo "  EPICS Base compiled successfully"
else
    echo "  EPICS Base already compiled, skipping"
fi

# 3. 编译BPMIOC
echo ""
echo "[3/5] Building BPMIOC for ARM..."
cd "$BPMIOC_ROOT"

make clean
make CROSS_COMPILER_TARGET_ARCHS=$ARCH

# 4. 验证编译结果
echo ""
echo "[4/5] Verifying build..."
if [ ! -f "bin/$ARCH/BPMmonitor" ]; then
    echo "ERROR: BPMmonitor executable not found!"
    exit 1
fi

echo "  Executable: bin/$ARCH/BPMmonitor"
file "bin/$ARCH/BPMmonitor"
ls -lh "bin/$ARCH/BPMmonitor"

# 5. 创建部署包
echo ""
echo "[5/5] Creating deployment package..."
PACKAGE_NAME="bpmioc-arm-$(date +%Y%m%d-%H%M%S).tar.gz"

tar czf "$PACKAGE_NAME" \
    bin/$ARCH \
    dbd \
    db \
    iocBoot

echo "  Package created: $PACKAGE_NAME"
ls -lh "$PACKAGE_NAME"

echo ""
echo "=== Build Complete ==="
echo "Next steps:"
echo "  1. Transfer package to ARM board:"
echo "     scp $PACKAGE_NAME root@192.168.1.100:/tmp/"
echo "  2. Extract on ARM board:"
echo "     tar xzf /tmp/$PACKAGE_NAME -C /opt/BPMmonitor/"
echo "  3. Run IOC:"
echo "     cd /opt/BPMmonitor/iocBoot/iocBPMmonitor"
echo "     ./st.cmd"
```

使用脚本：

```bash
chmod +x build_arm.sh
./build_arm.sh
```

## 📝 交叉编译检查清单

### 编译前

- [ ] ARM工具链已安装
- [ ] EPICS Base源码已下载
- [ ] BPMIOC源码已准备
- [ ] 配置文件已修改（CONFIG_SITE, RELEASE）

### 编译中

- [ ] EPICS Base编译成功
- [ ] BPMIOC编译成功
- [ ] 硬件驱动库编译成功（如果需要）
- [ ] 无编译警告或错误

### 编译后

- [ ] 可执行文件存在（bin/linux-arm/BPMmonitor）
- [ ] 文件类型正确（ELF ARM）
- [ ] 库依赖正确（readelf -d）
- [ ] 文件大小合理（<5MB）

## 🔗 相关文档

- **[01-deployment-overview.md](./01-deployment-overview.md)** - 部署概览
- **[03-target-setup.md](./03-target-setup.md)** - 目标板设置
- **[Part 7: 03-cross-compile.md](../part7-build-system/03-cross-compile.md)** - 交叉编译基础

---

**下一步**: 学习 [目标板设置](./03-target-setup.md)
