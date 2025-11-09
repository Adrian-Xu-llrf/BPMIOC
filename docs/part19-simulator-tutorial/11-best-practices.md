# Part 19.11: Mock库开发最佳实践

> **目标**: 掌握Mock库开发的最佳实践
> **难度**: ⭐⭐⭐☆☆
> **时间**: 30分钟
> **用途**: 提高代码质量和开发效率

## 📖 内容概览

本文档总结Mock库开发的最佳实践：
- 代码组织
- 数据生成原则
- 性能优化
- 测试策略
- 维护建议

这些是经验总结，帮助你写出高质量的Mock库。

---

## 1. 设计原则

### 1.1 核心原则

**原则1: 接口一致性**
> Mock库和Real库必须有相同的API

```c
// ✅ 好：Mock和Real接口完全一致
// libbpm_mock.h 和 libbpm_zynq.h
float GetRFInfo(int channel, int type);

// ❌ 坏：Mock库有不同的接口
float MockGetRFInfo(int channel, int type, int mode);
```

**为什么重要**:
- driverWrapper无需修改即可切换库
- 代码在PC和ZYNQ上行为一致

---

**原则2: 数据真实感**
> Mock数据应接近真实硬件的行为

```c
// ✅ 好：包含漂移和噪声
float value = base + drift + variation + noise;

// ❌ 坏：固定值
float value = 1.0;
```

**为什么重要**:
- 能发现真实硬件可能遇到的问题
- 测试IOC的容错能力

---

**原则3: 配置灵活性**
> 允许通过配置调整Mock行为

```c
// ✅ 好：从配置文件读取参数
LoadConfig("mock_config.ini");

// ❌ 坏：硬编码所有参数
#define BASE_AMPLITUDE 1.0  // 无法修改
```

**为什么重要**:
- 适应不同的测试场景
- 无需重新编译即可调整

---

### 1.2 分层设计

```
┌─────────────────────────────┐
│  Public API Layer           │  ← driverWrapper调用
│  (GetRFInfo, GetXYPosition) │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│  Data Generation Layer      │  ← 核心算法
│  (generateRfAmplitude, etc) │
└──────────────┬──────────────┘
               │
┌──────────────▼──────────────┐
│  Configuration Layer        │  ← 参数管理
│  (g_config, g_rf_channels)  │
└─────────────────────────────┘
```

**好处**:
- 职责清晰
- 易于测试
- 方便修改

---

## 2. 代码组织

### 2.1 文件结构

**推荐结构**:

```
simulator/
├── src/
│   ├── libbpm_mock.c          # 主实现（API接口）
│   ├── rf_simulator.c         # RF数据生成
│   ├── xy_simulator.c         # XY位置生成
│   ├── button_simulator.c     # Button信号生成
│   ├── config.c               # 配置管理
│   └── utils.c                # 工具函数
├── include/
│   ├── libbpm_mock.h          # 公共API
│   └── mock_internal.h        # 内部接口
├── test/
│   ├── test_rf.c              # RF测试
│   ├── test_xy.c              # XY测试
│   └── test_integration.c     # 集成测试
└── config/
    └── mock_config.ini        # 默认配置
```

**好处**:
- 模块化，易于维护
- 每个文件专注一个功能
- 便于团队协作

---

### 2.2 函数命名规范

**公共API**:
```c
// 规范：动词 + 名词，简洁明了
float GetRFInfo(int channel, int type);
int SetReg(int addr, int value);
int TriggerAllDataReached(void);
```

**内部函数**:
```c
// 规范：前缀区分，描述性强
static float generateRfAmplitude(RfChannelConfig *cfg, double time);
static void updateXYTrajectory(XYPositionConfig *cfg, double time);
static double addWhiteNoise(double value, double noise_level);
```

---

### 2.3 注释规范

```c
/**
 * @brief 获取RF信号信息
 *
 * @param channel RF通道号（3-6对应RF3-RF6）
 * @param type    数据类型（0=幅度，1=相位，2=实部，3=虚部）
 *
 * @return 浮点数值，无效参数返回0.0
 *
 * @note 调用前需要先调用TriggerAllDataReached()更新数据
 *
 * @example
 *   float amp = GetRFInfo(3, 0);  // 获取RF3幅度
 */
float GetRFInfo(int channel, int type);
```

**要点**:
- 使用Doxygen格式
- 说明参数和返回值
- 提供使用示例
- 注明注意事项

---

## 3. 数据生成最佳实践

### 3.1 分层生成法

**推荐方法**:

```c
float generateRfAmplitude(RfChannelConfig *cfg, double time) {
    // 第1层：基准值
    float base = cfg->base_amplitude;

    // 第2层：长期漂移（周期：分钟）
    float drift = cfg->drift_rate * time;

    // 第3层：慢变化（周期：秒）
    float slow_var = cfg->amp_variation_amplitude *
                     sin(2.0 * M_PI * cfg->amp_variation_freq * time);

    // 第4层：快变化（周期：毫秒）
    float fast_var = cfg->fast_modulation_amp *
                     sin(2.0 * M_PI * cfg->fast_modulation_freq * time);

    // 第5层：随机噪声
    float noise = addWhiteNoise(0.0, cfg->amp_noise_level);

    // 组合所有层
    return base + drift + slow_var + fast_var + noise;
}
```

**好处**:
- 可以单独调整每一层
- 易于理解和调试
- 接近真实物理过程

---

### 3.2 参数范围设定

**原则：参考真实硬件**

| 参数 | 真实范围 | Mock范围 | 注意事项 |
|------|---------|----------|----------|
| RF幅度 | 0.9-1.1 | 0.8-1.2 | 留有余量测试边界 |
| RF相位 | ±π | ±π | 严格一致 |
| XY位置 | ±0.2mm | ±0.5mm | 稍大范围测试 |
| 噪声 | 0.1% | 0.1%-1% | 可配置 |

**示例配置**:

```ini
[RF3]
base_amplitude = 1.0
amp_variation_amplitude = 0.02  # ±2%变化
amp_noise_level = 0.001         # 0.1%噪声
drift_rate = 0.0001             # 慢漂移
```

---

### 3.3 时间处理

**✅ 好的做法**:

```c
// 使用模拟时间，可控制
static double g_simulation_time = 0.0;

int TriggerAllDataReached(void) {
    g_simulation_time += g_time_increment;
    return 0;
}

// 数据生成使用模拟时间
float generateData(double time) {
    // time参数来自g_simulation_time
}
```

**❌ 避免的做法**:

```c
// 不要使用真实系统时间
float generateData(void) {
    time_t now = time(NULL);  // ❌ 不可控
    // ...
}
```

**为什么**:
- 模拟时间可控、可重复
- 便于测试和调试
- 可以暂停、快进

---

## 4. 性能优化

### 4.1 预计算

**优化前**:

```c
float generateRfAmplitude(...) {
    // 每次都计算sin/cos
    float var1 = sin(2.0 * M_PI * 0.1 * time);
    float var2 = sin(2.0 * M_PI * 1.0 * time);
    float var3 = cos(2.0 * M_PI * 0.5 * time);
    // ...
}
```

**优化后**:

```c
// 缓存计算结果
static double g_last_time = -1.0;
static float g_cached_var1 = 0.0;
static float g_cached_var2 = 0.0;

float generateRfAmplitude(...) {
    // 只在时间变化时重新计算
    if (time != g_last_time) {
        g_cached_var1 = sin(2.0 * M_PI * 0.1 * time);
        g_cached_var2 = sin(2.0 * M_PI * 1.0 * time);
        g_last_time = time;
    }

    // 使用缓存值
    float result = base + g_cached_var1 + g_cached_var2;
    // ...
}
```

**性能提升**: ~5x

---

### 4.2 避免重复计算

**优化前**:

```c
// 每个通道都计算噪声基数
float GetRFInfo(int channel, int type) {
    srand(time(NULL));  // ❌ 每次都重新设置种子
    float noise = (float)rand() / RAND_MAX;
    // ...
}
```

**优化后**:

```c
// 只在SystemInit时设置一次
int SystemInit(void) {
    srand(time(NULL));  // ✅ 只设置一次
    // ...
}

float GetRFInfo(int channel, int type) {
    float noise = (float)rand() / RAND_MAX;
    // ...
}
```

---

### 4.3 内存分配

**✅ 推荐**:

```c
// 使用静态分配
static float g_waveform_buffer[100000];

int ReadWaveform(..., float *output, int size) {
    // 生成数据到静态buffer
    // 然后复制到output
    memcpy(output, g_waveform_buffer, size * sizeof(float));
}
```

**❌ 避免**:

```c
// 避免频繁动态分配
int ReadWaveform(..., float *output, int size) {
    float *temp = malloc(size * sizeof(float));  // ❌ 每次都分配
    // ...
    free(temp);
}
```

---

## 5. 错误处理

### 5.1 参数验证

**总是验证输入参数**:

```c
float GetRFInfo(int channel, int type) {
    // 验证channel
    if (channel < 3 || channel > 6) {
        LOG(LOG_ERROR, "Invalid channel: %d (expected 3-6)\n", channel);
        return 0.0f;
    }

    // 验证type
    if (type < 0 || type > 3) {
        LOG(LOG_ERROR, "Invalid type: %d (expected 0-3)\n", type);
        return 0.0f;
    }

    // 验证初始化
    if (!g_config.initialized) {
        LOG(LOG_ERROR, "Mock library not initialized\n");
        return 0.0f;
    }

    // ... 正常逻辑
}
```

---

### 5.2 优雅降级

```c
float GetRFInfo(int channel, int type) {
    // ...参数验证

    float value = generateRfAmplitude(cfg, time);

    // 检查数值有效性
    if (isnan(value) || isinf(value)) {
        LOG(LOG_WARN, "Generated invalid value, using default\n");
        return cfg->base_amplitude;  // 返回默认值
    }

    // 范围限制
    if (value < 0.0f) value = 0.0f;
    if (value > 10.0f) value = 10.0f;

    return value;
}
```

---

## 6. 测试策略

### 6.1 单元测试

**测试每个数据生成函数**:

```c
// test_rf_generator.c
void test_rf_amplitude_range(void) {
    RfChannelConfig cfg = {
        .base_amplitude = 1.0,
        .amp_variation_amplitude = 0.02,
        .amp_noise_level = 0.01
    };

    // 测试1000个时间点
    for (int i = 0; i < 1000; i++) {
        double time = i * 0.1;
        float amp = generateRfAmplitude(&cfg, time);

        // 验证范围
        assert(amp >= 0.8 && amp <= 1.2);
        printf(".");
    }
    printf(" PASS\n");
}
```

---

### 6.2 集成测试

**测试完整的API调用序列**:

```c
// test_integration.c
void test_complete_workflow(void) {
    // 初始化
    assert(SystemInit() == 0);

    // 100个采集周期
    for (int cycle = 0; cycle < 100; cycle++) {
        TriggerAllDataReached();

        // 验证所有数据源
        for (int ch = 3; ch <= 6; ch++) {
            float amp = GetRFInfo(ch, 0);
            assert(!isnan(amp) && !isinf(amp));
        }

        for (int ch = 0; ch < 8; ch++) {
            float pos = GetXYPosition(ch);
            assert(pos >= -1.0 && pos <= 1.0);
        }
    }

    SystemClose();
    printf("Integration test PASS\n");
}
```

---

### 6.3 性能测试

```c
void test_performance(void) {
    SystemInit();

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // 调用10000次
    for (int i = 0; i < 10000; i++) {
        GetRFInfo(3, 0);
    }

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000000.0 +
                     (end.tv_usec - start.tv_usec);

    double us_per_call = elapsed / 10000.0;
    printf("Performance: %.2f μs/call\n", us_per_call);

    // 验证性能目标：< 10μs/call
    assert(us_per_call < 10.0);
}
```

---

## 7. 版本管理

### 7.1 添加版本信息

```c
// libbpm_mock.h
#define MOCK_VERSION_MAJOR 1
#define MOCK_VERSION_MINOR 0
#define MOCK_VERSION_PATCH 0
#define MOCK_VERSION_STRING "1.0.0"

// 新增API
const char* GetMockVersion(void);
```

```c
// libbpm_mock.c
const char* GetMockVersion(void) {
    return MOCK_VERSION_STRING;
}
```

**使用**:

```c
printf("Mock library version: %s\n", GetMockVersion());
```

---

### 7.2 兼容性保证

**规则**:
- 不要删除公共API函数
- 不要修改现有函数的签名
- 新功能通过新函数添加

```c
// ✅ 好：添加新函数
float GetRFInfo_v2(int channel, int type, int flags);

// ❌ 坏：修改现有函数
float GetRFInfo(int channel, int type, int flags);  // 破坏兼容性
```

---

## 8. 文档维护

### 8.1 保持文档更新

**每次修改代码时**:
- [ ] 更新函数注释
- [ ] 更新API文档
- [ ] 更新使用示例
- [ ] 更新CHANGELOG

---

### 8.2 CHANGELOG示例

```markdown
# Changelog

## [1.1.0] - 2024-01-15
### Added
- 新增故障注入功能
- 新增场景回放功能
- 新增性能统计API

### Changed
- 优化RF数据生成算法，性能提升3倍
- 改进噪声生成的随机性

### Fixed
- 修复XY位置边界检查bug
- 修复Button信号计算错误

## [1.0.0] - 2024-01-01
### Added
- 初始版本发布
- 基本RF/XY/Button数据生成
- 寄存器读写
```

---

## 9. 协作开发

### 9.1 代码审查清单

**提交代码前检查**:

- [ ] 代码符合命名规范
- [ ] 添加了必要的注释
- [ ] 参数验证完整
- [ ] 错误处理完善
- [ ] 添加了单元测试
- [ ] 通过所有测试
- [ ] 更新了文档
- [ ] 性能符合要求

---

### 9.2 Git提交规范

```bash
# 好的提交信息
git commit -m "feat: 添加故障注入功能"
git commit -m "fix: 修复RF相位计算bug"
git commit -m "perf: 优化数据生成性能（3x faster）"
git commit -m "docs: 更新API文档"

# 不好的提交信息
git commit -m "更新"
git commit -m "修复bug"
git commit -m "一些改动"
```

**格式**:
```
<type>: <subject>

<body>

<footer>
```

**Type**:
- `feat`: 新功能
- `fix`: Bug修复
- `perf`: 性能优化
- `docs`: 文档更新
- `refactor`: 重构
- `test`: 测试相关

---

## 10. 常见陷阱

### 10.1 时间相关问题

**❌ 陷阱**:
```c
// 使用真实时间 - 不可重复
float value = sin(time(NULL));
```

**✅ 解决**:
```c
// 使用模拟时间 - 可控制、可重复
float value = sin(g_simulation_time);
```

---

### 10.2 随机数问题

**❌ 陷阱**:
```c
// 每次都重置种子 - 伪随机性差
srand(42);
float noise = rand();
```

**✅ 解决**:
```c
// 在SystemInit中设置一次种子
int SystemInit(void) {
    srand(time(NULL));
    // ...
}

// 使用时直接调用
float noise = (float)rand() / RAND_MAX;
```

---

### 10.3 浮点数比较

**❌ 陷阱**:
```c
if (value == 1.0) {  // 可能永远不相等
    // ...
}
```

**✅ 解决**:
```c
#define EPSILON 1e-6

if (fabs(value - 1.0) < EPSILON) {
    // ...
}
```

---

## 11. 部署建议

### 11.1 目录结构

**生产环境**:

```
/opt/bpmioc/
├── lib/
│   ├── libbpm_mock.so.1.0.0
│   ├── libbpm_mock.so.1 -> libbpm_mock.so.1.0.0
│   └── libbpm_mock.so -> libbpm_mock.so.1
├── config/
│   └── mock_config.ini
└── bin/
    └── test_mock
```

**开发环境**:

```
~/BPMIOC/simulator/
├── src/          # 源码
├── lib/          # 编译输出
├── test/         # 测试
└── config/       # 配置
```

---

### 11.2 配置管理

**分环境配置**:

```bash
# 开发环境
export MOCK_CONFIG=~/BPMIOC/simulator/config/mock_config_dev.ini

# 测试环境
export MOCK_CONFIG=/opt/bpmioc/config/mock_config_test.ini

# 生产环境（稳定参数）
export MOCK_CONFIG=/opt/bpmioc/config/mock_config_prod.ini
```

---

## 12. 检查清单

### 发布前检查

- [ ] **代码质量**
  - [ ] 无编译警告
  - [ ] 无内存泄漏（Valgrind检查）
  - [ ] 代码审查通过

- [ ] **测试**
  - [ ] 所有单元测试通过
  - [ ] 集成测试通过
  - [ ] 性能测试达标

- [ ] **文档**
  - [ ] API文档完整
  - [ ] 使用示例更新
  - [ ] CHANGELOG更新

- [ ] **兼容性**
  - [ ] 与BPMIOC IOC集成成功
  - [ ] 向后兼容（如果有旧版本）

---

## 13. 学习资源

### 推荐阅读

**Mock开发**:
- [01-how-to-write-simulator.md](./01-how-to-write-simulator.md) - 基础教程
- [05-complete-mock-implementation.md](./05-complete-mock-implementation.md) - 完整实现

**C语言最佳实践**:
- "The Practice of Programming" - Kernighan & Pike
- "C Programming: A Modern Approach" - K.N. King

**测试**:
- "Test Driven Development for Embedded C" - James Grenning

---

## 14. 总结

### 核心要点

✅ **设计原则**
- 接口一致性
- 数据真实感
- 配置灵活性

✅ **代码组织**
- 模块化设计
- 清晰的命名
- 完整的注释

✅ **数据生成**
- 分层生成法
- 合理的参数范围
- 可控的时间处理

✅ **质量保证**
- 完整的测试
- 严格的错误处理
- 性能优化

✅ **协作开发**
- 版本管理
- 文档维护
- 代码审查

---

### 最后的建议

**对于初学者**:
1. 先实现基本功能，再优化
2. 多写测试，少修复bug
3. 保持代码简单

**对于进阶开发者**:
1. 关注性能和可维护性平衡
2. 设计可扩展的架构
3. 分享经验，帮助他人

---

**🎓 记住**: 好的Mock库不仅能模拟硬件，还能发现设计中的问题。投入时间打磨Mock库是值得的！

---

**恭喜！** 你已经完成了Part 19的所有11个文档！

现在你掌握了：
- ✅ 如何从零开始编写模拟器
- ✅ BPMIOC Mock库的完整实现
- ✅ 编译、测试和调试技巧
- ✅ 与IOC的集成方法
- ✅ 完整的API参考
- ✅ 最佳实践和经验总结

**下一步**: 开始实践！编写你自己的Mock库，在PC上开发BPMIOC！🚀
