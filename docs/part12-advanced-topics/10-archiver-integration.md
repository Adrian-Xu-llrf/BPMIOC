# 数据归档集成

> **目标**: 配置EPICS数据归档系统
> **难度**: ⭐⭐⭐
> **预计时间**: 2-3天

## Archiver Appliance

### 安装配置

```bash
# 下载Archiver Appliance
git clone https://github.com/slacmshankar/epicsarchiverap.git
cd epicsarchiverap

# 配置
cp install_scripts/singleMachine.sh .
./singleMachine.sh install

# 启动
./singleMachine.sh start
```

### 配置归档PV

访问 http://localhost:17665/mgmt/ui/index.html

添加PV：
```
LLRF:BPM:RFIn_01_Amp
LLRF:BPM:RFIn_01_Pha
LLRF:BPM:RFIn_02_Amp
LLRF:BPM:RFIn_02_Pha
```

### 采样策略

```
# 归档策略配置
{
    "pvName": "LLRF:BPM:RFIn_01_Amp",
    "samplingPeriod": "1.0",
    "samplingMethod": "MONITOR"
}
```

## 数据检索

### REST API

```bash
# 获取数据
curl "http://localhost:17668/retrieval/data/getData.json?pv=LLRF:BPM:RFIn_01_Amp&from=2025-11-09T00:00:00Z&to=2025-11-09T23:59:59Z"
```

### Python检索

```python
import requests
import json
from datetime import datetime, timedelta

def get_archived_data(pv, hours=24):
    end = datetime.now()
    start = end - timedelta(hours=hours)
    
    url = "http://localhost:17668/retrieval/data/getData.json"
    params = {
        'pv': pv,
        'from': start.isoformat(),
        'to': end.isoformat()
    }
    
    response = requests.get(url, params=params)
    data = response.json()
    
    timestamps = []
    values = []
    
    for point in data[0]['data']:
        timestamps.append(point['secs'])
        values.append(point['val'])
    
    return timestamps, values

# 使用
times, vals = get_archived_data('LLRF:BPM:RFIn_01_Amp')
```

## CS-Studio集成

### Data Browser配置

```xml
<!-- databrowser.plt -->
<databrowser>
    <title>BPM Monitoring</title>
    <pv>
        <name>LLRF:BPM:RFIn_01_Amp</name>
        <archive>
            <name>Archiver Appliance</name>
            <url>pbraw://localhost:17668/retrieval</url>
        </archive>
    </pv>
    <pv>
        <name>LLRF:BPM:RFIn_01_Pha</name>
        <archive>
            <name>Archiver Appliance</name>
            <url>pbraw://localhost:17668/retrieval</url>
        </archive>
    </pv>
</databrowser>
```

## 归档监控

### 检查归档状态

```bash
# 获取PV归档状态
curl "http://localhost:17665/mgmt/bpl/getPVStatus?pv=LLRF:BPM:RFIn_01_Amp"

# 获取归档统计
curl "http://localhost:17665/mgmt/bpl/getStorageMetrics"
```

## 🔗 相关文档

- [07-monitoring.md](../part13-deployment/07-monitoring.md)
- [02-ca-programming.md](./02-ca-programming.md)

---

**总结**: Part 12完成！你已掌握EPICS的高级特性和最佳实践。
