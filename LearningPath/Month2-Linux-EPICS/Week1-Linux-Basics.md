# Week 1 - Linux 基础

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 掌握 Linux 基本命令，为 EPICS 开发做准备

**为什么要学 Linux？**
- EPICS IOC 通常运行在 Linux 系统上
- 需要用命令行编译、运行、调试 IOC
- Linux 是工业控制领域的标准平台

---

## 第1天：Linux 系统入门（2小时）

### 什么是 Linux？

Linux 是一个操作系统，类似于 Windows，但主要通过命令行（而不是图形界面）操作。

**为什么用命令行？**
- 更高效（不需要鼠标点击）
- 可以自动化（写脚本）
- 远程操作（SSH）
- 资源占用少

### 终端（Terminal）

终端是输入命令的地方：

```bash
username@hostname:~$
```

**解释**：
- `username`: 你的用户名
- `hostname`: 计算机名
- `~`: 当前目录（~ 表示家目录）
- `$`: 普通用户提示符（`#` 表示 root 用户）

### 第一个命令：pwd

```bash
$ pwd
/home/username
```

**pwd**: **P**rint **W**orking **D**irectory（打印当前工作目录）

### Linux 文件系统结构

```
/                  根目录
├── home/          用户家目录
│   └── username/  你的个人文件
├── usr/           用户程序
│   └── local/     本地安装的软件（EPICS 通常安装在这里）
├── etc/           配置文件
├── tmp/           临时文件
├── var/           变量文件（日志等）
└── opt/           可选软件包
```

### 绝对路径 vs 相对路径

**绝对路径**（从根目录 / 开始）：
```bash
/home/username/Documents/file.txt
```

**相对路径**（从当前目录开始）：
```bash
Documents/file.txt
./file.txt         # ./ 表示当前目录
../file.txt        # ../ 表示上级目录
```

### 实践：导航文件系统

```bash
# 查看当前目录
$ pwd
/home/username

# 查看当前目录内容
$ ls
Documents  Downloads  Pictures  Videos

# 进入 Documents 目录
$ cd Documents
$ pwd
/home/username/Documents

# 返回上级目录
$ cd ..
$ pwd
/home/username

# 直接回到家目录
$ cd
$ pwd
/home/username

# 或者使用 ~
$ cd ~
$ pwd
/home/username
```

### 练习1.1

完成以下操作：
1. 查看当前目录
2. 列出当前目录的内容
3. 进入 `/tmp` 目录
4. 返回家目录
5. 进入 `/usr/local` 目录并查看内容

---

## 第2天：文件和目录操作（2小时）

### ls 命令详解

```bash
# 基本用法
$ ls
file1.txt  file2.txt  mydir/

# 显示详细信息
$ ls -l
-rw-r--r-- 1 user group 1024 Dec 19 10:00 file1.txt
drwxr-xr-x 2 user group 4096 Dec 19 10:01 mydir/

# 显示隐藏文件（以 . 开头的文件）
$ ls -a
.  ..  .bashrc  .profile  file1.txt

# 组合选项
$ ls -la

# 人类可读的文件大小
$ ls -lh
-rw-r--r-- 1 user group 1.0K Dec 19 10:00 file1.txt
```

**ls -l 输出解释**：
```
-rw-r--r-- 1 user group 1024 Dec 19 10:00 file1.txt
│          │ │    │     │    │           └─ 文件名
│          │ │    │     │    └─ 修改时间
│          │ │    │     └─ 文件大小（字节）
│          │ │    └─ 所属组
│          │ └─ 所有者
│          └─ 链接数
└─ 权限
```

### 创建目录和文件

```bash
# 创建目录
$ mkdir mydir
$ ls
mydir/

# 创建多级目录
$ mkdir -p parent/child/grandchild
$ ls -R parent/
parent/:
child/

parent/child:
grandchild/

parent/child/grandchild:

# 创建空文件
$ touch file.txt
$ ls -l file.txt
-rw-r--r-- 1 user group 0 Dec 19 10:00 file.txt
```

### 复制文件和目录

```bash
# 复制文件
$ cp source.txt destination.txt

# 复制到目录
$ cp file.txt mydir/

# 复制并重命名
$ cp file.txt mydir/newname.txt

# 复制目录（需要 -r 递归）
$ cp -r dir1/ dir2/

# 保持权限和时间戳
$ cp -p original.txt copy.txt
```

### 移动和重命名

```bash
# 重命名文件（mv = move）
$ mv oldname.txt newname.txt

# 移动文件到目录
$ mv file.txt mydir/

# 移动并重命名
$ mv file.txt mydir/newname.txt

# 移动目录
$ mv dir1/ dir2/
```

### 删除文件和目录

```bash
# 删除文件
$ rm file.txt

# 删除多个文件
$ rm file1.txt file2.txt file3.txt

# 删除目录（需要 -r 递归）
$ rm -r mydir/

# 强制删除（不提示确认，危险！）
$ rm -f file.txt

# 删除前确认
$ rm -i file.txt
rm: remove regular file 'file.txt'? y

# 删除空目录
$ rmdir emptydir/
```

**警告**：`rm -rf /` 会删除整个系统！永远不要这样做！

### 实践：EPICS 项目目录结构

```bash
# 创建 EPICS 项目目录结构
$ cd ~
$ mkdir -p epics/ioc/BPM
$ cd epics/ioc/BPM
$ mkdir -p Db dbd src

$ tree ~/epics/ioc/BPM
BPM/
├── Db/     # 数据库文件
├── dbd/    # 设备/驱动定义
└── src/    # 源代码

# 创建示例文件
$ touch Db/BPMMonitor.db
$ touch dbd/BPMMonitor.dbd
$ touch src/devBPMMonitor.c
```

### 练习1.2

1. 在家目录创建 `projects/bpm_test` 目录结构
2. 在 `bpm_test` 下创建 `src`, `include`, `build` 目录
3. 创建文件 `src/main.c` 和 `include/bpm.h`
4. 将 `main.c` 复制到 `build` 目录
5. 列出整个目录树

---

## 第3天：查看和编辑文件（2小时）

### 查看文件内容

```bash
# cat: 显示整个文件
$ cat file.txt
Hello, EPICS!
This is a test file.

# 显示行号
$ cat -n file.txt
     1  Hello, EPICS!
     2  This is a test file.

# 查看多个文件
$ cat file1.txt file2.txt

# less: 分页查看（大文件）
$ less large_file.txt
# 按空格翻页，q 退出

# head: 查看文件开头
$ head file.txt          # 默认前10行
$ head -n 5 file.txt     # 前5行

# tail: 查看文件末尾
$ tail file.txt          # 默认后10行
$ tail -n 20 file.txt    # 后20行

# 实时监控文件（查看日志）
$ tail -f /var/log/ioc.log
```

### 文本编辑器：nano

**nano** 是最简单的命令行编辑器：

```bash
$ nano file.txt
```

**基本操作**：
- `Ctrl + O`: 保存（Write **O**ut）
- `Ctrl + X`: 退出（E**x**it）
- `Ctrl + K`: 剪切当前行
- `Ctrl + U`: 粘贴
- `Ctrl + W`: 搜索

**示例：创建一个简单的配置文件**：

```bash
$ nano bpm_config.txt
```

输入内容：
```
# BPM Configuration
num_channels=4
sample_rate=1000
threshold=0.5
```

按 `Ctrl+O` 保存，`Ctrl+X` 退出。

### 文本编辑器：vim（可选，较难）

**vim** 是强大但复杂的编辑器：

```bash
$ vim file.txt
```

**基本模式**：
- 普通模式（默认）：移动光标
- 插入模式：按 `i` 进入，可以编辑
- 命令模式：按 `:` 进入

**基本操作**：
- `i`: 进入插入模式
- `Esc`: 返回普通模式
- `:w`: 保存
- `:q`: 退出
- `:wq`: 保存并退出
- `:q!`: 不保存强制退出

### 重定向和管道

```bash
# 输出重定向（覆盖）
$ echo "Hello" > file.txt
$ cat file.txt
Hello

# 输出重定向（追加）
$ echo "World" >> file.txt
$ cat file.txt
Hello
World

# 输入重定向
$ cat < file.txt

# 管道（将一个命令的输出传给另一个命令）
$ cat file.txt | grep "Hello"
Hello

# 组合使用
$ ls -l | grep ".txt" | wc -l
5
```

### 实践：创建 EPICS 数据库文件

```bash
$ nano BPM.db
```

输入内容：
```
record(ai, "BPM:CH0:Amp") {
    field(DTYP, "BPMmonitor")
    field(INP, "@AMP:0 ch=0")
    field(SCAN, "I/O Intr")
}

record(ai, "BPM:CH1:Amp") {
    field(DTYP, "BPMmonitor")
    field(INP, "@AMP:0 ch=1")
    field(SCAN, "I/O Intr")
}
```

保存后查看：
```bash
$ cat BPM.db
$ head -n 5 BPM.db
$ tail -n 3 BPM.db
```

### 练习1.3

1. 创建文件 `channels.txt`，包含 4 行通道配置
2. 查看文件内容
3. 将文件内容追加到 `config.txt`
4. 查看 `config.txt` 的前 3 行
5. 使用管道统计 `config.txt` 的行数

---

## 第4天：查找和搜索（2小时）

### grep：文本搜索

```bash
# 基本搜索
$ grep "pattern" file.txt

# 忽略大小写
$ grep -i "pattern" file.txt

# 显示行号
$ grep -n "pattern" file.txt

# 递归搜索目录
$ grep -r "pattern" directory/

# 显示匹配的文件名
$ grep -l "pattern" *.txt

# 反向匹配（不包含 pattern 的行）
$ grep -v "pattern" file.txt

# 使用正则表达式
$ grep "^BPM" file.txt      # 以 BPM 开头的行
$ grep "\.txt$" file.txt    # 以 .txt 结尾的行
```

### 实践：搜索 EPICS 代码

```bash
# 搜索包含 "record" 的行
$ grep "record" BPM.db

# 搜索所有 .c 文件中的 "read_ai"
$ grep -n "read_ai" *.c

# 搜索目录中所有包含 "BPMmonitor" 的文件
$ grep -r "BPMmonitor" .

# 统计匹配的行数
$ grep -c "field" BPM.db
```

### find：查找文件

```bash
# 按名字查找
$ find . -name "*.c"
./src/devBPMMonitor.c
./src/driverWrapper.c

# 按类型查找（f=文件，d=目录）
$ find . -type f
$ find . -type d

# 按大小查找
$ find . -size +1M        # 大于 1MB
$ find . -size -100k      # 小于 100KB

# 按时间查找
$ find . -mtime -7        # 7天内修改的文件
$ find . -mtime +30       # 30天前修改的文件

# 组合条件
$ find . -name "*.c" -type f

# 执行命令
$ find . -name "*.txt" -exec cat {} \;
```

### 实践：查找 EPICS 文件

```bash
# 查找所有 .db 文件
$ find ~/epics -name "*.db"

# 查找所有 C 源文件
$ find ~/epics -name "*.c" -o -name "*.h"

# 查找所有目录
$ find ~/epics -type d

# 查找空文件
$ find ~/epics -type f -empty

# 查找并统计
$ find ~/epics -name "*.c" | wc -l
```

### 文件统计：wc

```bash
# 统计行数、单词数、字符数
$ wc file.txt
  10  50  300 file.txt
  │   │   │
  │   │   └─ 字符数
  │   └─ 单词数
  └─ 行数

# 只统计行数
$ wc -l file.txt

# 只统计单词数
$ wc -w file.txt

# 统计多个文件
$ wc *.txt
```

### sort 和 uniq

```bash
# 排序
$ sort file.txt

# 逆序排序
$ sort -r file.txt

# 按数字排序
$ sort -n numbers.txt

# 去重（必须先排序）
$ sort file.txt | uniq

# 统计重复次数
$ sort file.txt | uniq -c
      3 apple
      2 banana
      1 orange
```

### 练习1.4

假设你有以下文件结构：
```
epics/
├── ioc/
│   ├── BPM/
│   │   ├── Db/BPM.db
│   │   └── src/devBPM.c
│   └── Vacuum/
│       ├── Db/Vacuum.db
│       └── src/devVacuum.c
└── lib/
    └── common.c
```

完成以下任务：
1. 查找所有 `.c` 文件
2. 查找所有包含 "record" 的 `.db` 文件
3. 统计所有 `.c` 文件的总行数
4. 列出所有目录

---

## 第5天：权限和进程（2小时）

### 文件权限

```bash
$ ls -l file.txt
-rw-r--r-- 1 user group 1024 Dec 19 10:00 file.txt
```

**权限解释**：
```
-rw-r--r--
│││││││││
│││││││└└─ 其他用户：读
││││││└──── 其他用户：写（无）
│││││└───── 其他用户：执行（无）
││││└────── 组：读
│││└─────── 组：写（无）
││└──────── 组：执行（无）
│└───────── 所有者：读
└────────── 所有者：写
           （第一个字符：- 文件，d 目录，l 链接）
```

**权限数字表示**：
- `r` (read) = 4
- `w` (write) = 2
- `x` (execute) = 1

```
rwx = 4+2+1 = 7
rw- = 4+2+0 = 6
r-- = 4+0+0 = 4

-rw-r--r-- = 644
-rwxr-xr-x = 755
```

### chmod：修改权限

```bash
# 数字方式
$ chmod 644 file.txt        # rw-r--r--
$ chmod 755 script.sh       # rwxr-xr-x

# 符号方式
$ chmod u+x file.txt        # 给所有者添加执行权限
$ chmod g-w file.txt        # 移除组的写权限
$ chmod o+r file.txt        # 给其他用户添加读权限
$ chmod a+x script.sh       # 给所有人添加执行权限

# 递归修改
$ chmod -R 755 directory/
```

### 创建可执行脚本

```bash
$ nano hello.sh
```

输入内容：
```bash
#!/bin/bash
echo "Hello, EPICS!"
```

添加执行权限：
```bash
$ chmod +x hello.sh
$ ./hello.sh
Hello, EPICS!
```

### 进程管理

```bash
# 查看当前进程
$ ps
  PID TTY          TIME CMD
 1234 pts/0    00:00:00 bash
 5678 pts/0    00:00:00 ps

# 查看所有进程
$ ps aux
$ ps -ef

# 实时查看进程（类似 Windows 任务管理器）
$ top
# 按 q 退出

# 查找特定进程
$ ps aux | grep ioc
user  1234  0.1  0.5 12345 6789 ?  Ss  10:00  0:01 /usr/local/epics/bin/linux-x86_64/softIoc

# 杀死进程
$ kill 1234              # 正常终止
$ kill -9 1234           # 强制终止

# 按名字杀死进程
$ pkill ioc

# 后台运行程序
$ ./myprogram &

# 查看后台任务
$ jobs

# 将后台任务调到前台
$ fg 1
```

### 实践：运行 IOC

```bash
# 启动 IOC（后台运行）
$ cd ~/epics/ioc/BPM/iocBoot/iocBPM
$ ./st.cmd &
[1] 12345

# 查看 IOC 进程
$ ps aux | grep softIoc

# 查看 IOC 输出（如果有日志文件）
$ tail -f ioc.log

# 停止 IOC
$ pkill softIoc
```

### 练习1.5

1. 创建脚本 `start_ioc.sh`，内容为 `echo "Starting IOC..."`
2. 给脚本添加执行权限
3. 运行脚本
4. 查看当前所有进程
5. 创建一个后台运行的脚本

---

## 第6天：环境变量和 Shell 基础（2小时）

### 环境变量

```bash
# 查看所有环境变量
$ env

# 查看特定变量
$ echo $HOME
/home/username

$ echo $PATH
/usr/local/bin:/usr/bin:/bin

$ echo $USER
username

# 设置临时变量（只在当前 shell 有效）
$ MY_VAR="Hello"
$ echo $MY_VAR
Hello

# 导出变量（子进程可见）
$ export MY_VAR="Hello"

# 设置永久变量（添加到 ~/.bashrc）
$ nano ~/.bashrc
```

在 `~/.bashrc` 末尾添加：
```bash
export EPICS_BASE=/usr/local/epics/base
export EPICS_HOST_ARCH=linux-x86_64
export PATH=$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH
```

使配置生效：
```bash
$ source ~/.bashrc
```

### PATH 变量

```bash
# 查看 PATH
$ echo $PATH
/usr/local/bin:/usr/bin:/bin

# 添加目录到 PATH
$ export PATH=$PATH:/new/directory

# EPICS 常用 PATH 设置
$ export PATH=$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH
```

### 常用 Shell 快捷键

```bash
# 命令行编辑
Ctrl + A    # 移动到行首
Ctrl + E    # 移动到行尾
Ctrl + U    # 删除光标前的内容
Ctrl + K    # 删除光标后的内容
Ctrl + W    # 删除光标前的单词
Ctrl + L    # 清屏（等同于 clear）

# 历史命令
↑/↓         # 浏览历史命令
Ctrl + R    # 反向搜索历史命令
!!          # 执行上一条命令
!n          # 执行历史中第 n 条命令
!string     # 执行最近以 string 开头的命令

# Tab 补全
Tab         # 自动补全文件名/命令
Tab Tab     # 显示所有可能的补全
```

### 命令历史

```bash
# 查看历史命令
$ history
    1  ls
    2  cd /tmp
    3  pwd
  ...

# 查看最近 10 条
$ history 10

# 执行历史中的命令
$ !100        # 执行第 100 条命令
$ !!          # 执行上一条命令
$ !ls         # 执行最近的 ls 命令

# 清除历史
$ history -c
```

### 别名（Alias）

```bash
# 创建别名
$ alias ll='ls -lh'
$ alias la='ls -alh'

# 使用别名
$ ll
total 4.0K
-rw-r--r-- 1 user group 100 Dec 19 10:00 file.txt

# 查看所有别名
$ alias

# 删除别名
$ unalias ll

# 永久别名（添加到 ~/.bashrc）
$ nano ~/.bashrc
```

添加：
```bash
alias ll='ls -lh'
alias la='ls -alh'
alias ioc-start='cd ~/epics/ioc/BPM/iocBoot/iocBPM && ./st.cmd'
```

### 实践：设置 EPICS 环境

创建 `~/epics_env.sh`：

```bash
$ nano ~/epics_env.sh
```

内容：
```bash
#!/bin/bash
# EPICS Environment Setup

export EPICS_BASE=/usr/local/epics/base
export EPICS_HOST_ARCH=linux-x86_64
export PATH=$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH

echo "EPICS environment loaded:"
echo "  EPICS_BASE=$EPICS_BASE"
echo "  EPICS_HOST_ARCH=$EPICS_HOST_ARCH"
```

使用：
```bash
$ source ~/epics_env.sh
EPICS environment loaded:
  EPICS_BASE=/usr/local/epics/base
  EPICS_HOST_ARCH=linux-x86_64
```

### 练习1.6

1. 创建环境变量 `IOC_DIR`，指向你的 IOC 目录
2. 将 `IOC_DIR/bin` 添加到 PATH
3. 创建别名 `gotoioc`，快速进入 IOC 目录
4. 查看最近 20 条历史命令
5. 创建脚本自动设置所有 EPICS 环境变量

---

## 第7天：综合练习（2小时）

### 项目：EPICS 项目管理脚本

创建一个脚本来管理 EPICS IOC 项目。

**项目结构**：
```
~/epics-projects/
├── create_ioc.sh          # 创建新 IOC 项目
├── build_ioc.sh           # 编译 IOC
├── start_ioc.sh           # 启动 IOC
└── monitor_ioc.sh         # 监控 IOC
```

### create_ioc.sh

```bash
#!/bin/bash
# 创建新的 IOC 项目

echo "=== EPICS IOC Project Creator ==="

# 获取项目名
read -p "Enter IOC name: " IOC_NAME

if [ -z "$IOC_NAME" ]; then
    echo "Error: IOC name cannot be empty"
    exit 1
fi

# 创建目录结构
PROJECT_DIR=~/epics-projects/$IOC_NAME
echo "Creating project in $PROJECT_DIR"

mkdir -p $PROJECT_DIR/{Db,dbd,src,iocBoot/ioc${IOC_NAME}}

# 创建示例数据库文件
cat > $PROJECT_DIR/Db/${IOC_NAME}.db <<EOF
record(ai, "${IOC_NAME}:CH0:Value") {
    field(DESC, "Channel 0 Value")
    field(SCAN, "1 second")
}

record(ai, "${IOC_NAME}:CH1:Value") {
    field(DESC, "Channel 1 Value")
    field(SCAN, "1 second")
}
EOF

# 创建 README
cat > $PROJECT_DIR/README.md <<EOF
# $IOC_NAME IOC

Created: $(date)

## Directory Structure
- Db/: Database files
- dbd/: Device/Driver definitions
- src/: Source code
- iocBoot/: Boot scripts

## Usage
1. Build: cd $PROJECT_DIR && make
2. Run: cd iocBoot/ioc${IOC_NAME} && ./st.cmd
EOF

echo "Project created successfully!"
echo "Location: $PROJECT_DIR"
ls -lR $PROJECT_DIR
```

### start_ioc.sh

```bash
#!/bin/bash
# 启动 IOC

IOC_NAME=$1

if [ -z "$IOC_NAME" ]; then
    echo "Usage: $0 <IOC_NAME>"
    exit 1
fi

IOC_DIR=~/epics-projects/$IOC_NAME/iocBoot/ioc${IOC_NAME}

if [ ! -d "$IOC_DIR" ]; then
    echo "Error: IOC directory not found: $IOC_DIR"
    exit 1
fi

echo "Starting IOC: $IOC_NAME"
cd $IOC_DIR

# 检查 st.cmd 是否存在
if [ ! -f "st.cmd" ]; then
    echo "Error: st.cmd not found"
    exit 1
fi

# 启动 IOC（后台运行）
./st.cmd > ioc.log 2>&1 &
IOC_PID=$!

echo "IOC started with PID: $IOC_PID"
echo "Log file: $IOC_DIR/ioc.log"
```

### monitor_ioc.sh

```bash
#!/bin/bash
# 监控 IOC 状态

echo "=== IOC Monitor ==="

# 查找所有 IOC 进程
IOC_PIDS=$(ps aux | grep "st.cmd" | grep -v grep | awk '{print $2}')

if [ -z "$IOC_PIDS" ]; then
    echo "No IOC processes found"
    exit 0
fi

echo "Running IOCs:"
ps aux | grep "st.cmd" | grep -v grep

echo ""
echo "Process details:"
for PID in $IOC_PIDS; do
    echo "PID: $PID"
    ps -p $PID -o pid,ppid,user,%cpu,%mem,etime,cmd
    echo ""
done
```

### 综合练习

使用上述脚本完成以下任务：

```bash
# 1. 创建脚本目录
$ mkdir -p ~/bin
$ cd ~/bin

# 2. 创建所有脚本
$ nano create_ioc.sh
# ... 粘贴内容 ...

# 3. 添加执行权限
$ chmod +x *.sh

# 4. 添加到 PATH
$ echo 'export PATH=$HOME/bin:$PATH' >> ~/.bashrc
$ source ~/.bashrc

# 5. 创建新 IOC
$ create_ioc.sh
Enter IOC name: TestBPM

# 6. 查看创建的项目
$ ls -lR ~/epics-projects/TestBPM

# 7. 启动 IOC（如果已安装 EPICS）
$ start_ioc.sh TestBPM

# 8. 监控 IOC
$ monitor_ioc.sh
```

### 练习题

**练习1：日志分析脚本**

创建脚本分析 IOC 日志文件：

```bash
#!/bin/bash
# analyze_log.sh

LOG_FILE=$1

if [ -z "$LOG_FILE" ]; then
    echo "Usage: $0 <log_file>"
    exit 1
fi

echo "=== Log Analysis ==="
echo "File: $LOG_FILE"
echo "Total lines: $(wc -l < $LOG_FILE)"
echo ""

echo "Error count:"
grep -c "ERROR" $LOG_FILE

echo ""
echo "Warning count:"
grep -c "WARNING" $LOG_FILE

echo ""
echo "Last 5 errors:"
grep "ERROR" $LOG_FILE | tail -5
```

**练习2：备份脚本**

```bash
#!/bin/bash
# backup_ioc.sh

IOC_NAME=$1
BACKUP_DIR=~/backups

if [ -z "$IOC_NAME" ]; then
    echo "Usage: $0 <IOC_NAME>"
    exit 1
fi

SOURCE_DIR=~/epics-projects/$IOC_NAME
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE=$BACKUP_DIR/${IOC_NAME}_${TIMESTAMP}.tar.gz

mkdir -p $BACKUP_DIR

echo "Backing up $IOC_NAME..."
tar -czf $BACKUP_FILE -C ~/epics-projects $IOC_NAME

echo "Backup created: $BACKUP_FILE"
echo "Size: $(du -h $BACKUP_FILE | cut -f1)"
```

---

## 本周知识点总结

### 基本命令

| 命令 | 用途 | 示例 |
|------|------|------|
| `pwd` | 显示当前目录 | `pwd` |
| `ls` | 列出文件 | `ls -lah` |
| `cd` | 切换目录 | `cd /path/to/dir` |
| `mkdir` | 创建目录 | `mkdir -p parent/child` |
| `touch` | 创建文件 | `touch file.txt` |
| `cp` | 复制 | `cp -r src/ dest/` |
| `mv` | 移动/重命名 | `mv old.txt new.txt` |
| `rm` | 删除 | `rm -rf directory/` |
| `cat` | 查看文件 | `cat file.txt` |
| `grep` | 搜索文本 | `grep "pattern" file.txt` |
| `find` | 查找文件 | `find . -name "*.c"` |
| `chmod` | 修改权限 | `chmod 755 script.sh` |
| `ps` | 查看进程 | `ps aux` |
| `kill` | 终止进程 | `kill -9 1234` |

### 文件权限

```
-rwxr-xr-x
 ││││││││└─ 其他用户执行
 │││││││└── 其他用户写
 ││││││└─── 其他用户读
 │││││└──── 组执行
 ││││└───── 组写
 │││└────── 组读
 ││└─────── 所有者执行
 │└──────── 所有者写
 └───────── 所有者读
```

### 环境变量

```bash
export EPICS_BASE=/usr/local/epics/base
export EPICS_HOST_ARCH=linux-x86_64
export PATH=$EPICS_BASE/bin/$EPICS_HOST_ARCH:$PATH
```

### 常用技巧

```bash
# Tab 补全
$ cd /usr/lo<Tab>  → cd /usr/local/

# 历史命令
$ !ls              → 执行最近的 ls 命令

# 管道
$ ls -l | grep ".c" | wc -l

# 重定向
$ command > output.txt
$ command >> append.txt
```

---

## 自我检查清单

完成本周学习后，你应该能够：

- [ ] 在文件系统中导航（cd, pwd, ls）
- [ ] 创建、复制、移动、删除文件和目录
- [ ] 查看和编辑文本文件（cat, nano）
- [ ] 搜索文件和文本（find, grep）
- [ ] 理解和修改文件权限（chmod）
- [ ] 管理进程（ps, kill）
- [ ] 设置环境变量
- [ ] 编写简单的 Shell 脚本

---

## 下一步

完成本周学习后，继续学习：
- **Week 2**: 编译工具和 Makefile
- **Week 3**: EPICS Base 安装
- **Week 4**: 创建第一个 IOC

继续加油！🚀
