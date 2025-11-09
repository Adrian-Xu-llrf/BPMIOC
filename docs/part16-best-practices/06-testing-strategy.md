# 测试策略

> **目标**: 制定全面的测试策略
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 2-3天

## 测试金字塔

```
       /\
      /E2E\      端到端测试（少量）
     /------\
    / 集成测试 \   集成测试（适量）
   /----------\
  /  单元测试   \  单元测试（大量）
 /--------------\
```

## 测试覆盖目标

| 测试类型 | 覆盖率目标 | 说明 |
|----------|-----------|------|
| **单元测试** | >80% | 函数级别测试 |
| **集成测试** | >60% | 模块间交互 |
| **系统测试** | >40% | 完整功能场景 |

## 单元测试

### 测试框架选择

```c
// 使用Unity框架
#include "unity.h"

void test_ReadData_valid_channel(void) {
    g_data_buffer[OFFSET_AMP][0] = 12.5;
    float result = ReadData(OFFSET_AMP, 0, 0);
    TEST_ASSERT_EQUAL_FLOAT(12.5, result);
}

void test_ReadData_invalid_channel(void) {
    float result = ReadData(OFFSET_AMP, 999, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0, result);
}
```

### Mock对象

```c
// Mock硬件函数
float mock_BPM_RFIn_ReadADC(int channel, int type) {
    return 10.0 + channel;  // 返回测试数据
}

void test_with_mock(void) {
    // 替换真实函数
    BPM_RFIn_ReadADC = mock_BPM_RFIn_ReadADC;
    
    // 测试
    float result = ReadData(OFFSET_AMP, 0, 0);
    TEST_ASSERT_EQUAL_FLOAT(10.0, result);
}
```

## 集成测试

### 测试IOC启动

```python
import subprocess
import epics
import time

def test_ioc_startup():
    # 启动IOC
    proc = subprocess.Popen(['./st.cmd'])
    
    # 等待启动
    time.sleep(3)
    
    # 测试连接
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')
    assert pv.connected, "PV not connected"
    
    # 清理
    proc.terminate()
```

## 性能测试

### 吞吐量测试

```python
def test_throughput():
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')
    
    count = 0
    start = time.time()
    
    while time.time() - start < 10:
        value = pv.get()
        count += 1
    
    rate = count / 10.0
    assert rate > 100, f"Throughput too low: {rate} reads/sec"
```

## 测试自动化

### Makefile集成

```makefile
# test/Makefile
test: unit_test integration_test

unit_test:
	./test_driverWrapper
	./test_device

integration_test:
	python test_ioc.py

coverage:
	gcov -r ../src/*.c
	gcovr -r ..
```

## 🔗 相关文档

- [05-code-review.md](./05-code-review.md)
- [Part 10: Testing](../part10-debugging-testing/)
