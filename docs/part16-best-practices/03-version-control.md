# 版本控制

> **目标**: 有效使用Git管理代码
> **难度**: ⭐⭐⭐
> **预计时间**: 1天

## Git工作流

### 分支策略

```
main (生产)
  ↑
develop (开发)
  ↑
feature/xxx (功能分支)
```

### 分支命名

```bash
# 功能分支
feature/add-snr-calculation
feature/support-10-channels

# Bug修复
bugfix/fix-memory-leak
bugfix/correct-phase-calculation

# 发布分支
release/v1.0.0
release/v1.1.0

# 热修复
hotfix/critical-bug-fix
```

## Commit规范

### Commit消息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type类型

- **feat**: 新功能
- **fix**: Bug修复
- **docs**: 文档更新
- **style**: 代码格式（不影响功能）
- **refactor**: 重构
- **test**: 测试相关
- **chore**: 构建/工具相关

### 示例

```bash
# 好的commit
git commit -m "feat(driver): add SNR calculation function

Implement CalculateSNR() using log10 to compute signal-to-noise ratio.
Formula: SNR = 20 * log10(signal / noise)

Closes #123"

# 不好的commit
git commit -m "update"
git commit -m "fix bug"
```

## .gitignore

```bash
# EPICS生成文件
O.*/
bin/
lib/
dbd/
db/
*.db.d

# 编译产物
*.o
*.a
*.so
*.d

# 编辑器
.vscode/
.idea/
*.swp
*~

# 系统文件
.DS_Store
Thumbs.db

# 日志
*.log

# 备份
*.bak
*.backup
```

## Git常用操作

### 创建分支

```bash
# 从develop创建功能分支
git checkout develop
git pull
git checkout -b feature/add-new-pv

# 开发...

# 提交
git add .
git commit -m "feat(database): add new PV for temperature"
git push -u origin feature/add-new-pv
```

### 合并流程

```bash
# 更新develop
git checkout develop
git pull

# 合并feature分支
git merge --no-ff feature/add-new-pv

# 删除feature分支
git branch -d feature/add-new-pv
git push origin --delete feature/add-new-pv
```

## 🔗 相关文档

- [04-documentation.md](./04-documentation.md)
- [05-code-review.md](./05-code-review.md)
