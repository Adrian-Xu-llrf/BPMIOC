# Lab 6: 添加新PV

> **难度**: ⭐⭐
> **时间**: 2小时
> **前置**: Lab 1-5, Part 6

## 🎯 实验目标

学会如何在BPMIOC中添加新的Process Variable（PV），包括：

1. 在.db文件中添加新Record
2. 理解PV命名规范
3. 配置Record字段
4. 测试新PV
5. 理解数据库加载过程

---

## 📚 背景知识

### PV的本质

```
PV (Process Variable) = Record实例

.db文件中的一个record{} → IOC中的一个PV
```

### 添加PV的完整流程

```
1. 编辑.db文件
   ├─ 添加record定义
   ├─ 配置字段
   └─ 遵循命名规范
     ↓
2. 重新编译（如果需要）
   ├─ make clean
   └─ make
     ↓
3. 重启IOC
   ├─ 加载新.db
   └─ iocInit()
     ↓
4. 测试新PV
   ├─ dbl查看
   ├─ caget读取
   └─ camonitor监控
```

---

## 🔧 实验任务

### 任务1: 添加温度监控PV

**需求**: 为BPMIOC添加一个温度监控PV `$(P):Temperature`

---

### 步骤1: 备份原始文件

```bash
cd /home/user/BPMIOC/BPMmonitorApp/Db

# 备份
cp BPMMonitor.db BPMMonitor.db.backup

# 查看原始文件
head -30 BPMMonitor.db
```

**输出示例**:
```
record(ai, "$(P):RFIn_01_Amp")
{
    field(SCAN, ".5 second")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=0")
}
...
```

---

### 步骤2: 添加温度Record

**编辑BPMMonitor.db**，在文件末尾添加：

```bash
# 在文件末尾添加温度监控PV
vim BPMMonitor.db
```

**添加内容**:
```
#================================================
# 温度监控
#================================================

record(ai, "$(P):Temperature")
{
    field(DESC, "FPGA Temperature")
    field(DTYP, "BPMmonitor")
    field(INP,  "@TEMP:0")
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "0")
    field(HIHI, "85")
    field(HIGH, "75")
    field(LOW,  "0")
    field(LOLO, "-10")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
}
```

**字段说明**:
- `DESC`: 描述信息
- `DTYP`: 设备类型（使用BPMmonitor设备支持）
- `INP`: 输入链接（`@TEMP:0`需要驱动层支持）
- `SCAN`: 5秒扫描一次（温度变化慢）
- `PREC`: 1位小数
- `EGU`: 工程单位（摄氏度）
- `HOPR/LOPR`: 显示范围0-100°C
- `HIHI/HIGH`: 高温报警限值（75°C/85°C）
- `LOW/LOLO`: 低温报警限值（0°C/-10°C）

---

### 步骤3: 理解INP字段

**关键**: `field(INP, "@TEMP:0")`

这个字段将传递给设备支持层的`init_record()`函数：

```c
// devBPMMonitor.c: init_record_ai()
char *param = prec->inp.value.instio.string;
// param = "@TEMP:0"

// 解析参数
sscanf(param, "@%[^:]:%d", type_str, &offset);
// type_str = "TEMP"
// offset = 0
```

**问题**: 当前驱动层可能不支持`TEMP`类型！

**解决方案** (两种):

**方案A**: 使用已支持的类型（如AMP）
```
field(INP, "@AMP:0 ch=15")  # 假装是第15通道
```

**方案B**: 修改驱动层添加TEMP支持（Lab 7将实现）

本实验先使用**方案A**进行测试。

---

### 步骤4: 修改INP字段使用已支持类型

**修改为**:
```
record(ai, "$(P):Temperature")
{
    field(DESC, "FPGA Temperature (Mock)")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=15")  # 使用第15通道模拟温度
    field(SCAN, "5 second")
    field(PREC, "1")
    field(EGU,  "C")
    field(HOPR, "100")
    field(LOPR, "0")
    field(HIHI, "85")
    field(HIGH, "75")
    field(LOW,  "0")
    field(LOLO, "-10")
    field(HHSV, "MAJOR")
    field(HSV,  "MINOR")
    field(LSV,  "MINOR")
    field(LLSV, "MAJOR")
}
```

---

### 步骤5: 重新编译

```bash
cd /home/user/BPMIOC

# 清理旧编译文件
make clean

# 重新编译
make
```

**注意**:
- 如果只修改.db文件，**不一定需要重新编译**
- 但为了确保.db被复制到正确位置，建议执行`make`

---

### 步骤6: 重启IOC

```bash
cd /home/user/BPMIOC/iocBoot/iocBPMmonitor

# 重启IOC
./st.cmd
```

**启动输出**:
```
Starting iocInit
...
iocRun: All initialization complete
```

---

### 步骤7: 验证新PV

**在另一个终端**:

```bash
# 列出所有PV，查找Temperature
dbl | grep Temperature

# 应该看到：
iLinac_007:BPM14And15:Temperature
```

**读取PV值**:
```bash
caget iLinac_007:BPM14And15:Temperature

# 输出示例：
iLinac_007:BPM14And15:Temperature 45.3 C
```

**监控PV**:
```bash
camonitor iLinac_007:BPM14And15:Temperature

# 输出示例：
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:15.123 45.3
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:20.456 45.4
...
```

---

### 步骤8: 查看PV详细信息

```bash
# 查看所有字段
dbpr iLinac_007:BPM14And15:Temperature 3

# 输出：
ASG:                          # Access Security Group
DTYP: BPMmonitor             # Device Type
EGU: C                       # Engineering Units
HIHI: 85                     # High High Limit
HIGH: 75                     # High Limit
HOPR: 100                    # High Operating Range
INP:INST_IO @AMP:0 ch=15     # Input Link
LOPR: 0                      # Low Operating Range
PREC: 1                      # Precision
SCAN: 5 second               # Scan Period
VAL: 45.3                    # Value
...
```

---

### 步骤9: 测试报警功能

**模拟高温**: 如果值超过75°C，应触发MINOR报警

**查看报警状态**:
```bash
caget -t iLinac_007:BPM14And15:Temperature

# 如果温度正常：
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:15.123 45.3 NO_ALARM

# 如果温度过高（假设驱动层返回了高值）：
iLinac_007:BPM14And15:Temperature 2024-11-09 12:30:20.456 76.0 HIGH MINOR
```

---

## 📝 任务2: 添加更多PV

### 子任务2.1: 添加FPGA ID PV

**需求**: 添加一个longin Record读取FPGA ID

```
record(longin, "$(P):FPGA_ID")
{
    field(DESC, "FPGA Identifier")
    field(DTYP, "BPMmonitor")
    field(INP,  "@AMP:0 ch=16")  # 暂用ch=16
    field(SCAN, "10 second")
}
```

**测试**:
```bash
# 重新make和启动IOC

caget iLinac_007:BPM14And15:FPGA_ID
# 输出：iLinac_007:BPM14And15:FPGA_ID 12345
```

---

### 子任务2.2: 添加Status字符串PV

**需求**: 添加stringin Record显示状态

```
record(stringin, "$(P):Status")
{
    field(DESC, "System Status")
    field(DTYP, "Soft Channel")  # 软Record，不连接硬件
    field(VAL,  "OK")
    field(PINI, "YES")
}
```

**注意**: `DTYP="Soft Channel"`表示这是一个软PV，不需要设备支持层。

**测试**:
```bash
caget iLinac_007:BPM14And15:Status
# 输出：iLinac_007:BPM14And15:Status OK

# 修改值
caput iLinac_007:BPM14And15:Status "Running"

# 再次读取
caget iLinac_007:BPM14And15:Status
# 输出：iLinac_007:BPM14And15:Status Running
```

---

## 🔍 深入理解

### .db文件加载过程

```
st.cmd执行：
  dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM14And15")
    ↓
EPICS解析.db文件
  ├─ 读取record(ai, "$(P):Temperature")
  ├─ 宏替换：$(P) → iLinac_007:BPM14And15
  └─ 创建PV："iLinac_007:BPM14And15:Temperature"
    ↓
分配内存创建Record结构
  ├─ aiRecord *prec = malloc(sizeof(aiRecord))
  ├─ 填充字段：DTYP="BPMmonitor", INP="@AMP:0 ch=15"
  └─ 添加到数据库
    ↓
iocInit()时
  ├─ 调用init_record_ai(prec)
  │  ├─ 解析INP字段
  │  ├─ 创建DevPvt
  │  └─ 保存到prec->dpvt
  ├─ 注册到扫描线程（5 second）
  └─ PINI="YES"的立即处理
    ↓
扫描线程每5秒
  └─ 调用read_ai(prec) → 更新VAL
```

---

### PV命名最佳实践

**BPMIOC的命名规范**:
```
$(P):Temperature
  ↑       ↑
  │       └─ 信号名（清晰描述）
  └───────── 宏（系统:设备）
```

**遵循规则**:
1. 使用宏 `$(P)` 实现复用
2. 信号名清晰（Temperature, not Temp or T）
3. 分隔符一致（使用`:`或`_`）
4. 全局唯一

**好的命名**:
```
$(P):Temperature
$(P):FPGA_ID
$(P):Status
$(P):RFIn_Avg_Amp
```

**不好的命名**:
```
temp         # 太模糊
$(P):T       # 缩写不清
$(P):Tmp1    # 数字含义不明
Temperature  # 没有使用宏（不可复用）
```

---

## ⚠️ 常见错误

### 错误1: PV名重复

**现象**:
```
Error: Duplicate record name 'Test:PV'
IOC initialization failed
```

**原因**: 两个record使用了相同的PV名

**解决**: 确保每个record的PV名全局唯一

---

### 错误2: 宏未定义

**现象**:
```
caget $(P):Temperature
Channel connect timed out: '$(P):Temperature'
```

**原因**: st.cmd中未定义宏`P`

**解决**:
```bash
# st.cmd中
dbLoadRecords("BPMMonitor.db", "P=iLinac_007:BPM14")
                                ↑ 确保定义了P
```

---

### 错误3: DTYP不匹配

**现象**:
```
devAi: init_record, Illegal DTYP field
```

**原因**: `DTYP`字段值不存在

**解决**: 使用已注册的设备类型（如`"BPMmonitor"`）

---

### 错误4: INP格式错误

**现象**:
```
read_ai returned error
```

**原因**: INP字段格式不符合设备支持层的解析规则

**解决**: 检查设备支持层代码，确认INP格式

---

## ✅ 学习检查点

- [ ] 理解PV和Record的关系
- [ ] 能够在.db文件中添加新Record
- [ ] 理解宏替换机制
- [ ] 能够配置Record的各个字段
- [ ] 理解报警配置的作用
- [ ] 能够使用caget/caput/camonitor测试PV
- [ ] 理解.db文件的加载过程

---

## 🚀 扩展挑战

### 挑战1: 添加计算PV

添加一个calc Record计算平均温度：

```
record(calc, "$(P):Temperature_Avg")
{
    field(DESC, "Average Temperature")
    field(SCAN, "5 second")
    field(INPA, "$(P):Temperature")
    field(INPB, "$(P):Temperature_Avg")
    field(CALC, "(A+B)/2")  # 简单移动平均
    field(PREC, "2")
    field(EGU,  "C")
}
```

---

### 挑战2: 添加输出PV

添加一个ao Record控制风扇速度：

```
record(ao, "$(P):Fan_Speed")
{
    field(DESC, "Fan Speed Control")
    field(DTYP, "BPMmonitor")
    field(OUT,  "@REG:10")  # 写到寄存器10
    field(DRVH, "100")      # 驱动高限
    field(DRVL, "0")        # 驱动低限
    field(PREC, "0")
    field(EGU,  "%")
}
```

测试：
```bash
caput iLinac_007:BPM14And15:Fan_Speed 75
caget iLinac_007:BPM14And15:Fan_Speed
```

---

### 挑战3: 批量添加PV

添加4个温度传感器PV：

```
record(ai, "$(P):Temp_Sensor_01") { ... }
record(ai, "$(P):Temp_Sensor_02") { ... }
record(ai, "$(P):Temp_Sensor_03") { ... }
record(ai, "$(P):Temp_Sensor_04") { ... }
```

提示：可以使用脚本生成重复的Record定义。

---

## 📚 相关知识

- **Part 6.1**: .db文件结构
- **Part 6.2**: Record配置详解
- **Part 6.3**: PV命名规范
- **Lab 2**: 修改扫描周期
- **Lab 9**: 使用calc Record实现校准

---

## 🎯 总结

本实验学习了如何添加新PV，这是IOC开发的基本技能：

1. **编辑.db文件**: 添加record定义
2. **配置字段**: DTYP, INP, SCAN等
3. **遵循命名规范**: 使用宏，清晰命名
4. **测试验证**: 使用CA工具测试

**关键理解**:
- PV = .db文件中的Record实例
- 宏使.db文件可复用
- 字段配置影响PV的行为

**下一步**: Lab 7学习如何修改驱动层，支持新的INP类型（如`@TEMP:0`）！

---

**恭喜完成Lab 6！** 你已经学会了添加新PV，继续加油！ 💪
