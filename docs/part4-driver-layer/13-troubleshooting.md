# 常见问题排查

> **阅读时间**: 20分钟
> **难度**: ⭐⭐⭐☆☆

## 📋 问题分类

```
驱动层问题
├─ 初始化问题
├─ 数据采集问题
├─ 性能问题
└─ 库加载问题
```

## 1. 初始化问题

### 问题1: dlopen失败

**现象**:
```
ERROR: Cannot load library: libbpm_mock.so: cannot open shared object file
```

**原因**:
- 库文件不存在
- 路径不正确
- LD_LIBRARY_PATH未设置

**解决方案**:
```bash
# 检查文件
ls -l ./libbpm_mock.so

# 使用绝对路径
handle = dlopen("/full/path/to/libbpm_mock.so", RTLD_LAZY);

# 设置LD_LIBRARY_PATH
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

### 问题2: dlsym找不到符号

**现象**:
```
ERROR: Cannot find symbol SystemInit
```

**原因**:
- 函数名拼写错误
- 库中未导出该函数
- 库版本不匹配

**解决方案**:
```bash
# 检查库中的符号
nm -D libbpm_mock.so | grep SystemInit

# 如果没有，检查源码
grep "SystemInit" libbpm_mock.c

# 确保函数可见性
__attribute__((visibility("default"))) int SystemInit(void);
```

### 问题3: pthread创建失败

**现象**:
```
ERROR: Failed to create thread: 11 (Resource temporarily unavailable)
```

**原因**:
- 系统资源不足
- 线程数量达到上限

**解决方案**:
```bash
# 检查线程限制
ulimit -u

# 增加限制
ulimit -u 4096

# 或在代码中检查
if (pthread_create(&tidp1, NULL, pthread, NULL) != 0) {
    perror("pthread_create");
    // 处理错误
}
```

## 2. 数据采集问题

### 问题4: 数据全是0

**现象**:
```
caget LLRF:BPM:RF3Amp
LLRF:BPM:RF3Amp 0
```

**原因**:
- funcTriggerAllDataReached()未执行
- 硬件函数返回0
- 缓冲区未更新

**调试**:
```c
void *pthread(void *arg)
{
    while (1) {
        printf("Before trigger\n");
        int ret = funcTriggerAllDataReached();
        printf("Trigger returned: %d\n", ret);

        // 检查缓冲区
        printf("rf3amp[0] = %.3f\n", rf3amp[0]);

        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

### 问题5: 数据不更新

**现象**:
数据读到初始值后不再变化

**原因**:
- pthread线程未运行
- scanIoRequest()未触发
- Record未注册I/O Intr

**调试**:
```c
// 添加心跳计数
static unsigned long heartbeat = 0;

void *pthread(void *arg)
{
    while (1) {
        heartbeat++;
        printf("Heartbeat: %lu\n", heartbeat);

        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);
        usleep(100000);
    }
}
```

### 问题6: 波形数据混乱

**现象**:
波形包含NaN或Inf值

**原因**:
- 缓冲区未初始化
- 数据类型转换错误
- 线程安全问题

**解决方案**:
```c
// 初始化缓冲区
memset(rf3amp, 0, sizeof(rf3amp));

// 检查数据
for (int i = 0; i < buf_len; i++) {
    if (isnan(rf3amp[i]) || isinf(rf3amp[i])) {
        printf("ERROR: Invalid data at [%d]\n", i);
        rf3amp[i] = 0.0;  // 清除
    }
}
```

## 3. 性能问题

### 问题7: CPU占用过高

**现象**:
IOC进程CPU占用100%

**原因**:
- usleep()太短
- 死循环
- 硬件函数耗时太长

**调试**:
```c
void *pthread(void *arg)
{
    while (1) {
        struct timeval start, end;
        gettimeofday(&start, NULL);

        funcTriggerAllDataReached();
        scanIoRequest(TriginScanPvt);

        gettimeofday(&end, NULL);
        long us = (end.tv_sec - start.tv_sec) * 1000000 +
                  (end.tv_usec - start.tv_usec);

        printf("Loop took %ld us\n", us);

        usleep(100000);
    }
}
```

### 问题8: 内存泄漏

**现象**:
IOC内存占用持续增长

**原因**:
- malloc未释放
- 缓冲区重复分配

**调试**:
```bash
valgrind --leak-check=full ./st.cmd

# 查看内存增长
watch -n 1 'ps aux | grep st.cmd'
```

## 4. 网络问题

### 问题9: CA无法连接

**现象**:
```
caget LLRF:BPM:RF3Amp
Channel connect timed out
```

**原因**:
- 防火墙阻止5064/5065端口
- EPICS_CA_ADDR_LIST未设置
- IOC未启动

**解决方案**:
```bash
# 检查IOC是否运行
ps aux | grep st.cmd

# 检查端口
netstat -an | grep 5064

# 设置环境变量
export EPICS_CA_ADDR_LIST=192.168.1.100
export EPICS_CA_AUTO_ADDR_LIST=NO

# 重试
caget LLRF:BPM:RF3Amp
```

### 问题10: PV not found

**现象**:
```
caget LLRF:BPM:RF3Amp
Channel connect timed out: 'LLRF:BPM:RF3Amp' not found.
```

**原因**:
- PV名称拼写错误
- 数据库未加载
- Record定义有误

**解决方案**:
```bash
# 在IOC中检查
epics> dbl | grep RF3Amp

# 检查数据库文件
cat ~/BPMIOC/BPMmonitorApp/Db/BPMMonitor.db | grep RF3Amp

# 重新加载数据库
epics> dbLoadRecords("db/BPMMonitor.db")
```

## 5. 快速排查清单

### 5.1 IOC启动检查

```bash
☐ 动态库存在
  ls -l libbpm_mock.so

☐ 库符号正确
  nm -D libbpm_mock.so | grep -E "SystemInit|GetRFInfo"

☐ 初始化成功
  grep "System initialized" ioc.log

☐ 线程运行
  ps -eLf | grep st.cmd
```

### 5.2 数据采集检查

```bash
☐ pthread运行
  # 检查心跳计数

☐ 数据触发
  # 检查funcTriggerAllDataReached返回值

☐ 缓冲区更新
  # 打印rf3amp[0]

☐ scanIoRequest触发
  # 检查Record更新时间
```

### 5.3 网络访问检查

```bash
☐ IOC运行
  ps aux | grep st.cmd

☐ 端口监听
  netstat -an | grep 5064

☐ PV存在
  epics> dbl | grep <PV_NAME>

☐ CA连接
  caget -n <PV_NAME>
```

## 6. 常用调试命令

### IOC内部命令

```bash
# 列出所有PV
epics> dbl

# 打印Record详细信息
epics> dbpr LLRF:BPM:RF3Amp 3

# 手动处理Record
epics> dbpf LLRF:BPM:RF3Amp.PROC 1

# 查看IOC统计
epics> dbior

# 查看变量
epics> var debug_level
```

### CA工具命令

```bash
# 获取PV值
caget LLRF:BPM:RF3Amp

# 设置PV值
caput LLRF:BPM:SetGain 50

# 监控PV变化
camonitor LLRF:BPM:RF3Amp

# 获取PV信息
cainfo LLRF:BPM:RF3Amp
```

### 系统命令

```bash
# 查看进程
ps aux | grep st.cmd

# 查看线程
ps -eLf | grep st.cmd

# 查看网络
netstat -an | grep 5064

# 查看文件
lsof -p <PID>

# 查看内存
top -p <PID>
```

## 📚 延伸阅读

- [12-debugging.md](./12-debugging.md) - 调试技巧
- EPICS FAQ: https://epics.anl.gov/faq/

## 🎓 本章总结

- ✅ 掌握常见问题的排查方法
- ✅ 使用快速检查清单定位问题
- ✅ 熟悉调试命令和工具

---

**建议**: 将常见问题和解决方案记录到日志中，便于后续查阅
