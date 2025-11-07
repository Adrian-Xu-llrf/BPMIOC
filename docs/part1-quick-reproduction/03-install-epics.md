# 安装 EPICS Base

> **预计时间**: 15-20分钟
> **难度**: ⭐⭐⭐☆☆

## 概述

EPICS Base 是所有 EPICS 应用的基础,必须先安装才能编译 BPMIOC。

## 系统要求

### Ubuntu 20.04/22.04
```bash
# 安装编译工具链
sudo apt update
sudo apt install -y build-essential git wget libreadline-dev perl

# 验证
gcc --version     # 应该 >= 4.8
make --version
```

### CentOS 7/8
```bash
# 安装开发工具
sudo yum groupinstall -y "Development Tools"
sudo yum install -y git wget readline-devel perl

# 验证
gcc --version
make --version
```

### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 Homebrew (如果还没有)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install readline
```

## 下载 EPICS Base

###方法1: 官方下载 (推荐)

```bash
cd ~
wget https://epics.anl.gov/download/base/base-3.15.6.tar.gz
tar -xzf base-3.15.6.tar.gz
cd base-3.15.6
```

### 方法2: Git 克隆

```bash
cd ~
git clone --branch R3.15.6 --depth 1 \
  https://github.com/epics-base/epics-base.git base-3.15.6
cd base-3.15.6
```

## 配置 EPICS Base

### 检查目标架构

```bash
./startup/EpicsHostArch
```

**输出示例**:
- `linux-x86_64` - 64位 Linux
- `linux-x86` - 32位 Linux
- `linux-arm` - ARM Linux
- `darwin-x86` - macOS

**记住这个值**,后面会用到！

### 配置构建选项 (可选)

通常使用默认配置即可。如需自定义:

```bash
vim configure/CONFIG_SITE
```

常用配置项:
```makefile
# 交叉编译目标 (留空表示仅编译本机)
CROSS_COMPILER_TARGET_ARCHS =

# 共享库支持 (推荐YES)
SHARED_LIBRARIES = YES

# 静态链接 (推荐NO)
STATIC_BUILD = NO
```

## 编译 EPICS Base

### 开始编译

```bash
make clean uninstall  # 清理旧的编译(如果有)
make -j$(nproc)       # 并行编译,使用所有CPU核心
```

**预期输出**:
```
make -C ./configure install
...
make -C ./src install
...
make[1]: Leaving directory '/home/yourname/base-3.15.6'
```

**编译时间**:
- 单核: 15-20分钟
- 4核: 5-8分钟
- 8核: 3-5分钟

### 可能的警告 (可忽略)

```
warning: format '%ld' expects argument of type 'long int'...
```
这些警告不影响功能,可以忽略。

### 验证编译结果

```bash
# 检查可执行文件
ls -lh bin/linux-x86_64/

# 应该看到:
# softIoc, caget, caput, camonitor, cainfo, ...
```

```bash
# 检查库文件
ls -lh lib/linux-x86_64/

# 应该看到:
# libca.so, libCom.so, libdbCore.so, ...
```

## 配置环境变量

### Bash (Ubuntu/CentOS)

```bash
# 添加到 ~/.bashrc
cat >> ~/.bashrc << 'EOF'

# ========== EPICS Configuration ==========
export EPICS_BASE=$HOME/base-3.15.6
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 可选: Channel Access 配置
export EPICS_CA_ADDR_LIST="localhost"
export EPICS_CA_AUTO_ADDR_LIST=YES
# =========================================
EOF

# 使配置生效
source ~/.bashrc
```

### Zsh (macOS)

```bash
# 添加到 ~/.zshrc
cat >> ~/.zshrc << 'EOF'

# ========== EPICS Configuration ==========
export EPICS_BASE=$HOME/base-3.15.6
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}
# =========================================
EOF

source ~/.zshrc
```

## 验证安装

### 1. 检查环境变量

```bash
echo $EPICS_BASE
# 输出: /home/yourname/base-3.15.6

echo $EPICS_HOST_ARCH
# 输出: linux-x86_64

echo $PATH | grep EPICS
# 应该包含 EPICS_BASE 路径
```

### 2. 检查命令可用性

```bash
which softIoc
# 输出: /home/yourname/base-3.15.6/bin/linux-x86_64/softIoc

which caget
# 输出: /home/yourname/base-3.15.6/bin/linux-x86_64/caget
```

### 3. 测试 softIoc

```bash
softIoc -v
```

**预期输出**:
```
softIoc: Version R3.15.6
```

### 4. 运行第一个 IOC

创建测试数据库:
```bash
mkdir -p ~/epics-test
cd ~/epics-test
cat > test.db << 'EOF'
record(ai, "test:value") {
    field(VAL, "42")
}
EOF
```

创建启动脚本:
```bash
cat > st.cmd << 'EOF'
#!/usr/bin/env softIoc
dbLoadRecords("test.db")
iocInit()
dbl
EOF

chmod +x st.cmd
```

运行:
```bash
./st.cmd
```

**预期输出**:
```
Starting iocInit
...
iocRun: All initialization complete
test:value
epics>
```

在IOC Shell中测试:
```bash
epics> dbgf "test:value"
DBR_DOUBLE:          42
```

按 `Ctrl+C` 退出。

### 5. 测试 Channel Access

**终端1 - 运行 IOC**:
```bash
./st.cmd
```

**终端2 - 测试 CA 工具**:
```bash
caget test:value
# 输出: test:value 42

caput test:value 100
# 输出: Old : test:value 42
#       New : test:value 100

camonitor test:value
# 持续监控,按 Ctrl+C 停止
```

## 故障排查

### 问题 1: 编译失败 "make: command not found"

**解决**:
```bash
# Ubuntu
sudo apt install build-essential

# CentOS
sudo yum groupinstall "Development Tools"
```

### 问题 2: 编译失败 "readline/readline.h: No such file"

**解决**:
```bash
# Ubuntu
sudo apt install libreadline-dev

# CentOS
sudo yum install readline-devel
```

### 问题 3: 环境变量不生效

**检查**:
```bash
# 确保 source 了配置文件
source ~/.bashrc

# 或者重新登录终端
exit
# 重新打开终端
```

### 问题 4: "softIoc: command not found"

**解决**:
```bash
# 手动设置
export EPICS_BASE=$HOME/base-3.15.6
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 验证
which softIoc
```

### 问题 5: caget 找不到 PV

**检查**:
```bash
# 1. IOC 是否在运行
# 2. PV 名称是否正确
# 3. 网络配置

# 测试本地连接
ping localhost

# 检查 CA 配置
echo $EPICS_CA_ADDR_LIST
```

### 问题 6: 权限错误

```bash
# 如果遇到权限问题,确保 EPICS_BASE 在你的 home 目录
ls -ld $EPICS_BASE
# 应该显示你的用户名

# 如果需要,修复权限
chmod -R u+rwX $EPICS_BASE
```

## 下一步

✅ EPICS Base 安装完成！

现在你可以:
1. **克隆 BPMIOC** → [04-clone-and-compile.md](./04-clone-and-compile.md)
2. **学习 EPICS 基础** → [Part 2: 理解基础](../part2-understanding-basics/)
3. **查看 CA 工具** → [Part 2: 06-ca-tools.md](../part2-understanding-basics/06-ca-tools.md)

---

**提示**: 保存好本页面,以后在其他机器上安装时可以参考！
