# Week 3 - EPICS Base 安装和配置

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 安装 EPICS Base，理解 EPICS 架构

---

## Day 1-2: EPICS Base 安装（4小时）

### 下载 EPICS Base

```bash
$ cd ~
$ mkdir epics
$ cd epics
$ wget https://epics-controls.org/download/base/base-7.0.7.tar.gz
$ tar -xzf base-7.0.7.tar.gz
$ mv base-7.0.7 base
$ cd base
```

### 编译 EPICS Base

```bash
# 设置环境变量
$ export EPICS_BASE=$HOME/epics/base
$ export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
$ export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}

# 编译（需要20-30分钟）
$ make -j4
```

### 配置环境变量

在 `~/.bashrc` 添加：

```bash
# EPICS Environment
export EPICS_BASE=$HOME/epics/base
export EPICS_HOST_ARCH=$(${EPICS_BASE}/startup/EpicsHostArch)
export PATH=${EPICS_BASE}/bin/${EPICS_HOST_ARCH}:${PATH}
```

加载配置：
```bash
$ source ~/.bashrc
```

### 验证安装

```bash
$ which softIoc
/home/user/epics/base/bin/linux-x86_64/softIoc

$ softIoc -h
Usage: softIoc [-D softIoc.dbd] [-h] [-S] [-a ascf]
...
```

---

## Day 3-4: EPICS 架构理解（4小时）

### EPICS 目录结构

```
$EPICS_BASE/
├── bin/              # 可执行文件
│   └── linux-x86_64/
│       ├── softIoc   # IOC 可执行文件
│       ├── caget     # CA 客户端工具
│       ├── caput
│       └── camonitor
├── lib/              # 库文件
│   └── linux-x86_64/
│       ├── libCom.so
│       ├── libca.so
│       └── libdbCore.so
├── include/          # 头文件
├── dbd/              # DBD 文件
└── db/               # 数据库文件
```

### Channel Access 工具

```bash
# caget - 读取 PV
$ caget PV_NAME

# caput - 写入 PV
$ caput PV_NAME value

# camonitor - 监控 PV
$ camonitor PV_NAME

# cainfo - PV 信息
$ cainfo PV_NAME
```

---

## Day 5-7: 第一个示例 IOC（6小时）

### 创建简单 IOC

```bash
$ cd $EPICS_BASE
$ bin/$EPICS_HOST_ARCH/makeBaseApp.pl -t example myExample
$ bin/$EPICS_HOST_ARCH/makeBaseApp.pl -i -t example myExample
```

### 编译和运行

```bash
$ cd myExample
$ make
$ cd iocBoot/iocmyExample
$ ./st.cmd
```

### 测试 IOC

```bash
# 在另一个终端
$ caget myExample:ai1
$ caput myExample:ao1 123
$ camonitor myExample:ai1
```

---

## 练习

1. 安装 EPICS Base
2. 配置环境变量
3. 创建并运行示例 IOC
4. 使用 CA 工具测试 PV

继续下周学习！
