# 集成测试完全指南

> **目标**: 掌握EPICS IOC集成测试
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 2-3天
> **前置知识**: Python、EPICS基础、单元测试

## 📋 本文档内容

1. 集成测试概述
2. EPICS IOC集成测试
3. Python测试脚本
4. pyepics使用
5. 自动化测试
6. CI/CD集成

## 🎯 为什么需要集成测试

**单元测试** vs **集成测试**:

```
单元测试:
  测试单个函数
  ├── ReadData()  ✓
  ├── SetReg()    ✓
  └── init_record()  ✓

集成测试:
  测试整个系统
  └── IOC启动 → CA连接 → caget/caput → 数据正确 ✓
```

集成测试验证：
- ✅ **组件协作**: 所有层是否正确配合
- ✅ **端到端流程**: 从硬件到CA的完整数据流
- ✅ **实际场景**: 模拟真实使用场景
- ✅ **配置正确**: .db文件、st.cmd是否正确

## 1️⃣ 集成测试概述

### 测试金字塔

```
        /\
       /E2E\      少量端到端测试（慢、全面）
      /------\
     /集成测试 \    中等数量集成测试（中速、重要场景）
    /----------\
   / 单元测试    \  大量单元测试（快速、细节）
  /--------------\
```

### 集成测试类型

| 类型 | 说明 | 示例 |
|------|------|------|
| **Big Bang** | 一次性集成所有模块 | 测试整个IOC |
| **Top-Down** | 从上层开始（Database→Device→Driver） | 先测PV访问 |
| **Bottom-Up** | 从底层开始（Driver→Device→Database） | 先测硬件驱动 |
| **Sandwich** | 结合Top-Down和Bottom-Up | 推荐方式 |

## 2️⃣ EPICS IOC集成测试策略

### 测试层次

```
Level 1: 启动测试
  └── IOC能否成功启动

Level 2: PV访问测试
  └── 能否caget/caput

Level 3: 数据正确性测试
  └── 值是否符合预期

Level 4: 功能测试
  └── 业务逻辑是否正确

Level 5: 压力测试
  └── 高负载下是否稳定
```

### 测试环境准备

```bash
# 1. 编译IOC
cd /opt/BPMmonitor
make clean
make

# 2. 准备测试数据库
cd iocBoot/iocBPMmonitor
cat > test.db <<'EOF'
record(ai, "TEST:Value") {
    field(DTYP, "Soft Channel")
    field(INP, "12.34")
}
EOF

# 3. 准备测试启动脚本
cat > st_test.cmd <<'EOF'
#!/opt/BPMmonitor/bin/linux-x86_64/BPMmonitor
dbLoadDatabase("../../dbd/BPMmonitor.dbd")
BPMmonitor_registerRecordDeviceDriver(pdbbase)

# 加载测试数据库
dbLoadRecords("test.db")

iocInit()
EOF

chmod +x st_test.cmd
```

## 3️⃣ Python集成测试

### 安装依赖

```bash
# 安装pyepics
pip install pyepics

# 或使用conda
conda install -c conda-forge pyepics

# 安装pytest（可选）
pip install pytest
```

### 基本测试框架

创建 `test/integration/test_basic.py`:

```python
#!/usr/bin/env python3
"""
BPMIOC集成测试 - 基础测试
"""
import epics
import time
import subprocess
import os
import signal

class BPMIOCTest:
    """BPMIOC测试基类"""

    def __init__(self):
        self.ioc_process = None
        self.ioc_started = False

    def start_ioc(self, timeout=10):
        """启动IOC"""
        print("Starting IOC...")

        # 启动IOC进程
        cmd = ["/opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd"]
        self.ioc_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn=os.setsid  # 创建新的进程组
        )

        # 等待IOC启动
        start_time = time.time()
        while time.time() - start_time < timeout:
            # 尝试连接PV
            try:
                pv = epics.PV('LLRF:BPM:RFIn_01_Amp', timeout=1.0)
                if pv.connected:
                    print("IOC started successfully")
                    self.ioc_started = True
                    return True
            except:
                pass

            time.sleep(0.5)

        print("ERROR: IOC failed to start")
        return False

    def stop_ioc(self):
        """停止IOC"""
        if self.ioc_process:
            print("Stopping IOC...")
            # 杀死整个进程组
            os.killpg(os.getpgid(self.ioc_process.pid), signal.SIGTERM)
            self.ioc_process.wait(timeout=5)
            self.ioc_started = False
            print("IOC stopped")

    def test_pv_connection(self, pv_name, timeout=5.0):
        """测试PV连接"""
        print(f"Testing PV connection: {pv_name}")

        pv = epics.PV(pv_name, timeout=timeout)

        if pv.connected:
            print(f"  ✓ Connected")
            return True
        else:
            print(f"  ✗ Connection failed")
            return False

    def test_pv_read(self, pv_name, expected_type=None):
        """测试PV读取"""
        print(f"Testing PV read: {pv_name}")

        pv = epics.PV(pv_name)

        if not pv.connected:
            print(f"  ✗ Not connected")
            return False

        value = pv.get()

        if value is None:
            print(f"  ✗ Read failed")
            return False

        print(f"  ✓ Value: {value}")

        if expected_type:
            if not isinstance(value, expected_type):
                print(f"  ✗ Type mismatch: expected {expected_type}, got {type(value)}")
                return False

        return True

    def test_pv_write(self, pv_name, value):
        """测试PV写入"""
        print(f"Testing PV write: {pv_name} = {value}")

        pv = epics.PV(pv_name)

        if not pv.connected:
            print(f"  ✗ Not connected")
            return False

        # 写入值
        pv.put(value)

        # 等待写入完成
        time.sleep(0.1)

        # 读回验证
        readback = pv.get()

        if readback == value:
            print(f"  ✓ Write successful, readback: {readback}")
            return True
        else:
            print(f"  ✗ Write failed, expected {value}, got {readback}")
            return False


def test_ioc_startup():
    """测试1: IOC启动"""
    print("\n=== Test 1: IOC Startup ===")

    test = BPMIOCTest()

    # 启动IOC
    assert test.start_ioc(), "IOC failed to start"

    # 停止IOC
    test.stop_ioc()

    print("✓ PASSED\n")


def test_pv_connections():
    """测试2: PV连接"""
    print("\n=== Test 2: PV Connections ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 测试所有RF输入PV
        pv_list = [
            'LLRF:BPM:RFIn_01_Amp',
            'LLRF:BPM:RFIn_01_Pha',
            'LLRF:BPM:RFIn_02_Amp',
            'LLRF:BPM:RFIn_02_Pha',
            'LLRF:BPM:RF3Amp',
            'LLRF:BPM:RF3Pha',
        ]

        all_connected = True
        for pv_name in pv_list:
            if not test.test_pv_connection(pv_name):
                all_connected = False

        assert all_connected, "Not all PVs connected"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()


def test_pv_read_values():
    """测试3: PV读取"""
    print("\n=== Test 3: PV Read Values ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 测试读取值
        pv_tests = [
            ('LLRF:BPM:RFIn_01_Amp', float),
            ('LLRF:BPM:RFIn_01_Pha', float),
            ('LLRF:BPM:Trigin', int),
        ]

        all_passed = True
        for pv_name, expected_type in pv_tests:
            if not test.test_pv_read(pv_name, expected_type):
                all_passed = False

        assert all_passed, "Some PV reads failed"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()


def test_pv_write():
    """测试4: PV写入"""
    print("\n=== Test 4: PV Write ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 测试寄存器写入
        assert test.test_pv_write('LLRF:BPM:RF3RegAddr', 0x1000)
        assert test.test_pv_write('LLRF:BPM:RF3RegValue', 0xABCD)

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()


def test_data_continuity():
    """测试5: 数据连续性"""
    print("\n=== Test 5: Data Continuity ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

        # 读取10次，检查值是否连续变化
        values = []
        for i in range(10):
            value = pv.get()
            values.append(value)
            print(f"  Read {i+1}: {value}")
            time.sleep(0.1)

        # 检查值是否在合理范围
        assert all(v is not None for v in values), "Some reads returned None"

        # 检查值是否在合理范围 (-10 到 30 dBm)
        assert all(-10 <= v <= 30 for v in values), "Values out of range"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()


if __name__ == '__main__':
    print("BPMIOC Integration Tests")
    print("=" * 50)

    # 运行所有测试
    try:
        test_ioc_startup()
        test_pv_connections()
        test_pv_read_values()
        test_pv_write()
        test_data_continuity()

        print("\n" + "=" * 50)
        print("ALL TESTS PASSED ✓")

    except AssertionError as e:
        print(f"\nTEST FAILED: {e}")
        exit(1)
    except Exception as e:
        print(f"\nERROR: {e}")
        exit(1)
```

运行测试：

```bash
python3 test_basic.py
```

输出示例：

```
BPMIOC Integration Tests
==================================================

=== Test 1: IOC Startup ===
Starting IOC...
IOC started successfully
Stopping IOC...
IOC stopped
✓ PASSED

=== Test 2: PV Connections ===
Starting IOC...
IOC started successfully
Testing PV connection: LLRF:BPM:RFIn_01_Amp
  ✓ Connected
Testing PV connection: LLRF:BPM:RFIn_01_Pha
  ✓ Connected
...
✓ PASSED

==================================================
ALL TESTS PASSED ✓
```

## 4️⃣ 高级测试场景

### 测试1: I/O中断扫描

```python
def test_io_interrupt_scan():
    """测试I/O中断扫描"""
    print("\n=== Test: I/O Interrupt Scan ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 创建PV并添加回调
        pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

        update_count = [0]  # 使用list以便在回调中修改

        def on_change(pvname=None, value=None, **kwargs):
            update_count[0] += 1
            print(f"  Update {update_count[0]}: {pvname} = {value}")

        # 添加回调
        pv.add_callback(on_change)

        # 等待10秒，收集更新
        print("  Waiting for updates...")
        time.sleep(10)

        # 验证收到了更新
        print(f"  Total updates: {update_count[0]}")
        assert update_count[0] > 0, "No updates received"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()
```

### 测试2: 扫描周期验证

```python
def test_scan_period():
    """测试扫描周期"""
    print("\n=== Test: Scan Period ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 测试0.5秒扫描的PV
        pv = epics.PV('LLRF:BPM:RF3Amp')

        timestamps = []
        values = []

        def on_change(pvname=None, value=None, timestamp=None, **kwargs):
            timestamps.append(timestamp)
            values.append(value)

        pv.add_callback(on_change)

        # 等待5秒
        time.sleep(5)

        # 分析时间戳
        if len(timestamps) > 1:
            intervals = [timestamps[i+1] - timestamps[i]
                         for i in range(len(timestamps)-1)]

            avg_interval = sum(intervals) / len(intervals)
            print(f"  Average interval: {avg_interval:.3f} seconds")

            # 验证平均间隔接近0.5秒
            assert 0.4 < avg_interval < 0.6, \
                   f"Scan period mismatch: expected 0.5s, got {avg_interval:.3f}s"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()
```

### 测试3: 波形数据

```python
def test_waveform_data():
    """测试波形数据"""
    print("\n=== Test: Waveform Data ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        # 假设有波形PV
        pv = epics.PV('LLRF:BPM:Waveform')

        # 读取波形
        waveform = pv.get()

        print(f"  Waveform length: {len(waveform)}")
        print(f"  First 10 points: {waveform[:10]}")

        # 验证波形长度
        assert len(waveform) == 1024, "Waveform length incorrect"

        # 验证数据类型
        assert all(isinstance(x, float) for x in waveform), \
               "Waveform contains non-float values"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()
```

### 测试4: 压力测试

```python
def test_high_frequency_access():
    """测试高频访问"""
    print("\n=== Test: High Frequency Access ===")

    test = BPMIOCTest()
    test.start_ioc()

    try:
        pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

        # 快速读取1000次
        start_time = time.time()
        errors = 0

        for i in range(1000):
            value = pv.get(timeout=1.0)
            if value is None:
                errors += 1

        elapsed = time.time() - start_time

        print(f"  1000 reads in {elapsed:.3f} seconds")
        print(f"  Rate: {1000/elapsed:.1f} reads/sec")
        print(f"  Errors: {errors}")

        # 验证错误率低于1%
        assert errors < 10, f"Too many errors: {errors}/1000"

        print("✓ PASSED\n")

    finally:
        test.stop_ioc()
```

## 5️⃣ 使用pytest框架

### 安装pytest

```bash
pip install pytest pytest-timeout
```

### 创建pytest测试

`test/integration/test_bpmioc_pytest.py`:

```python
import pytest
import epics
import time
import subprocess
import os
import signal

@pytest.fixture(scope="module")
def ioc():
    """Fixture: 启动/停止IOC"""
    print("\nStarting IOC for tests...")

    # 启动IOC
    cmd = ["/opt/BPMmonitor/iocBoot/iocBPMmonitor/st.cmd"]
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid
    )

    # 等待IOC启动
    time.sleep(3)

    # 验证IOC已启动
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp', timeout=5.0)
    assert pv.connected, "IOC failed to start"

    yield process

    # 停止IOC
    print("\nStopping IOC...")
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    process.wait(timeout=5)


@pytest.mark.parametrize("pv_name", [
    'LLRF:BPM:RFIn_01_Amp',
    'LLRF:BPM:RFIn_01_Pha',
    'LLRF:BPM:RFIn_02_Amp',
    'LLRF:BPM:RFIn_02_Pha',
])
def test_pv_connection(ioc, pv_name):
    """测试PV连接"""
    pv = epics.PV(pv_name, timeout=5.0)
    assert pv.connected, f"PV {pv_name} not connected"


@pytest.mark.parametrize("pv_name,expected_type", [
    ('LLRF:BPM:RFIn_01_Amp', float),
    ('LLRF:BPM:RFIn_01_Pha', float),
    ('LLRF:BPM:Trigin', int),
])
def test_pv_value_type(ioc, pv_name, expected_type):
    """测试PV值类型"""
    pv = epics.PV(pv_name)
    value = pv.get()
    assert isinstance(value, expected_type), \
           f"Expected {expected_type}, got {type(value)}"


def test_pv_value_range(ioc):
    """测试PV值范围"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

    # 读取10次
    values = [pv.get() for _ in range(10)]

    # 验证范围
    assert all(-10 <= v <= 30 for v in values), \
           f"Values out of range: {values}"


@pytest.mark.timeout(30)
def test_continuous_read(ioc):
    """测试连续读取"""
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')

    errors = 0
    for _ in range(100):
        value = pv.get(timeout=1.0)
        if value is None:
            errors += 1

    assert errors == 0, f"{errors} read errors occurred"
```

运行pytest：

```bash
cd test/integration

# 运行所有测试
pytest test_bpmioc_pytest.py -v

# 运行特定测试
pytest test_bpmioc_pytest.py::test_pv_connection -v

# 生成HTML报告
pytest test_bpmioc_pytest.py --html=report.html

# 并行运行（需要pytest-xdist）
pip install pytest-xdist
pytest test_bpmioc_pytest.py -n auto
```

输出示例：

```
======================== test session starts =========================
test_bpmioc_pytest.py::test_pv_connection[LLRF:BPM:RFIn_01_Amp] PASSED [ 14%]
test_bpmioc_pytest.py::test_pv_connection[LLRF:BPM:RFIn_01_Pha] PASSED [ 28%]
test_bpmioc_pytest.py::test_pv_value_type[LLRF:BPM:RFIn_01_Amp-float] PASSED [ 42%]
test_bpmioc_pytest.py::test_pv_value_range PASSED [ 57%]
test_bpmioc_pytest.py::test_continuous_read PASSED [ 71%]

======================= 7 passed in 15.23s ==========================
```

## 6️⃣ CI/CD集成

### GitHub Actions工作流

创建 `.github/workflows/test.yml`:

```yaml
name: BPMIOC Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  test:
    runs-on: ubuntu-20.04

    steps:
    - name: Checkout code
      uses: actions/checkout@v2

    - name: Install EPICS Base
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential libreadline-dev
        cd /opt
        sudo git clone --recursive https://github.com/epics-base/epics-base.git
        cd epics-base
        sudo make

    - name: Build BPMIOC
      run: |
        cd $GITHUB_WORKSPACE
        make clean
        make

    - name: Install Python dependencies
      run: |
        pip install pyepics pytest pytest-timeout

    - name: Run unit tests
      run: |
        cd test
        make test

    - name: Run integration tests
      run: |
        cd test/integration
        pytest test_bpmioc_pytest.py -v

    - name: Upload test results
      if: always()
      uses: actions/upload-artifact@v2
      with:
        name: test-results
        path: test/integration/report.html
```

### Jenkins Pipeline

创建 `Jenkinsfile`:

```groovy
pipeline {
    agent any

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build') {
            steps {
                sh 'make clean'
                sh 'make'
            }
        }

        stage('Unit Tests') {
            steps {
                sh 'cd test && make test'
            }
        }

        stage('Integration Tests') {
            steps {
                sh 'cd test/integration && pytest test_bpmioc_pytest.py -v --junitxml=results.xml'
            }
        }
    }

    post {
        always {
            junit 'test/integration/results.xml'
        }
    }
}
```

## 📝 练习任务

### 练习1: 基础集成测试

1. 编写Python脚本测试IOC启动
2. 测试所有PV是否可连接
3. 验证PV值范围

### 练习2: 功能测试

1. 测试寄存器读写功能
2. 测试I/O中断扫描
3. 测试波形数据采集

### 练习3: 压力测试

1. 高频率caget测试（1000次/秒）
2. 多客户端并发访问
3. 长时间运行稳定性测试

### 练习4: 自动化测试

1. 使用pytest编写测试套件
2. 配置CI/CD pipeline
3. 生成测试报告

## 🔍 最佳实践

### ✅ 好的集成测试

```python
# 1. 使用fixture管理IOC生命周期
@pytest.fixture(scope="module")
def ioc():
    # 启动IOC
    process = start_ioc()
    yield process
    # 停止IOC
    stop_ioc(process)

# 2. 参数化测试
@pytest.mark.parametrize("pv,expected_range", [
    ('LLRF:BPM:RFIn_01_Amp', (-10, 30)),
    ('LLRF:BPM:RFIn_01_Pha', (-180, 180)),
])
def test_pv_range(ioc, pv, expected_range):
    value = epics.caget(pv)
    assert expected_range[0] <= value <= expected_range[1]

# 3. 清晰的断言消息
def test_pv_connection(ioc):
    pv = epics.PV('LLRF:BPM:RFIn_01_Amp')
    assert pv.connected, \
           f"PV not connected. Check if IOC is running and firewall allows CA traffic."
```

### ⚠️ 常见陷阱

1. **未清理资源**: IOC未正确停止
2. **时序问题**: 未等待IOC启动完成
3. **环境依赖**: 测试依赖特定环境
4. **不稳定测试**: 时序相关的随机失败

## 📚 参考资源

- **pyepics文档**: https://pyepics.github.io/pyepics/
- **pytest文档**: https://docs.pytest.org/
- **EPICS测试**: https://epics-controls.org/resources-and-support/documents/appdev/
- **CI/CD**: https://docs.github.com/en/actions

## 🔗 相关文档

- **[04-unit-testing.md](./04-unit-testing.md)** - 单元测试
- **[Part 2: 06-channel-access.md](../part2-understanding-basics/06-channel-access.md)** - Channel Access
- **[Part 8: Labs](../part8-hands-on-labs/)** - 动手实验

---

**总结**: 完成Part 10后，你已掌握BPMIOC的调试、日志、性能分析和测试技能！
