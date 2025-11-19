# Week 2 - 编译工具和 Makefile

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 掌握 C 程序编译流程和 Makefile 使用，为编译 EPICS 做准备

**为什么重要？**
- EPICS 使用 Make 系统构建
- 需要理解编译过程才能调试问题
- Makefile 是 EPICS 项目的核心

---

## 第1天：GCC 编译器基础（2小时）

### 编译过程

C 程序从源码到可执行文件的过程：

```
源文件(.c) → 预处理 → 编译 → 汇编 → 链接 → 可执行文件
  ↓           ↓        ↓      ↓      ↓        ↓
hello.c    hello.i  hello.s hello.o hello    可执行
```

### 最简单的编译

```bash
# 创建源文件
$ nano hello.c
```

```c
#include <stdio.h>

int main() {
    printf("Hello, EPICS!\n");
    return 0;
}
```

```bash
# 编译
$ gcc hello.c -o hello

# 运行
$ ./hello
Hello, EPICS!
```

### GCC 编译选项

```bash
# -o: 指定输出文件名
$ gcc hello.c -o hello

# -Wall: 显示所有警告
$ gcc -Wall hello.c -o hello

# -g: 包含调试信息
$ gcc -g hello.c -o hello

# -O2: 优化级别2
$ gcc -O2 hello.c -o hello

# 组合使用
$ gcc -Wall -g -O2 hello.c -o hello
```

### 分步编译

```bash
# 1. 预处理（展开宏、包含头文件）
$ gcc -E hello.c -o hello.i

# 2. 编译（生成汇编代码）
$ gcc -S hello.c -o hello.s

# 3. 汇编（生成目标文件）
$ gcc -c hello.c -o hello.o

# 4. 链接（生成可执行文件）
$ gcc hello.o -o hello
```

### 多文件编译

创建项目：

**main.c**:
```c
#include <stdio.h>
#include "math_utils.h"

int main() {
    int result = add(10, 20);
    printf("10 + 20 = %d\n", result);
    return 0;
}
```

**math_utils.h**:
```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

int add(int a, int b);
int subtract(int a, int b);

#endif
```

**math_utils.c**:
```c
#include "math_utils.h"

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}
```

编译：
```bash
# 方法1：一次编译所有文件
$ gcc main.c math_utils.c -o calculator

# 方法2：分别编译再链接
$ gcc -c main.c -o main.o
$ gcc -c math_utils.c -o math_utils.o
$ gcc main.o math_utils.o -o calculator

# 运行
$ ./calculator
10 + 20 = 30
```

### 包含路径和库

```bash
# -I: 指定头文件搜索路径
$ gcc -I/path/to/headers main.c -o main

# -L: 指定库文件搜索路径
$ gcc -L/path/to/libs main.c -o main

# -l: 链接库（-lm 表示链接 libm.so 或 libm.a）
$ gcc main.c -lm -o main

# EPICS 典型编译命令
$ gcc -I$(EPICS_BASE)/include \
      -L$(EPICS_BASE)/lib/$(EPICS_HOST_ARCH) \
      -lCom -lca \
      myapp.c -o myapp
```

### 练习2.1

创建一个 BPM 数据处理程序：

**bpm_data.h**:
```c
#ifndef BPM_DATA_H
#define BPM_DATA_H

typedef struct {
    int channel;
    float amplitude;
} BPMData;

void print_bpm_data(BPMData *data);
float calculate_average(BPMData *data, int count);

#endif
```

**bpm_data.c**:
```c
#include <stdio.h>
#include "bpm_data.h"

void print_bpm_data(BPMData *data) {
    printf("CH%d: %.3f V\n", data->channel, data->amplitude);
}

float calculate_average(BPMData *data, int count) {
    float sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += data[i].amplitude;
    }
    return sum / count;
}
```

**main.c**:
```c
#include <stdio.h>
#include "bpm_data.h"

int main() {
    BPMData channels[4] = {
        {0, 0.123},
        {1, 0.456},
        {2, 0.789},
        {3, 0.234}
    };

    for (int i = 0; i < 4; i++) {
        print_bpm_data(&channels[i]);
    }

    printf("Average: %.3f V\n", calculate_average(channels, 4));

    return 0;
}
```

编译并运行。

---

## 第2天：Makefile 入门（2小时）

### 为什么需要 Makefile？

**手动编译的问题**：
```bash
$ gcc -c main.c
$ gcc -c math_utils.c
$ gcc -c file1.c
$ gcc -c file2.c
...  # 如果有100个文件呢？
$ gcc main.o math_utils.o file1.o file2.o ... -o program
```

**使用 Makefile**：
```bash
$ make
```

### 第一个 Makefile

**Makefile**:
```makefile
# 目标: 依赖
#     命令

hello: hello.c
	gcc hello.c -o hello

clean:
	rm -f hello
```

**重要**：命令前必须用 Tab 键（不能用空格）！

使用：
```bash
$ make
gcc hello.c -o hello

$ make clean
rm -f hello
```

### Makefile 基本语法

```makefile
target: dependencies
	command1
	command2
	...
```

示例：
```makefile
calculator: main.o math_utils.o
	gcc main.o math_utils.o -o calculator

main.o: main.c math_utils.h
	gcc -c main.c

math_utils.o: math_utils.c math_utils.h
	gcc -c math_utils.c

clean:
	rm -f *.o calculator
```

使用：
```bash
$ make          # 编译（默认第一个目标）
$ make clean    # 清理
```

### Makefile 变量

```makefile
# 定义变量
CC = gcc
CFLAGS = -Wall -g
OBJS = main.o math_utils.o
TARGET = calculator

# 使用变量
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

math_utils.o: math_utils.c
	$(CC) $(CFLAGS) -c math_utils.c

clean:
	rm -f $(OBJS) $(TARGET)
```

### 自动变量

```makefile
CC = gcc
CFLAGS = -Wall -g

calculator: main.o math_utils.o
	$(CC) $^ -o $@
	# $^ = 所有依赖 (main.o math_utils.o)
	# $@ = 目标 (calculator)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	# $< = 第一个依赖 (%.c)
	# $@ = 目标 (%.o)

clean:
	rm -f *.o calculator
```

### 常用自动变量

| 变量 | 含义 | 示例 |
|------|------|------|
| `$@` | 目标文件名 | `calculator` |
| `$<` | 第一个依赖文件 | `main.c` |
| `$^` | 所有依赖文件 | `main.o math_utils.o` |
| `$?` | 比目标新的依赖 | `main.o` (如果它更新了) |

### 练习2.2

为 BPM 数据处理程序创建 Makefile：

```makefile
CC = gcc
CFLAGS = -Wall -g
OBJS = main.o bpm_data.o
TARGET = bpm_monitor

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
```

---

## 第3天：Makefile 进阶（2小时）

### 模式规则

```makefile
# 通用规则：所有 .c 文件编译成 .o 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 等价于：
# main.o: main.c
#     $(CC) $(CFLAGS) -c main.c -o main.o
# math_utils.o: math_utils.c
#     $(CC) $(CFLAGS) -c math_utils.c -o math_utils.o
```

### 伪目标

```makefile
.PHONY: all clean install

all: program1 program2 program3

clean:
	rm -f *.o program1 program2 program3

install: all
	cp program1 /usr/local/bin/
	cp program2 /usr/local/bin/
```

### 条件语句

```makefile
DEBUG = 1

ifeq ($(DEBUG), 1)
    CFLAGS = -Wall -g -DDEBUG
else
    CFLAGS = -Wall -O2
endif

program: main.o
	$(CC) $(CFLAGS) main.o -o program
```

### 包含其他 Makefile

```makefile
# 主 Makefile
include config.mk
include rules.mk

program: $(OBJS)
	$(CC) $(OBJS) -o program
```

**config.mk**:
```makefile
CC = gcc
CFLAGS = -Wall -g
LIBS = -lm
```

**rules.mk**:
```makefile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

### EPICS 风格的 Makefile

```makefile
TOP = ../..
include $(TOP)/configure/CONFIG

# 程序名
PROD_HOST += myIOC

# 源文件
myIOC_SRCS += main.c
myIOC_SRCS += devMyDevice.c
myIOC_SRCS += drvMyDriver.c

# 数据库文件
DB += myApp.db

# DBD 文件
DBD += myApp.dbd

# 链接库
myIOC_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
```

### 实践：多目录项目

```
project/
├── Makefile
├── src/
│   ├── main.c
│   ├── bpm_data.c
│   └── hardware.c
├── include/
│   ├── bpm_data.h
│   └── hardware.h
└── build/
```

**Makefile**:
```makefile
CC = gcc
CFLAGS = -Wall -g -Iinclude
SRCDIR = src
BUILDDIR = build
OBJS = $(BUILDDIR)/main.o $(BUILDDIR)/bpm_data.o $(BUILDDIR)/hardware.o
TARGET = $(BUILDDIR)/bpm_monitor

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
```

### 练习2.3

创建一个完整的项目 Makefile，支持：
1. 调试版本和发布版本
2. 自动创建 build 目录
3. 安装目标
4. 清理目标

---

## 第4天：库的创建和使用（2小时）

### 静态库 (.a)

**创建静态库**：

```bash
# 1. 编译源文件为目标文件
$ gcc -c math_utils.c -o math_utils.o

# 2. 创建静态库
$ ar rcs libmath.a math_utils.o

# 3. 查看库内容
$ ar -t libmath.a
math_utils.o
```

**使用静态库**：

```bash
# 方法1：直接指定库文件
$ gcc main.c libmath.a -o calculator

# 方法2：使用 -L 和 -l
$ gcc main.c -L. -lmath -o calculator
#                │  └─ 库名（去掉 lib 前缀和 .a 后缀）
#                └─ 库搜索路径（. 表示当前目录）
```

**Makefile 示例**：

```makefile
CC = gcc
AR = ar
CFLAGS = -Wall -g

# 静态库
LIBNAME = libmath.a
LIBOBJS = math_utils.o

# 应用程序
PROGRAM = calculator
PROGOBJS = main.o

all: $(LIBNAME) $(PROGRAM)

$(LIBNAME): $(LIBOBJS)
	$(AR) rcs $(LIBNAME) $(LIBOBJS)

$(PROGRAM): $(PROGOBJS) $(LIBNAME)
	$(CC) $(PROGOBJS) -L. -lmath -o $(PROGRAM)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIBNAME) $(PROGRAM)

.PHONY: all clean
```

### 动态库 (.so)

**创建动态库**：

```bash
# 1. 编译位置无关代码 (Position Independent Code)
$ gcc -fPIC -c math_utils.c -o math_utils.o

# 2. 创建动态库
$ gcc -shared -o libmath.so math_utils.o

# 3. 查看库信息
$ file libmath.so
libmath.so: ELF 64-bit LSB shared object...
```

**使用动态库**：

```bash
# 编译
$ gcc main.c -L. -lmath -o calculator

# 运行时需要指定库路径
$ LD_LIBRARY_PATH=. ./calculator
```

**Makefile 示例**：

```makefile
CC = gcc
CFLAGS = -Wall -g -fPIC

# 动态库
LIBNAME = libmath.so
LIBOBJS = math_utils.o

# 应用程序
PROGRAM = calculator
PROGOBJS = main.o

all: $(LIBNAME) $(PROGRAM)

$(LIBNAME): $(LIBOBJS)
	$(CC) -shared -o $(LIBNAME) $(LIBOBJS)

$(PROGRAM): $(PROGOBJS) $(LIBNAME)
	$(CC) $(PROGOBJS) -L. -lmath -o $(PROGRAM)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIBNAME) $(PROGRAM)

run: $(PROGRAM)
	LD_LIBRARY_PATH=. ./$(PROGRAM)

.PHONY: all clean run
```

### 静态库 vs 动态库

| 特性 | 静态库 (.a) | 动态库 (.so) |
|------|-------------|--------------|
| 链接时间 | 编译时 | 运行时 |
| 可执行文件大小 | 大 | 小 |
| 内存占用 | 每个程序独立拷贝 | 多个程序共享 |
| 更新 | 需要重新编译 | 只需替换库文件 |
| 部署 | 简单（单个文件） | 复杂（需要库文件） |

### EPICS 中的库

EPICS 使用动态库：

```bash
$ ls $EPICS_BASE/lib/$EPICS_HOST_ARCH/
libCom.so          # 通用功能
libca.so           # Channel Access
libdbCore.so       # 数据库核心
libdbRecStd.so     # 标准 Record
libdbStaticHost.so # 静态数据库
...
```

### 练习2.4

创建 BPM 库：

**libbpm.so**:
- `bpm_init()`
- `bpm_read(int channel)`
- `bpm_write(int channel, float value)`

创建测试程序使用这个库。

---

## 第5天：调试工具 GDB（2小时）

### GDB 基础

**编译时包含调试信息**：

```bash
$ gcc -g program.c -o program
```

**启动 GDB**：

```bash
$ gdb program
(gdb)
```

### 基本命令

```gdb
(gdb) run                  # 运行程序 (简写: r)
(gdb) break main           # 在 main 设置断点 (简写: b)
(gdb) break 10             # 在第10行设置断点
(gdb) break file.c:20      # 在 file.c 第20行设置断点

(gdb) next                 # 单步执行（不进入函数）(简写: n)
(gdb) step                 # 单步执行（进入函数）(简写: s)
(gdb) continue             # 继续运行 (简写: c)
(gdb) finish               # 运行到函数返回

(gdb) print variable       # 打印变量值 (简写: p)
(gdb) print *pointer       # 打印指针指向的值
(gdb) print array[0]       # 打印数组元素

(gdb) info breakpoints     # 查看所有断点
(gdb) delete 1             # 删除断点1
(gdb) delete               # 删除所有断点

(gdb) backtrace            # 查看调用栈 (简写: bt)
(gdb) frame 2              # 切换到栈帧2

(gdb) quit                 # 退出 (简写: q)
```

### 调试示例

**bug_program.c**:
```c
#include <stdio.h>

int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

int main() {
    int num = 5;
    int result = factorial(num);
    printf("%d! = %d\n", num, result);
    return 0;
}
```

调试会话：

```bash
$ gcc -g bug_program.c -o bug_program
$ gdb bug_program

(gdb) break main
Breakpoint 1 at 0x...: file bug_program.c, line 11.

(gdb) run
Starting program: ./bug_program
Breakpoint 1, main () at bug_program.c:11
11          int num = 5;

(gdb) next
12          int result = factorial(num);

(gdb) step
factorial (n=5) at bug_program.c:4
4           int result = 1;

(gdb) print n
$1 = 5

(gdb) next
5           for (int i = 1; i <= n; i++) {

(gdb) next
6               result *= i;

(gdb) print result
$2 = 1

(gdb) print i
$3 = 1

(gdb) continue
Continuing.
5! = 120
[Inferior 1 (process ...) exited normally]

(gdb) quit
```

### 调试崩溃程序

**crash.c**:
```c
#include <stdio.h>

int main() {
    int *p = NULL;
    *p = 10;  // 段错误！
    return 0;
}
```

调试：

```bash
$ gcc -g crash.c -o crash
$ gdb crash

(gdb) run
Starting program: ./crash

Program received signal SIGSEGV, Segmentation fault.
0x... in main () at crash.c:5
5           *p = 10;

(gdb) backtrace
#0  0x... in main () at crash.c:5

(gdb) print p
$1 = (int *) 0x0

(gdb) list
1       #include <stdio.h>
2
3       int main() {
4           int *p = NULL;
5           *p = 10;  // 段错误！
6           return 0;
7       }
```

### 观察点（Watchpoint）

```gdb
(gdb) watch variable      # 当变量值改变时中断
(gdb) rwatch variable     # 当变量被读取时中断
(gdb) awatch variable     # 当变量被访问时中断
```

示例：

```c
int main() {
    int count = 0;
    for (int i = 0; i < 10; i++) {
        count += i;
    }
    return 0;
}
```

```gdb
(gdb) break main
(gdb) run
(gdb) watch count
Hardware watchpoint 2: count

(gdb) continue
Hardware watchpoint 2: count
Old value = 0
New value = 1
```

### 练习2.5

调试以下程序，找出错误：

```c
#include <stdio.h>

float calculate_average(int *arr, int size) {
    int sum = 0;
    for (int i = 0; i <= size; i++) {  // 错误：应该是 i < size
        sum += arr[i];
    }
    return (float)sum / size;
}

int main() {
    int data[] = {10, 20, 30, 40, 50};
    float avg = calculate_average(data, 5);
    printf("Average: %.2f\n", avg);
    return 0;
}
```

---

## 第6天：Git 版本控制（2小时）

### Git 基础

**配置 Git**：

```bash
$ git config --global user.name "Your Name"
$ git config --global user.email "your.email@example.com"
```

**创建仓库**：

```bash
$ mkdir myproject
$ cd myproject
$ git init
Initialized empty Git repository in /path/to/myproject/.git/
```

### 基本工作流程

```bash
# 1. 创建/修改文件
$ echo "# My Project" > README.md

# 2. 查看状态
$ git status
On branch master
Untracked files:
  README.md

# 3. 添加到暂存区
$ git add README.md

# 4. 提交
$ git commit -m "Add README"

# 5. 查看历史
$ git log
commit abc123...
Author: Your Name <your.email@example.com>
Date:   ...

    Add README
```

### 常用命令

```bash
# 查看状态
$ git status

# 添加文件
$ git add file.txt          # 添加单个文件
$ git add *.c               # 添加所有 .c 文件
$ git add .                 # 添加所有文件

# 提交
$ git commit -m "Commit message"
$ git commit -am "Add and commit"  # 添加并提交已跟踪文件

# 查看差异
$ git diff                  # 工作区 vs 暂存区
$ git diff --staged         # 暂存区 vs 仓库

# 查看历史
$ git log
$ git log --oneline         # 简洁输出
$ git log --graph           # 图形化

# 撤销修改
$ git checkout -- file.txt  # 撤销工作区修改
$ git reset HEAD file.txt   # 取消暂存

# 分支操作
$ git branch                # 查看分支
$ git branch feature        # 创建分支
$ git checkout feature      # 切换分支
$ git checkout -b feature   # 创建并切换

# 合并分支
$ git checkout master
$ git merge feature

# 删除分支
$ git branch -d feature
```

### .gitignore

创建 `.gitignore` 文件：

```bash
# 忽略模式

# 目标文件
*.o
*.a
*.so

# 可执行文件
*.exe
a.out

# 编辑器临时文件
*~
*.swp
.vscode/

# Build 目录
build/
bin/

# EPICS 编译产物
O.*/
db/
dbd/
```

### 实践：管理 EPICS 项目

```bash
# 创建项目
$ mkdir BPMMonitor
$ cd BPMMonitor
$ git init

# 创建文件结构
$ mkdir -p {src,include,Db,dbd}
$ touch src/devBPM.c include/devBPM.h Db/BPM.db

# 创建 .gitignore
$ cat > .gitignore <<EOF
*.o
*.a
*.so
O.*/
EOF

# 添加并提交
$ git add .
$ git commit -m "Initial project structure"

# 修改文件
$ nano src/devBPM.c
# ... 编辑 ...

# 查看修改
$ git diff

# 提交更改
$ git add src/devBPM.c
$ git commit -m "Add BPM device support skeleton"

# 查看历史
$ git log --oneline
```

### 练习2.6

1. 创建一个 Git 仓库
2. 创建 C 程序文件
3. 创建适当的 .gitignore
4. 提交初始版本
5. 修改程序
6. 提交更改
7. 查看历史

---

## 第7天：综合练习（2小时）

### 项目：完整的 BPM 数据采集系统

创建一个包含完整构建系统的项目。

**项目结构**：

```
BPMDataAcq/
├── .git/
├── .gitignore
├── Makefile
├── README.md
├── include/
│   ├── bpm_data.h
│   └── hardware.h
├── src/
│   ├── main.c
│   ├── bpm_data.c
│   └── hardware.c
├── lib/
└── build/
```

**Makefile**:

```makefile
# Project Configuration
PROJECT = BPMDataAcq
VERSION = 1.0.0

# Compiler Settings
CC = gcc
AR = ar
CFLAGS = -Wall -Werror -g -Iinclude
LDFLAGS = -Llib
LIBS = -lm

# Debug/Release
ifdef DEBUG
    CFLAGS += -DDEBUG -O0
else
    CFLAGS += -O2
endif

# Directories
SRCDIR = src
INCDIR = include
LIBDIR = lib
BUILDDIR = build

# Files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))
LIBSOURCES = $(filter-out $(SRCDIR)/main.c,$(SOURCES))
LIBOBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(LIBSOURCES))

# Targets
PROGRAM = $(BUILDDIR)/$(PROJECT)
LIBRARY = $(LIBDIR)/lib$(PROJECT).a
SHARED_LIB = $(LIBDIR)/lib$(PROJECT).so

# Main Target
all: $(BUILDDIR) $(LIBDIR) $(PROGRAM)

# Create Directories
$(BUILDDIR) $(LIBDIR):
	@mkdir -p $@

# Build Program
$(PROGRAM): $(OBJECTS)
	@echo "Linking $@"
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

# Build Object Files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Build Static Library
lib: $(LIBDIR) $(LIBRARY)

$(LIBRARY): $(LIBOBJECTS)
	@echo "Creating static library $@"
	$(AR) rcs $@ $^

# Build Shared Library
shared: $(LIBDIR) $(SHARED_LIB)

$(SHARED_LIB): CFLAGS += -fPIC
$(SHARED_LIB): $(LIBOBJECTS)
	@echo "Creating shared library $@"
	$(CC) -shared -o $@ $^

# Clean
clean:
	@echo "Cleaning..."
	rm -rf $(BUILDDIR) $(LIBDIR)

# Run
run: $(PROGRAM)
	@echo "Running $(PROJECT) v$(VERSION)"
	@$(PROGRAM)

# Install
PREFIX = /usr/local
install: $(PROGRAM)
	install -D $(PROGRAM) $(PREFIX)/bin/$(PROJECT)
	install -D -m 644 $(INCDIR)/*.h $(PREFIX)/include/$(PROJECT)/

# Uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(PROJECT)
	rm -rf $(PREFIX)/include/$(PROJECT)

# Dependencies
-include $(OBJECTS:.o=.d)

$(BUILDDIR)/%.d: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	@$(CC) $(CFLAGS) -MM -MT $(BUILDDIR)/$*.o $< > $@

# Help
help:
	@echo "$(PROJECT) v$(VERSION) Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build program (default)"
	@echo "  lib      - Build static library"
	@echo "  shared   - Build shared library"
	@echo "  clean    - Remove build files"
	@echo "  run      - Build and run program"
	@echo "  install  - Install program"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1  - Build with debug symbols"

.PHONY: all lib shared clean run install uninstall help
```

**README.md**:

```markdown
# BPM Data Acquisition System

Version 1.0.0

## Description

A complete BPM data acquisition and processing system.

## Building

```bash
# Normal build
make

# Debug build
make DEBUG=1

# Build library
make lib

# Build shared library
make shared
```

## Usage

```bash
# Run
make run

# Or directly
./build/BPMDataAcq
```

## Installation

```bash
sudo make install
```

## Project Structure

- `src/` - Source files
- `include/` - Header files
- `build/` - Build output
- `lib/` - Libraries

## License

MIT
```

**.gitignore**:

```
# Build files
build/
lib/
*.o
*.a
*.so

# Editor files
*~
*.swp
.vscode/
.idea/

# OS files
.DS_Store
Thumbs.db
```

### 实践步骤

```bash
# 1. 创建项目
$ mkdir BPMDataAcq
$ cd BPMDataAcq
$ git init

# 2. 创建目录结构
$ mkdir -p src include

# 3. 创建所有文件
$ touch Makefile README.md .gitignore
$ touch include/bpm_data.h include/hardware.h
$ touch src/main.c src/bpm_data.c src/hardware.c

# 4. 编写代码（参考 Month1 的项目）

# 5. 构建
$ make

# 6. 测试
$ make run

# 7. Git 提交
$ git add .
$ git commit -m "Initial commit: BPM Data Acquisition System v1.0.0"

# 8. 创建 tag
$ git tag v1.0.0

# 9. 构建不同版本
$ make clean
$ make DEBUG=1
$ make lib
$ make shared
```

---

## 本周知识点总结

### GCC 编译

```bash
# 基本编译
gcc -Wall -g file.c -o program

# 多文件编译
gcc -c file1.c -o file1.o
gcc -c file2.c -o file2.o
gcc file1.o file2.o -o program

# 链接库
gcc main.c -I/path/include -L/path/lib -lmylib -o program
```

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -g

program: main.o utils.o
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o program

.PHONY: clean
```

### GDB 调试

```gdb
break main      # 设置断点
run             # 运行
next            # 单步（不进入函数）
step            # 单步（进入函数）
print var       # 打印变量
backtrace       # 调用栈
```

### Git

```bash
git init              # 初始化仓库
git add file          # 添加文件
git commit -m "msg"   # 提交
git log               # 查看历史
git diff              # 查看差异
```

---

## 自我检查清单

- [ ] 能够使用 gcc 编译 C 程序
- [ ] 理解编译的各个阶段
- [ ] 能够编写基本的 Makefile
- [ ] 理解 Makefile 变量和自动变量
- [ ] 能够创建和使用静态库/动态库
- [ ] 能够使用 gdb 调试程序
- [ ] 掌握基本的 Git 操作
- [ ] 能够创建完整的构建系统

---

## 下一步

- **Week 3**: EPICS Base 安装和配置
- **Week 4**: 创建第一个 IOC

继续加油！🔧
