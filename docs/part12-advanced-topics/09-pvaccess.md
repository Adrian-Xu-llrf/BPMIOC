# PVAccess协议

> **目标**: 使用EPICS 7的PVAccess协议
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 3-5天

## PVAccess vs Channel Access

| 特性 | Channel Access | PVAccess |
|------|---------------|----------|
| **数据类型** | 标量 + 数组 | 结构化数据 |
| **协议** | 专有协议 | 现代化协议 |
| **性能** | 良好 | 更好 |
| **版本** | EPICS 3 | EPICS 4/7 |

## Python pvaccess

### 基本操作

```python
from pvaccess import Channel

# 创建Channel
c = Channel('LLRF:BPM:RFIn_01_Amp')

# 读取值
value = c.get()
print(value)

# 写入值
c.put(12.5)

# 监控
def monitor_callback(pv):
    print(f"Value changed: {pv}")

c.subscribe('field()', monitor_callback)
c.startMonitor()
```

### 结构化数据

```python
from pvaccess import PvObject, STRING, DOUBLE

# 创建结构化PV
structure = {
    'value': DOUBLE,
    'alarm': {
        'severity': INT,
        'status': INT,
        'message': STRING
    },
    'timeStamp': {
        'secondsPastEpoch': LONG,
        'nanoseconds': INT
    }
}

pv = PvObject(structure)
pv['value'] = 12.5
pv['alarm.severity'] = 0
```

## C++ PVAccess

### 客户端

```cpp
#include <pv/pvAccess.h>

using namespace epics::pvAccess;
using namespace epics::pvData;

int main() {
    ChannelProvider::shared_pointer provider = 
        getChannelProviderRegistry()->getProvider("pva");
    
    Channel::shared_pointer channel = 
        provider->createChannel("LLRF:BPM:RFIn_01_Amp");
    
    ChannelGet::shared_pointer get = 
        channel->createChannelGet();
    
    PVStructurePtr pvData = get->get();
    
    double value = pvData->getSubField<PVDouble>("value")->get();
    std::cout << "Value: " << value << std::endl;
    
    return 0;
}
```

### 服务器

```cpp
#include <pv/serverContext.h>

class MyPV : public ChannelProvider {
public:
    virtual PVStructurePtr getPVStructure() {
        FieldCreatePtr fieldCreate = getFieldCreate();
        
        StructureConstPtr structure = fieldCreate->createFieldBuilder()
            ->add("value", pvDouble)
            ->add("alarm.severity", pvInt)
            ->add("alarm.status", pvInt)
            ->createStructure();
        
        return getPVDataCreate()->createPVStructure(structure);
    }
};

int main() {
    ServerContext::shared_pointer server = 
        ServerContext::create(ServerContext::Config()
            .provider(MyPV::shared_pointer(new MyPV())));
    
    server->run(0);  // 运行直到Ctrl+C
    
    return 0;
}
```

## QSRV - PVAccess支持

```bash
# st.cmd
# 启用QSRV（EPICS 7中内置）
epicsEnvSet("EPICS_PVAS_INTF_ADDR_LIST", "192.168.1.100")

dbLoadDatabase("dbd/myApp.dbd")
myApp_registerRecordDeviceDriver(pdbbase)

dbLoadRecords("db/test.db")

iocInit()

# 现在PV可以通过CA和PVA两种方式访问
```

## 测试PVAccess

```bash
# 使用pvget
pvget LLRF:BPM:RFIn_01_Amp

# 使用pvput
pvput LLRF:BPM:RF3RegAddr 0x1000

# 使用pvmonitor
pvmonitor LLRF:BPM:RFIn_01_Amp
```

## 🔗 相关文档

- [02-ca-programming.md](./02-ca-programming.md)
- [03-database-design.md](./03-database-design.md)
