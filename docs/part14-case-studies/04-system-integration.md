# 案例4: 系统集成案例

> **项目**: EPICS IOC与TANGO、Archiver、实时数据库集成
> **时长**: 2天（设计0.5天 + 实现1天 + 测试0.5天）
> **难度**: ⭐⭐⭐⭐
> **关键技术**: CA Gateway、数据转换、REST API

## 1. 项目背景

### 需求

某大型加速器设施使用多个控制系统：
- **EPICS**: 低层硬件控制（IOC）
- **TANGO**: 高层应用和界面
- **Archiver Appliance**: 历史数据存储
- **InfluxDB**: 实时监控数据库

需求：
1. TANGO应用访问EPICS PV
2. 所有PV数据归档到Archiver
3. 关键PV实时写入InfluxDB用于Grafana展示

### 挑战

1. **协议转换**: TANGO和EPICS使用不同协议
2. **性能**: 数千个PV，高频更新
3. **可靠性**: 集成不能影响IOC稳定性

## 2. 架构设计

### 2.1 整体架构

```
┌────────────────────────────────────────────────┐
│          TANGO Control System                  │
│  ┌──────────────┐  ┌──────────────┐           │
│  │ Operator GUI │  │ Applications │           │
│  └──────────────┘  └──────────────┘           │
└────────────────────────────────────────────────┘
                    ↕ (Tango Protocol)
┌────────────────────────────────────────────────┐
│        EPICS-TANGO Bridge (Python)             │
│  - pytango Device Server                       │
│  - pyepics CA Client                           │
└────────────────────────────────────────────────┘
                    ↕ (Channel Access)
┌────────────────────────────────────────────────┐
│            CA Gateway (Optional)               │
│  - Protocol translation                        │
│  - Access control                              │
│  - Load balancing                              │
└────────────────────────────────────────────────┘
                    ↕
┌────────────────────────────────────────────────┐
│              EPICS IOC                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  BPM IOC │  │  PS IOC  │  │  VG IOC  │    │
│  └──────────┘  └──────────┘  └──────────┘    │
└────────────────────────────────────────────────┘
        ↓                    ↓
┌──────────────┐    ┌──────────────┐
│   Archiver   │    │   InfluxDB   │
│  Appliance   │    │   + Grafana  │
└──────────────┘    └──────────────┘
```

## 3. 实现

### 3.1 EPICS-TANGO Bridge

使用Python实现双向桥接：

```python
# epics_tango_bridge.py
import PyTango as tango
import epics
import threading

class EPICSBridge(tango.Device_4Impl):
    """TANGO Device Server that bridges to EPICS PVs"""

    def __init__(self, cl, name):
        tango.Device_4Impl.__init__(self, cl, name)
        self.epics_pvs = {}
        self.init_device()

    def init_device(self):
        self.set_state(tango.DevState.ON)
        self.get_device_properties(self.get_device_class())

        # 从配置文件读取PV映射
        # TANGO属性 -> EPICS PV
        self.pv_mapping = {
            'BPM01_Amplitude': 'LLRF:BPM:RFIn_01_Amp',
            'BPM01_Phase': 'LLRF:BPM:RFIn_01_Phase',
            'PS_Q1_Voltage': 'BL01:PS_Q1:Voltage',
            # ... 更多映射
        }

        # 创建EPICS PV连接
        for tango_attr, epics_pv in self.pv_mapping.items():
            pv = epics.PV(epics_pv)
            self.epics_pvs[tango_attr] = pv
            print(f"Connected: {tango_attr} -> {epics_pv}")

    def read_BPM01_Amplitude(self, attr):
        """读取BPM01幅度"""
        pv = self.epics_pvs['BPM01_Amplitude']
        value = pv.get()
        attr.set_value(value)

    def read_BPM01_Phase(self, attr):
        """读取BPM01相位"""
        pv = self.epics_pvs['BPM01_Phase']
        value = pv.get()
        attr.set_value(value)

    def write_PS_Q1_Voltage(self, attr):
        """写入电源电压"""
        pv = self.epics_pvs['PS_Q1_Voltage']
        value = attr.get_write_value()

        # 验证范围
        if 0 <= value <= 30:
            pv.put(value)
        else:
            tango.Except.throw_exception(
                "ValueError",
                "Voltage out of range (0-30V)",
                "write_PS_Q1_Voltage")

    # 使用装饰器自动生成属性
    @tango.command(dtype_in=str, dtype_out=float)
    def ReadPV(self, pv_name):
        """通用PV读取命令"""
        if pv_name in self.epics_pvs:
            return self.epics_pvs[pv_name].get()
        else:
            # 动态创建PV
            pv = epics.PV(pv_name, connection_timeout=1.0)
            if pv.connected:
                self.epics_pvs[pv_name] = pv
                return pv.get()
            else:
                tango.Except.throw_exception(
                    "ConnectionError",
                    f"Cannot connect to {pv_name}",
                    "ReadPV")

class EPICSBridgeClass(tango.DeviceClass):
    """TANGO Device Class"""

    # 属性定义
    attr_list = {
        'BPM01_Amplitude': [[tango.DevDouble, tango.SCALAR,
                            tango.READ]],
        'BPM01_Phase': [[tango.DevDouble, tango.SCALAR,
                        tango.READ]],
        'PS_Q1_Voltage': [[tango.DevDouble, tango.SCALAR,
                          tango.READ_WRITE]],
    }

    cmd_list = {
        'ReadPV': [[tango.DevString, "PV name"],
                   [tango.DevDouble, "PV value"]],
    }

    def __init__(self, name):
        tango.DeviceClass.__init__(self, name)
        self.set_type(name)

if __name__ == '__main__':
    # 启动TANGO Device Server
    util = tango.Util(sys.argv)
    util.add_class(EPICSBridgeClass, EPICSBridge)

    U = tango.Util.instance()
    U.server_init()
    U.server_run()
```

启动：
```bash
python epics_tango_bridge.py EPICSBridge instance01
```

TANGO客户端访问：
```python
import PyTango as tango

device = tango.DeviceProxy("EPICSBridge/instance01")
amp = device.BPM01_Amplitude
print(f"BPM01 Amplitude: {amp} dBm")

# 写入电源电压
device.PS_Q1_Voltage = 25.0
```

### 3.2 Archiver Appliance集成

配置PV归档：

```bash
# 1. 创建PV列表文件
cat > pvs_to_archive.txt <<EOF
LLRF:BPM:RFIn_01_Amp
LLRF:BPM:RFIn_01_Phase
LLRF:BPM:RFIn_02_Amp
LLRF:BPM:RFIn_02_Phase
BL01:PS_Q1:Voltage
BL01:PS_Q1:Current
BL01:VG01:Pressure
EOF

# 2. 使用REST API提交归档请求
for pv in $(cat pvs_to_archive.txt); do
    curl -X POST \
         -H "Content-Type: application/json" \
         -d "{\"pv\": \"$pv\", \"samplingperiod\": \"1.0\", \"samplingmethod\": \"MONITOR\"}" \
         http://archiver.example.com:17665/mgmt/bpl/archivePV
done

# 3. 验证归档状态
curl http://archiver.example.com:17665/mgmt/bpl/getPVStatus?pv=LLRF:BPM:RFIn_01_Amp
```

检索归档数据：

```python
import requests
import datetime

def get_archived_data(pv_name, start_time, end_time):
    """从Archiver检索历史数据"""
    url = f"http://archiver.example.com:17668/retrieval/data/getData.json"
    params = {
        'pv': pv_name,
        'from': start_time.isoformat(),
        'to': end_time.isoformat()
    }

    response = requests.get(url, params=params)
    data = response.json()

    timestamps = []
    values = []
    for point in data[0]['data']:
        timestamps.append(point['secs'])
        values.append(point['val'])

    return timestamps, values

# 使用示例
start = datetime.datetime.now() - datetime.timedelta(hours=1)
end = datetime.datetime.now()
timestamps, values = get_archived_data('LLRF:BPM:RFIn_01_Amp', start, end)

import matplotlib.pyplot as plt
plt.plot(timestamps, values)
plt.xlabel('Time')
plt.ylabel('Amplitude (dBm)')
plt.title('BPM01 Amplitude - Last Hour')
plt.show()
```

### 3.3 InfluxDB集成

实时写入关键PV到InfluxDB：

```python
# epics_to_influx.py
import epics
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import time

# InfluxDB配置
INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "your-token"
INFLUX_ORG = "your-org"
INFLUX_BUCKET = "epics-data"

# 初始化InfluxDB客户端
client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = client.write_api(write_options=SYNCHRONOUS)

# 需要监控的PV
pvs_to_monitor = [
    {'name': 'LLRF:BPM:RFIn_01_Amp', 'tag': 'bpm01', 'field': 'amplitude'},
    {'name': 'LLRF:BPM:RFIn_01_Phase', 'tag': 'bpm01', 'field': 'phase'},
    {'name': 'BL01:PS_Q1:Voltage', 'tag': 'ps_q1', 'field': 'voltage'},
]

def pv_callback(pvname, value, **kwargs):
    """PV变化回调函数"""
    # 查找PV配置
    pv_config = next((p for p in pvs_to_monitor if p['name'] == pvname), None)
    if not pv_config:
        return

    # 创建InfluxDB数据点
    point = Point("epics_pv") \
        .tag("device", pv_config['tag']) \
        .field(pv_config['field'], value) \
        .time(time.time_ns())

    # 写入InfluxDB
    try:
        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print(f"Written: {pvname} = {value}")
    except Exception as e:
        print(f"Error writing to InfluxDB: {e}")

# 订阅PV变化
for pv_config in pvs_to_monitor:
    pv = epics.PV(pv_config['name'])
    pv.add_callback(pv_callback)
    print(f"Monitoring: {pv_config['name']}")

print("InfluxDB bridge running...")

# 保持运行
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Shutting down...")
    client.close()
```

Grafana查询示例（Flux语法）：

```flux
from(bucket: "epics-data")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "epics_pv")
  |> filter(fn: (r) => r.device == "bpm01")
  |> filter(fn: (r) => r._field == "amplitude")
```

### 3.4 CA Gateway配置

使用CA Gateway提供访问控制和负载均衡：

```bash
# gateway.pvlist - PV访问控制
# 格式: PATTERN ALLOW/DENY HOST_PATTERN

# 允许所有主机读取BPM数据
LLRF:BPM:.*     ALLOW   .*

# 仅允许控制室写入电源PV
BL01:PS:.*:Set.*   ALLOW   192.168.1.0/24
BL01:PS:.*:Set.*   DENY    .*

# 禁止外部访问内部诊断PV
.*:Internal:.*     DENY    .*
```

```bash
# gateway.access - 主机访问规则
# HOST_PATTERN    ACCESS_LEVEL

192.168.1.0/24    WRITE     # 控制室网络：读写
10.0.0.0/8        READ      # 办公网络：只读
.*                DENY      # 其他：拒绝
```

启动Gateway：

```bash
gateway \
    -access gateway.access \
    -pvlist gateway.pvlist \
    -server 192.168.1.100 \
    -cip 192.168.1.255 \
    -log gateway.log \
    -debug 1
```

## 4. 性能优化

### 4.1 批量订阅

原代码每个PV单独订阅：

```python
# 慢：每个PV独立连接
for pv_name in pv_list:
    pv = epics.PV(pv_name)
    pv.add_callback(callback)
```

优化：使用上下文管理器：

```python
# 快：共享连接上下文
import epics

# 预连接所有PV
pvs = {name: epics.PV(name) for name in pv_list}
epics.ca.flush_io()  # 批量flush

# 添加回调
for pv in pvs.values():
    pv.add_callback(callback)
```

### 4.2 缓存和去重

避免重复写入相同值：

```python
# 缓存上次写入的值
last_values = {}

def pv_callback(pvname, value, **kwargs):
    # 去重：仅值变化时写入
    if pvname in last_values and last_values[pvname] == value:
        return  # 跳过

    last_values[pvname] = value
    write_to_influxdb(pvname, value)
```

## 5. 监控和维护

### 5.1 健康检查

```python
# health_check.py
import epics
import requests

def check_epics_ioc():
    """检查EPICS IOC连接"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp', connection_timeout=2.0)
    return pv.connected

def check_tango_bridge():
    """检查TANGO桥接"""
    try:
        device = tango.DeviceProxy("EPICSBridge/instance01")
        state = device.state()
        return state == tango.DevState.ON
    except:
        return False

def check_archiver():
    """检查Archiver"""
    try:
        resp = requests.get("http://archiver:17665/mgmt/bpl/getVersion",
                          timeout=2)
        return resp.status_code == 200
    except:
        return False

# 定期检查
import time
while True:
    status = {
        'EPICS': check_epics_ioc(),
        'TANGO': check_tango_bridge(),
        'Archiver': check_archiver()
    }
    print(f"Status: {status}")

    # 报警
    if not all(status.values()):
        send_alert(status)

    time.sleep(60)
```

### 5.2 性能监控

```python
# 监控Bridge性能
import time
from collections import deque

class PerformanceMonitor:
    def __init__(self, window_size=60):
        self.timestamps = deque(maxlen=window_size)
        self.counts = deque(maxlen=window_size)

    def record(self):
        now = time.time()
        self.timestamps.append(now)
        self.counts.append(1)

    def get_rate(self):
        """计算每秒更新率"""
        if len(self.timestamps) < 2:
            return 0.0
        duration = self.timestamps[-1] - self.timestamps[0]
        return len(self.counts) / duration if duration > 0 else 0.0

monitor = PerformanceMonitor()

def pv_callback_with_monitor(pvname, value, **kwargs):
    monitor.record()
    pv_callback(pvname, value, **kwargs)

# 定期报告
def report_performance():
    while True:
        time.sleep(10)
        rate = monitor.get_rate()
        print(f"Update rate: {rate:.1f} updates/sec")
```

## 6. 经验教训

### ✅ 成功经验

1. **分层集成**
   - 不直接耦合系统
   - 使用桥接和Gateway解耦

2. **异步处理**
   - InfluxDB写入使用异步
   - 避免阻塞PV回调

3. **监控完善**
   - 每个集成点都有健康检查
   - 性能指标可视化

### ❌ 遇到的问题

1. **网络分区**
   - EPICS和TANGO在不同子网
   - 解决：配置路由或使用Gateway

2. **性能瓶颈**
   - 初期单线程处理，跟不上1kHz更新
   - 解决：多线程、批量处理

3. **时间同步**
   - 各系统时间不一致导致数据对不上
   - 解决：NTP同步所有服务器

## 🔗 相关资源

- [EPICS CA Gateway](https://epics.anl.gov/extensions/gateway/)
- [Archiver Appliance](https://slacmshankar.github.io/epicsarchiver_docs/)
- [PyTango Documentation](https://pytango.readthedocs.io/)
- [InfluxDB Python Client](https://github.com/influxdata/influxdb-client-python)
