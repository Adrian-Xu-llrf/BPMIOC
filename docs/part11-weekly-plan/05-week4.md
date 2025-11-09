# Week 4: 驱动层修改实践

> **时间**: 第4周（15-20小时）
> **目标**: 动手修改驱动层代码，添加新功能
> **难度**: ⭐⭐⭐⭐☆

## 📅 本周目标

完成本周后，你应该能够：
- ✅ 独立添加新的PV
- ✅ 修改数据采集逻辑
- ✅ 理解代码修改的影响
- ✅ 完成修改实验6-7

## 📚 学习内容

### Day 1-2: 添加新PV实践

**学习材料**：
- Part 4: 14-modification-guide.md
- Part 8: lab06-add-new-pv.md

**学习要点**：
- 添加新PV的完整流程
- 修改driverWrapper.c
- 修改BPMMonitor.db
- 编译和测试

**实践**：
```bash
# 实验6: 添加温度PV
cd ~/BPMIOC/BPMmonitorApp/src
vim driverWrapper.c

# 添加GetTemperature()硬件函数
# 在ReadData()中添加offset=29

# 编译测试
cd ~/BPMIOC
make

cd iocBoot/iocBPMmonitor
./st.cmd

# 测试新PV
epics> caget LLRF:BPM:Temp1
```

**检查点**：
- [ ] 理解添加新PV的5个步骤
- [ ] 能够独立添加新标量PV
- [ ] 能够测试新添加的PV

### Day 3-4: 修改采集逻辑

**学习材料**：
- Part 4: 06-pthread.md
- Part 4: 07-readdata.md

**学习要点**：
- pthread数据采集线程
- 修改采集周期
- 添加数据处理
- 性能影响分析

**实践**：
```bash
# 修改采集周期为50ms (20 Hz)
cd ~/BPMIOC/BPMmonitorApp/src
vim driverWrapper.c

# 找到pthread函数
# 修改 usleep(100000) → usleep(50000)

# 重新编译
cd ~/BPMIOC
make

# 测试并观察CPU占用
./st.cmd
top -p $(pidof st.cmd)
```

**检查点**：
- [ ] 理解pthread的作用
- [ ] 能够修改采集周期
- [ ] 能够添加数据处理逻辑
- [ ] 理解性能影响

### Day 5: 调试和故障排查

**学习材料**：
- Part 4: 12-debugging.md
- Part 4: 13-troubleshooting.md

**学习要点**：
- printf调试技巧
- gdb使用方法
- 常见问题排查

**实践**：
```bash
# 添加调试输出
cd ~/BPMIOC/BPMmonitorApp/src
vim driverWrapper.c

# 在关键位置添加printf
printf("[DEBUG] ReadData: offset=%d, channel=%d\n", offset, channel);

# 使用gdb调试
make USR_CFLAGS="-g -O0"
cd iocBoot/iocBPMmonitor
gdb ./st.cmd

(gdb) break ReadData
(gdb) run
```

**检查点**：
- [ ] 掌握printf调试方法
- [ ] 能够使用gdb设置断点
- [ ] 能够排查基本问题

### Weekend: 实验7 + 代码回顾

**实验7**: lab07-modify-driver.md
- 修改驱动层添加新功能
- 测试修改后的代码
- 验证性能影响

**代码回顾**：
- 回顾本周修改的代码
- 理解修改对系统的影响
- 思考改进方案

**总结**：
- 记录遇到的问题和解决方法
- 整理修改代码的经验
- 准备下周学习

## ✅ Week 4 检查点

### 实践能力
- [ ] 能够独立添加新PV（标量和波形）
- [ ] 能够修改数据采集周期
- [ ] 能够添加数据处理逻辑
- [ ] 能够调试代码问题

### 代码理解
- [ ] 理解driverWrapper.c的结构
- [ ] 理解ReadData()的工作原理
- [ ] 理解pthread的数据采集流程
- [ ] 理解dlopen/dlsym机制

### 问题排查
- [ ] 能够使用printf调试
- [ ] 能够使用gdb调试
- [ ] 能够排查编译错误
- [ ] 能够排查运行时问题

## 📊 学习进度

在 [10-progress-tracker.md](./10-progress-tracker.md) 中记录：

```markdown
## Week 4: 驱动层修改实践 ✅
- [x] Day 1: 添加新PV (2025-11-17)
- [x] Day 2: 实验6 - 添加温度PV (2025-11-18)
- [x] Day 3: 修改采集周期 (2025-11-19)
- [x] Day 4: 添加数据处理逻辑 (2025-11-20)
- [x] Day 5: 调试和故障排查 (2025-11-21)
- [x] Weekend: 实验7 + 代码回顾 (2025-11-22/23)

收获：
- 成功添加了温度监控PV
- 掌握了代码修改的完整流程
- 学会了使用gdb调试

问题：
- 修改采集周期后CPU占用略有上升
- 需要更深入理解dlopen机制
```

## 💡 学习建议

1. **动手为主**：本周重点是实践，多动手修改代码
2. **小步快跑**：每次只修改一个地方，及时测试
3. **做好备份**：修改前备份代码，出错可以回滚
4. **记录过程**：记录每次修改和遇到的问题
5. **对比验证**：修改前后对比，验证功能正确性

## 🔗 相关资源

- [Part 4: 驱动层详解](../part4-driver-layer/)
- [Part 8: 修改实验](../part8-hands-on-labs/labs-modification/)

## ⚠️ 注意事项

1. **备份代码**：每次修改前先备份
   ```bash
   cp driverWrapper.c driverWrapper.c.bak
   ```

2. **编译检查**：修改后立即编译，发现错误
   ```bash
   make 2>&1 | tee compile.log
   ```

3. **测试验证**：修改后充分测试
   ```bash
   # 测试现有PV是否正常
   caget LLRF:BPM:RF3Amp
   # 测试新PV
   caget LLRF:BPM:Temp1
   ```

4. **性能监控**：修改后监控性能
   ```bash
   top -p $(pidof st.cmd)
   ```

## 🎯 本周重点

**核心技能**：
- 添加新PV
- 修改数据采集逻辑
- 调试代码

**核心理解**：
- 代码修改的完整流程
- 修改对系统的影响
- 调试的方法和思路

---

**下一周**: [06-week5.md](./06-week5.md) - 设备支持层

**本周时间**: 15-20小时
- 工作日: 2-3小时/天 × 5 = 10-15小时
- 周末: 3-4小时/天 × 2 = 6-8小时

**学习方法**: 70%动手实践 + 20%理论学习 + 10%问题排查
