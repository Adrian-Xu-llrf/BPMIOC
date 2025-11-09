# 代码审查

> **目标**: 建立有效的代码审查流程
> **难度**: ⭐⭐⭐
> **预计时间**: 1天

## 代码审查流程

### Pull Request流程

```
1. 开发者创建PR
   ↓
2. 自动CI/CD检查
   ↓
3. 团队成员审查
   ↓
4. 反馈和修改
   ↓
5. 批准合并
```

### PR模板

```markdown
## 变更说明
简要描述本次变更的内容和目的。

## 变更类型
- [ ] Bug修复
- [ ] 新功能
- [ ] 性能优化
- [ ] 重构
- [ ] 文档更新

## 测试
- [ ] 单元测试已通过
- [ ] 集成测试已通过
- [ ] 手动测试已完成

## 检查清单
- [ ] 代码符合规范
- [ ] 添加了必要的注释
- [ ] 更新了文档
- [ ] 没有引入编译警告
- [ ] 通过静态分析

## 相关Issue
Closes #123
```

## 审查要点

### 功能正确性

```c
// 审查：边界条件是否处理？
float ReadData(int offset, int channel, int type) {
    // ✓ 检查channel范围
    if (channel < 0 || channel >= MAX_RF_CHANNELS) {
        return 0.0;
    }
    
    // ✓ 检查offset有效性
    if (offset >= NUM_OFFSETS) {
        return 0.0;
    }
    
    return g_data_buffer[offset][channel];
}
```

### 代码质量

```c
// 审查：函数是否过长？是否职责单一？
// 不好：一个函数做太多事
void ProcessData() {
    ReadFromHardware();
    ValidateData();
    CalculateResults();
    UpdateDatabase();
    SendNotification();
}

// 好：拆分为多个函数
void ProcessData() {
    float* data = ReadFromHardware();
    if (!ValidateData(data)) {
        return;
    }
    Results* results = CalculateResults(data);
    UpdateDatabase(results);
}
```

### 性能影响

```c
// 审查：是否有性能问题？
// 不好：每次都重新计算
for (int i = 0; i < count; i++) {
    float value = ExpensiveCalculation();  // 重复计算
    Use(value);
}

// 好：缓存结果
float value = ExpensiveCalculation();
for (int i = 0; i < count; i++) {
    Use(value);
}
```

## 审查反馈

### 建设性反馈

```
不好的反馈：
"这代码写得太烂了"

好的反馈：
"建议将这个200行的函数拆分成几个小函数，每个函数
负责一个清晰的职责，这样更容易理解和测试。"
```

### 提问而非命令

```
命令式：
"这里必须用mutex"

提问式：
"这里是否需要考虑线程安全？在多线程环境下可能有竞态条件"
```

## 自动化检查

### GitHub Actions

```yaml
# .github/workflows/pr-check.yml
name: PR Checks

on: [pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build
      run: make
    - name: Run tests
      run: make test
    - name: Static analysis
      run: cppcheck --enable=all src/
```

## 🔗 相关文档

- [01-coding-standards.md](./01-coding-standards.md)
- [06-testing-strategy.md](./06-testing-strategy.md)
