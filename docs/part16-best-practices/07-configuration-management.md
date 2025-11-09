# 配置管理

> **目标**: 管理不同环境的配置
> **难度**: ⭐⭐⭐
> **预计时间**: 1天

## 配置分离

### 环境配置

```bash
# config/development.env
EPICS_CA_ADDR_LIST=127.0.0.1
BPM_DRIVER_LIB=/opt/simulator/lib/libBPMDriver.so
LOG_LEVEL=DEBUG

# config/production.env
EPICS_CA_ADDR_LIST=192.168.1.255
BPM_DRIVER_LIB=/opt/BPMDriver/lib/libBPMDriver.so
LOG_LEVEL=INFO
```

### st.cmd配置

```bash
# iocBoot/iocBPMmonitor/st.cmd
#!/bin/sh

# 加载环境配置
ENV_FILE=${ENV_FILE:-development}
source $(dirname $0)/../../config/${ENV_FILE}.env

# 启动IOC
cd $(dirname $0)
exec ../../bin/${EPICS_HOST_ARCH}/BPMmonitor st.cmd.template
```

## 版本管理

### 配置版本化

```
config/
├── v1.0/
│   ├── BPMmonitor.db
│   └── st.cmd
├── v1.1/
│   ├── BPMmonitor.db
│   └── st.cmd
└── current -> v1.1/
```

## 密钥管理

### 不要提交密钥

```bash
# .gitignore
*.key
*.pem
secrets.env
```

### 使用环境变量

```bash
# 从环境变量读取
export DB_PASSWORD="secret"

# 或使用密钥文件
export DB_PASSWORD=$(cat /secure/db_password)
```

## 🔗 相关文档

- [03-version-control.md](./03-version-control.md)
- [Part 13: Deployment](../part13-deployment/)
