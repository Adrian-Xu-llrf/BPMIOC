# 案例3: 多设备IOC设计

> **项目**: 一个IOC管理BPM、电源、真空计三种设备
> **时长**: 3天（设计1天 + 实现1.5天 + 测试0.5天）
> **难度**: ⭐⭐⭐⭐
> **关键技术**: 设备抽象、插件架构、配置驱动

## 1. 项目背景

### 需求

加速器束线站需要一个统一的控制系统管理多种设备：

| 设备类型 | 数量 | 功能 | 接口 |
|----------|------|------|------|
| **BPM** | 8个 | 束流位置监测 | PCIe + 共享内存 |
| **电源** | 12个 | 磁铁电源控制 | Modbus TCP |
| **真空计** | 6个 | 真空度监测 | Serial RS485 |

### 挑战

1. **设备异构性**
   - 不同通信协议（PCIe、TCP、Serial）
   - 不同数据格式和速率
   - 不同错误处理机制

2. **可扩展性**
   - 未来可能增加新设备类型
   - 设备数量可能变化

3. **独立性**
   - 一个设备故障不影响其他设备
   - 可以单独重启某类设备驱动

## 2. 架构设计

### 2.1 整体架构

采用**插件化驱动架构**：

```
┌─────────────────────────────────────────────┐
│           Database Layer                    │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐       │
│  │ BPM.db  │ │ PS.db   │ │ VG.db   │       │
│  └─────────┘ └─────────┘ └─────────┘       │
└─────────────────────────────────────────────┘
                    ↕ (DSET)
┌─────────────────────────────────────────────┐
│        Device Support Layer                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │devBPM.c  │ │devPS.c   │ │devVG.c   │    │
│  └──────────┘ └──────────┘ └──────────┘    │
└─────────────────────────────────────────────┘
                    ↕
┌─────────────────────────────────────────────┐
│      Device Manager (deviceManager.c)       │
│  - 设备注册/发现                             │
│  - 统一错误处理                              │
│  - 配置加载                                  │
└─────────────────────────────────────────────┘
                    ↕
┌─────────────────────────────────────────────┐
│           Device Drivers (插件)              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │BPMDrv.so │ │PSDrv.so  │ │VGDrv.so  │    │
│  └──────────┘ └──────────┘ └──────────┘    │
└─────────────────────────────────────────────┘
```

### 2.2 设备抽象接口

定义统一的设备驱动接口：

```c
// deviceInterface.h
typedef struct DeviceDriver {
    // 驱动信息
    const char *name;
    const char *version;

    // 生命周期函数
    int (*init)(void *config);
    int (*start)(void);
    int (*stop)(void);
    void (*cleanup)(void);

    // 数据访问
    int (*read)(int dev_id, int param_id, void *value);
    int (*write)(int dev_id, int param_id, const void *value);

    // 状态查询
    int (*get_status)(int dev_id, DeviceStatus *status);
    int (*get_error)(int dev_id, char *error_msg, size_t len);

    // 可选：异步通知
    int (*register_callback)(int dev_id, DeviceCallback cb, void *user_data);
} DeviceDriver;

// 设备状态
typedef enum {
    DEV_STATUS_OK = 0,
    DEV_STATUS_WARNING,
    DEV_STATUS_ERROR,
    DEV_STATUS_OFFLINE
} DeviceStatus;
```

### 2.3 配置文件驱动

使用JSON配置文件描述设备：

```json
{
  "devices": [
    {
      "type": "BPM",
      "driver": "./drivers/libBPMDriver.so",
      "instances": [
        {"id": 0, "name": "BPM01", "address": "0x1000"},
        {"id": 1, "name": "BPM02", "address": "0x2000"}
      ],
      "config": {
        "sampling_rate": 1000,
        "channels": 8
      }
    },
    {
      "type": "PowerSupply",
      "driver": "./drivers/libPSDriver.so",
      "instances": [
        {"id": 0, "name": "PS_Q1", "ip": "192.168.1.10", "port": 502},
        {"id": 1, "name": "PS_Q2", "ip": "192.168.1.11", "port": 502}
      ],
      "config": {
        "max_voltage": 30.0,
        "max_current": 10.0
      }
    },
    {
      "type": "VacuumGauge",
      "driver": "./drivers/libVGDriver.so",
      "instances": [
        {"id": 0, "name": "VG01", "serial": "/dev/ttyS0", "baud": 9600}
      ]
    }
  ]
}
```

## 3. 核心实现

### 3.1 设备管理器

```c
// deviceManager.c
#include <dlfcn.h>
#include "deviceInterface.h"
#include "cJSON.h"

#define MAX_DEVICE_TYPES 10
#define MAX_DEVICES 100

// 设备类型注册表
static struct {
    char type_name[32];
    DeviceDriver *driver;
    void *lib_handle;
    int num_instances;
} g_device_types[MAX_DEVICE_TYPES];
static int g_num_types = 0;

// 设备实例表
static struct {
    int type_index;
    int dev_id;
    char name[32];
    DeviceStatus status;
    char error_msg[256];
} g_devices[MAX_DEVICES];
static int g_num_devices = 0;

// 加载配置
int DeviceManager_LoadConfig(const char *config_file) {
    // 1. 读取JSON配置
    FILE *fp = fopen(config_file, "r");
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *json_str = malloc(len + 1);
    fread(json_str, 1, len, fp);
    fclose(fp);

    cJSON *root = cJSON_Parse(json_str);
    cJSON *devices = cJSON_GetObjectItem(root, "devices");

    // 2. 遍历设备类型
    cJSON *device_type = NULL;
    cJSON_ArrayForEach(device_type, devices) {
        const char *type = cJSON_GetObjectItem(device_type, "type")->valuestring;
        const char *driver_path = cJSON_GetObjectItem(device_type, "driver")->valuestring;

        // 3. 加载驱动插件
        void *lib_handle = dlopen(driver_path, RTLD_LAZY);
        if (!lib_handle) {
            printf("ERROR: Cannot load %s: %s\n", driver_path, dlerror());
            continue;
        }

        // 4. 获取驱动接口
        DeviceDriver* (*get_driver)(void) = dlsym(lib_handle, "GetDeviceDriver");
        if (!get_driver) {
            printf("ERROR: %s missing GetDeviceDriver\n", driver_path);
            dlclose(lib_handle);
            continue;
        }

        DeviceDriver *driver = get_driver();

        // 5. 注册设备类型
        int type_idx = g_num_types++;
        strncpy(g_device_types[type_idx].type_name, type, 31);
        g_device_types[type_idx].driver = driver;
        g_device_types[type_idx].lib_handle = lib_handle;

        // 6. 初始化驱动
        cJSON *config = cJSON_GetObjectItem(device_type, "config");
        char *config_str = cJSON_PrintUnformatted(config);
        driver->init(config_str);
        free(config_str);

        // 7. 创建设备实例
        cJSON *instances = cJSON_GetObjectItem(device_type, "instances");
        cJSON *instance = NULL;
        cJSON_ArrayForEach(instance, instances) {
            int dev_id = cJSON_GetObjectItem(instance, "id")->valueint;
            const char *name = cJSON_GetObjectItem(instance, "name")->valuestring;

            int dev_idx = g_num_devices++;
            g_devices[dev_idx].type_index = type_idx;
            g_devices[dev_idx].dev_id = dev_id;
            strncpy(g_devices[dev_idx].name, name, 31);
            g_devices[dev_idx].status = DEV_STATUS_OK;

            printf("Registered device: %s (type=%s, id=%d)\n", name, type, dev_id);
        }

        g_device_types[type_idx].num_instances =
            cJSON_GetArraySize(instances);

        // 8. 启动驱动
        driver->start();
    }

    cJSON_Delete(root);
    free(json_str);

    printf("Device Manager: Loaded %d types, %d devices\n",
           g_num_types, g_num_devices);
    return 0;
}

// 统一的读取接口
int DeviceManager_Read(const char *device_name, int param_id, void *value) {
    // 查找设备
    for (int i = 0; i < g_num_devices; i++) {
        if (strcmp(g_devices[i].name, device_name) == 0) {
            int type_idx = g_devices[i].type_index;
            DeviceDriver *driver = g_device_types[type_idx].driver;

            // 调用驱动的read函数
            int ret = driver->read(g_devices[i].dev_id, param_id, value);

            // 更新状态
            if (ret != 0) {
                g_devices[i].status = DEV_STATUS_ERROR;
                driver->get_error(g_devices[i].dev_id,
                                 g_devices[i].error_msg, 256);
            } else {
                g_devices[i].status = DEV_STATUS_OK;
            }

            return ret;
        }
    }

    printf("ERROR: Device %s not found\n", device_name);
    return -1;
}

// 健康检查线程
void* HealthCheckThread(void *arg) {
    while (1) {
        for (int i = 0; i < g_num_devices; i++) {
            int type_idx = g_devices[i].type_index;
            DeviceDriver *driver = g_device_types[type_idx].driver;

            DeviceStatus status;
            driver->get_status(g_devices[i].dev_id, &status);

            if (status != g_devices[i].status) {
                printf("Device %s status changed: %d -> %d\n",
                       g_devices[i].name, g_devices[i].status, status);
                g_devices[i].status = status;

                // 记录错误信息
                if (status == DEV_STATUS_ERROR) {
                    driver->get_error(g_devices[i].dev_id,
                                     g_devices[i].error_msg, 256);
                    printf("  Error: %s\n", g_devices[i].error_msg);
                }
            }
        }

        sleep(5);  // 每5秒检查一次
    }
}
```

### 3.2 BPM驱动插件

```c
// drivers/bpmDriver.c
#include "deviceInterface.h"

static int g_initialized = 0;
static float g_data_buffer[8][8][14];  // [dev_id][channel][param]

static int bpm_init(void *config) {
    // 解析配置
    printf("BPM Driver: Initializing with config: %s\n", (char*)config);

    // 初始化硬件
    // ...

    g_initialized = 1;
    return 0;
}

static int bpm_start(void) {
    printf("BPM Driver: Started\n");
    // 启动采集线程...
    return 0;
}

static int bpm_read(int dev_id, int param_id, void *value) {
    if (!g_initialized) return -1;

    // param_id格式: (channel << 8) | offset
    int channel = (param_id >> 8) & 0xFF;
    int offset = param_id & 0xFF;

    *(float*)value = g_data_buffer[dev_id][channel][offset];
    return 0;
}

static int bpm_get_status(int dev_id, DeviceStatus *status) {
    *status = DEV_STATUS_OK;  // 简化实现
    return 0;
}

static DeviceDriver g_bpm_driver = {
    .name = "BPM Driver",
    .version = "1.0",
    .init = bpm_init,
    .start = bpm_start,
    .stop = NULL,
    .cleanup = NULL,
    .read = bpm_read,
    .write = NULL,
    .get_status = bpm_get_status,
    .get_error = NULL,
    .register_callback = NULL
};

// 导出接口
DeviceDriver* GetDeviceDriver(void) {
    return &g_bpm_driver;
}
```

### 3.3 电源驱动插件（Modbus TCP）

```c
// drivers/psDriver.c
#include "deviceInterface.h"
#include <modbus/modbus.h>

#define MAX_PS 12

static modbus_t *g_modbus_ctx[MAX_PS];

static int ps_init(void *config) {
    printf("PowerSupply Driver: Init\n");
    // 解析配置，初始化Modbus连接...
    return 0;
}

static int ps_read(int dev_id, int param_id, void *value) {
    if (!g_modbus_ctx[dev_id]) return -1;

    // param_id: 0=voltage, 1=current, 2=status
    uint16_t reg_addr = param_id * 2;  // 假设每个参数占2个寄存器
    uint16_t regs[2];

    int ret = modbus_read_input_registers(g_modbus_ctx[dev_id],
                                          reg_addr, 2, regs);
    if (ret != 2) return -1;

    // 转换为浮点数（假设高字节在前）
    uint32_t raw = (regs[0] << 16) | regs[1];
    *(float*)value = *((float*)&raw);

    return 0;
}

static int ps_write(int dev_id, int param_id, const void *value) {
    if (!g_modbus_ctx[dev_id]) return -1;

    float val = *(float*)value;
    uint32_t raw = *((uint32_t*)&val);
    uint16_t regs[2] = {raw >> 16, raw & 0xFFFF};

    return modbus_write_registers(g_modbus_ctx[dev_id],
                                  param_id * 2, 2, regs);
}

static DeviceDriver g_ps_driver = {
    .name = "PowerSupply Driver",
    .version = "1.0",
    .init = ps_init,
    .start = NULL,
    .read = ps_read,
    .write = ps_write,
    // ...
};

DeviceDriver* GetDeviceDriver(void) {
    return &g_ps_driver;
}
```

### 3.4 设备支持层（通用）

```c
// devGeneric.c
#include <aiRecord.h>
#include <aoRecord.h>
#include "deviceManager.h"

typedef struct {
    char device_name[32];
    int param_id;
} GenericDevPvt;

static long init_ai_record(aiRecord *prec) {
    // INP格式: "@device_name param_id"
    // 例如: "@BPM01 0x0100"  (channel=1, offset=0)
    char device_name[32];
    int param_id;

    if (sscanf(prec->inp.value.instio.string, "@%s %x",
               device_name, &param_id) != 2) {
        return -1;
    }

    GenericDevPvt *pPvt = malloc(sizeof(GenericDevPvt));
    strcpy(pPvt->device_name, device_name);
    pPvt->param_id = param_id;
    prec->dpvt = pPvt;

    return 0;
}

static long read_ai(aiRecord *prec) {
    GenericDevPvt *pPvt = (GenericDevPvt*)prec->dpvt;

    float value;
    int ret = DeviceManager_Read(pPvt->device_name, pPvt->param_id, &value);

    if (ret == 0) {
        prec->val = value;
        prec->udf = 0;
        return 0;
    } else {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return -1;
    }
}

struct {
    long number;
    // ...
    DEVSUPFUN read;
} devAiGeneric = {
    6, NULL, NULL, init_ai_record, NULL, read_ai, NULL
};
epicsExportAddress(dset, devAiGeneric);
```

## 4. 数据库配置

### BPM PV

```
record(ai, "BL01:BPM01:Ch1:Amp") {
    field(DTYP, "Generic Device")
    field(INP, "@BPM01 0x0100")  # dev=BPM01, ch=1, offset=0
    field(SCAN, ".1 second")
    field(EGU, "dBm")
}
```

### 电源 PV

```
record(ai, "BL01:PS_Q1:Voltage") {
    field(DTYP, "Generic Device")
    field(INP, "@PS_Q1 0")  # param_id=0 (voltage)
    field(SCAN, "1 second")
    field(EGU, "V")
}

record(ao, "BL01:PS_Q1:SetVoltage") {
    field(DTYP, "Generic Device")
    field(OUT, "@PS_Q1 0")
    field(DRVL, "0")
    field(DRVH, "30")
}
```

### 真空计 PV

```
record(ai, "BL01:VG01:Pressure") {
    field(DTYP, "Generic Device")
    field(INP, "@VG01 0")
    field(SCAN, "2 second")
    field(EGU, "mbar")
    field(PREC, "6")
}
```

## 5. 优点和效果

### ✅ 优点

1. **统一管理**
   - 一个IOC管理所有设备
   - 统一的配置和监控

2. **易扩展**
   - 添加新设备类型只需编写驱动插件
   - 无需修改IOC核心代码

3. **故障隔离**
   - 一个驱动崩溃不影响其他驱动
   - 可以单独重启某个驱动

4. **配置灵活**
   - JSON配置文件易于修改
   - 支持热加载（reload配置）

### 📊 效果

| 指标 | 值 |
|------|-----|
| 管理设备数 | 26个（8 BPM + 12 PS + 6 VG） |
| 总PV数 | 156个 |
| CPU占用 | ~12% |
| 内存占用 | 85MB |
| 启动时间 | ~5s |

## 6. 经验教训

### ❌ 问题

1. **dlopen插件卸载问题**
   - dlclose后仍有引用导致崩溃
   - 解决：永不卸载，或引用计数

2. **JSON解析性能**
   - 启动时解析大配置文件慢
   - 解决：缓存解析结果

### 💡 改进

1. **设备自动发现**
   ```c
   // 扫描网络发现Modbus设备
   for (int ip = 10; ip < 100; ip++) {
       modbus_t *ctx = modbus_new_tcp("192.168.1." + ip, 502);
       if (modbus_connect(ctx) == 0) {
           // 发现设备，自动注册
       }
   }
   ```

2. **热插拔支持**
   - USB设备插入自动识别
   - 设备断开自动标记offline

## 🔗 相关资源

- [Part 4: 驱动层](../part4-driver-layer/)
- [案例1: 完整项目](./01-bpm-ioc-complete-project.md)
- [cJSON库](https://github.com/DaveGamble/cJSON)
- [libmodbus](https://libmodbus.org/)
