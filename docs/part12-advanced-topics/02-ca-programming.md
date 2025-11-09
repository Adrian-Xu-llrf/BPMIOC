# Channel Access编程

> **目标**: 掌握CA客户端和服务器编程
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 3-5天

## C语言CA客户端

### 基本读写

```c
#include <cadef.h>
#include <stdio.h>

int main() {
    chid pv_chid;
    double value;
    
    // 初始化CA上下文
    SEVCHK(ca_context_create(ca_disable_preemptive_callback), "ca_context_create");
    
    // 创建Channel
    SEVCHK(ca_create_channel("LLRF:BPM:RFIn_01_Amp",
                             NULL, NULL, 10, &pv_chid),
           "ca_create_channel");
    
    // 等待连接
    SEVCHK(ca_pend_io(5.0), "ca_pend_io");
    
    // 读取值
    SEVCHK(ca_get(DBR_DOUBLE, pv_chid, &value), "ca_get");
    SEVCHK(ca_pend_io(1.0), "ca_pend_io");
    
    printf("Value: %.3f\n", value);
    
    // 清理
    ca_clear_channel(pv_chid);
    ca_context_destroy();
    
    return 0;
}
```

### 监控PV变化

```c
void connection_handler(struct connection_handler_args args) {
    chid ch = args.chid;
    printf("PV %s %s\n", ca_name(ch),
           args.op == CA_OP_CONN_UP ? "connected" : "disconnected");
}

void event_handler(struct event_handler_args args) {
    if (args.status == ECA_NORMAL) {
        double value = *(double*)args.dbr;
        printf("Value changed: %.3f\n", value);
    }
}

int main() {
    chid pv_chid;
    evid pv_evid;
    
    ca_context_create(ca_enable_preemptive_callback);
    
    ca_create_channel("LLRF:BPM:RFIn_01_Amp",
                      connection_handler, NULL, 10, &pv_chid);
    
    ca_pend_io(5.0);
    
    // 订阅变化通知
    ca_create_subscription(DBR_DOUBLE, 1, pv_chid,
                           DBE_VALUE, event_handler, NULL, &pv_evid);
    
    // 保持程序运行
    ca_pend_event(0.0);
    
    return 0;
}
```

## Python CA编程

### pyepics基础

```python
import epics

# 简单读写
value = epics.caget('LLRF:BPM:RFIn_01_Amp')
epics.caput('LLRF:BPM:RF3RegAddr', 0x1000)

# 使用PV对象
pv = epics.PV('LLRF:BPM:RFIn_01_Amp')
print(f"Value: {pv.get()}")
print(f"Timestamp: {pv.timestamp}")

# 监控回调
def on_change(pvname=None, value=None, **kwargs):
    print(f"{pvname} = {value}")

pv.add_callback(on_change)
```

### 批量操作

```python
# 批量读取
pvnames = [f'LLRF:BPM:Ch{i}:Amp' for i in range(8)]
values = epics.caget_many(pvnames)

# 批量写入
epics.caput_many(pvnames, [1.0] * 8)
```

## 编译和链接

```makefile
# Makefile
CC = gcc
CFLAGS = -I$(EPICS_BASE)/include -Wall
LDFLAGS = -L$(EPICS_BASE)/lib/$(EPICS_HOST_ARCH) -lca -lCom

ca_client: ca_client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
```

## 🔗 相关文档

- [03-database-design.md](./03-database-design.md)
- [Part 2: 06-channel-access.md](../part2-understanding-basics/06-channel-access.md)
