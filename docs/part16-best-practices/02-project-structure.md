# 项目结构

> **目标**: 组织清晰的项目目录结构
> **难度**: ⭐⭐
> **预计时间**: 半天

## 标准EPICS项目结构

```
BPMmonitor/
├── configure/              # 构建配置
│   ├── CONFIG
│   ├── CONFIG_SITE
│   ├── RELEASE
│   └── RULES
├── BPMmonitorApp/          # 应用代码
│   ├── src/                # 源代码
│   │   ├── driverWrapper.c
│   │   ├── devBPMMonitor.c
│   │   ├── Makefile
│   │   └── BPMmonitor_registerRecordDeviceDriver.cpp
│   └── Db/                 # 数据库文件
│       ├── BPMmonitor.db
│       └── Makefile
├── iocBoot/                # IOC启动脚本
│   └── iocBPMmonitor/
│       ├── st.cmd
│       └── Makefile
├── dbd/                    # 生成的DBD文件
├── db/                     # 生成的DB文件
├── bin/                    # 生成的可执行文件
│   ├── linux-x86_64/
│   └── linux-arm/
├── lib/                    # 生成的库文件
├── docs/                   # 文档
│   ├── README.md
│   ├── DESIGN.md
│   └── API.md
├── test/                   # 测试
│   ├── unit/
│   └── integration/
├── scripts/                # 工具脚本
│   ├── deploy.sh
│   └── backup.sh
├── README.md               # 项目说明
├── LICENSE                 # 许可证
├── .gitignore              # Git忽略文件
└── Makefile                # 顶层Makefile
```

## 模块化组织

### 按功能分模块

```
BPMmonitorApp/src/
├── driver/             # 驱动层
│   ├── driver.c
│   ├── driver.h
│   └── Makefile
├── device/             # 设备支持层
│   ├── devAi.c
│   ├── devAo.c
│   └── Makefile
├── utils/              # 工具函数
│   ├── math_utils.c
│   ├── log_utils.c
│   └── Makefile
└── Makefile
```

### 头文件组织

```c
// driver.h - 公共接口
#ifndef DRIVER_H
#define DRIVER_H

int InitDevice(void);
float ReadData(int offset, int channel, int type);

#endif

// driver_internal.h - 内部实现
#ifndef DRIVER_INTERNAL_H
#define DRIVER_INTERNAL_H

#include "driver.h"

// 内部函数和数据结构
typedef struct {
    int initialized;
    void *lib_handle;
} DriverPrivate;

#endif
```

## 🔗 相关文档

- [01-coding-standards.md](./01-coding-standards.md)
- [03-version-control.md](./03-version-control.md)
