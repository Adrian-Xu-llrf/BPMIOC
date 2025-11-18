# EPICS IOC 开发学习路径

**适合人群**: 想要成为 IOC 开发者，但目前没有 C 语言和硬件基础

**学习周期**: 3 个月（每天投入 2-3 小时）

**最终目标**: 能够独立开发 EPICS Device Support，实现 IOC 与 FPGA 的交互

---

## 📁 学习路径结构

```
LearningPath/
├── README.md                          # 本文件 - 总体说明
│
├── Month1-C-Basics/                   # 第1个月：C 语言基础
│   ├── Week1-Basic-Syntax.md          # 第1周：基础语法
│   ├── Week2-Pointers.md              # 第2周：指针详解
│   ├── Week3-Structs-Functions.md     # 第3周：结构体和函数
│   ├── Week4-Practice.md              # 第4周：综合练习
│   └── exercises/                     # 练习代码
│
├── Month2-Linux-EPICS/                # 第2个月：Linux 和 EPICS
│   ├── Week1-Linux-Basics.md          # 第1周：Linux 基础
│   ├── Week2-EPICS-Installation.md    # 第2周：EPICS 安装
│   ├── Week3-Simple-IOC.md            # 第3周：简单 IOC
│   ├── Week4-Database-PV.md           # 第4周：数据库和 PV
│   └── examples/                      # 示例代码
│
└── Month3-IOC-Development/            # 第3个月：IOC 开发
    ├── Week1-EPICS-Architecture.md    # 第1周：EPICS 架构深入
    ├── Week2-Device-Support.md        # 第2周：Device Support
    ├── Week3-Driver-Support.md        # 第3周：Driver Support
    ├── Week4-Complete-Project.md      # 第4周：完整项目
    └── project/                       # 项目代码
```

---

## 🎯 学习目标

### 第1个月结束时，你应该能够：
- ✅ 读懂基本的 C 语言代码
- ✅ 理解指针的概念和使用
- ✅ 能够编写简单的 C 程序
- ✅ 理解结构体和函数

### 第2个月结束时，你应该能够：
- ✅ 熟练使用 Linux 命令行
- ✅ 成功安装和配置 EPICS Base
- ✅ 运行和修改简单的 IOC
- ✅ 定义 PV 和数据库文件

### 第3个月结束时，你应该能够：
- ✅ 理解 EPICS 分层架构
- ✅ 编写简单的 Device Support
- ✅ 集成硬件库到 Driver Support
- ✅ 完成一个完整的 IOC 项目

---

## 📅 详细时间规划

### 第1个月：C 语言基础

| 周次 | 主题 | 时间投入 | 重点内容 |
|------|------|----------|----------|
| Week 1 | 基础语法 | 14 小时 | 变量、数据类型、控制流程 |
| Week 2 | 指针 | 14 小时 | 指针概念、指针运算、数组指针 |
| Week 3 | 结构体和函数 | 14 小时 | 函数定义、结构体、typedef |
| Week 4 | 综合练习 | 14 小时 | 实战项目、调试技巧 |

**总计**: 56 小时

### 第2个月：Linux 和 EPICS

| 周次 | 主题 | 时间投入 | 重点内容 |
|------|------|----------|----------|
| Week 1 | Linux 基础 | 14 小时 | 命令行、文件操作、gcc/make |
| Week 2 | EPICS 安装 | 14 小时 | 编译 Base、配置环境 |
| Week 3 | 简单 IOC | 14 小时 | softIoc、基本 record |
| Week 4 | 数据库和 PV | 14 小时 | .db 文件、CA 工具 |

**总计**: 56 小时

### 第3个月：IOC 开发

| 周次 | 主题 | 时间投入 | 重点内容 |
|------|------|----------|----------|
| Week 1 | EPICS 架构 | 14 小时 | Record、dset、drvet |
| Week 2 | Device Support | 14 小时 | 编写 Device Support |
| Week 3 | Driver Support | 14 小时 | 硬件库集成 |
| Week 4 | 完整项目 | 14 小时 | 从零搭建 IOC |

**总计**: 56 小时

**三个月总计**: 168 小时（约 2.5 小时/天）

---

## 🚀 如何使用这些教程

### 每周的学习流程

```
周一到周五（工作日）
├─ 每天 2 小时
│  ├─ 1 小时：学习理论（阅读教程）
│  └─ 1 小时：动手实践（写代码）
│
周末
└─ 每天 3 小时
   ├─ 1.5 小时：复习本周内容
   └─ 1.5 小时：完成本周练习
```

### 每个教程文件的结构

```
# Week X - 主题

## 学习目标
- 本周结束时应该掌握什么

## 理论知识
- 详细讲解概念
- 图示和代码示例

## 实践练习
- 分步指导的练习
- 可运行的代码示例

## 自我检查
- 验证是否掌握
- 常见错误

## 下一步
- 指向下周内容
```

---

## 💡 学习建议

### ✅ 好的学习习惯

1. **每天坚持**: 每天 2-3 小时比周末集中学习效果好
2. **动手实践**: 所有代码都要自己打一遍（不要复制粘贴）
3. **做笔记**: 记录不懂的地方，定期回顾
4. **寻求帮助**: 遇到问题及时查资料或提问
5. **循序渐进**: 不要跳过基础，一步一步来

### ❌ 避免的陷阱

1. ❌ 只看不练：光看教程不写代码
2. ❌ 追求完美：过度纠结细节，影响进度
3. ❌ 跳过基础：C 语言基础很重要，不要跳过
4. ❌ 孤军奋战：找个学习伙伴或加入社区
5. ❌ 三天打鱼：学习需要持续投入

---

## 📖 推荐资源

### 书籍

**C 语言**:
- 《C Primer Plus》（第6版）- 最推荐
- 《C 程序设计语言》（K&R）- 经典参考

**EPICS**:
- EPICS Application Developer's Guide
- EPICS Record Reference Manual

### 在线课程

- 中国大学 MOOC - C 语言程序设计
- Coursera - C Programming
- YouTube - EPICS Training Videos

### 工具

- **编辑器**: VS Code、Vim、Sublime Text
- **编译器**: GCC（Linux）
- **终端**: Linux 命令行或 WSL（Windows）
- **版本控制**: Git

---

## ✅ 阶段性检查点

### 第1个月结束

**你应该能够**:
```c
// 1. 理解这段代码
struct Point {
    float x;
    float y;
};

void movePoint(struct Point *p, float dx, float dy) {
    p->x += dx;
    p->y += dy;
}

int main() {
    struct Point p1 = {0.0, 0.0};
    movePoint(&p1, 1.0, 2.0);
    printf("Point: (%.1f, %.1f)\n", p1.x, p1.y);
    return 0;
}
```

### 第2个月结束

**你应该能够**:
```bash
# 1. 编译和运行 IOC
$ cd MyIOC/iocBoot/iocMyIOC
$ ./st.cmd

# 2. 使用 CA 工具
$ caget MyPV:Value
$ caput MyPV:Value 123
```

### 第3个月结束

**你应该能够**:
- 从零创建一个 IOC 项目
- 编写 Device Support 读取硬件数据
- 定义 PV 并测试功能
- 调试和解决问题

---

## 🎓 完成证明

完成所有学习后，你可以：

1. **提交一个完整的 IOC 项目**到 GitHub
2. **写一篇技术博客**分享学习经验
3. **为 BPMIOC 项目贡献代码**
4. **帮助其他初学者**

---

## 📞 获取帮助

**遇到问题时**：

1. **查看文档**: 先查阅相关教程和官方文档
2. **搜索引擎**: Google/Bing 搜索错误信息
3. **EPICS 社区**:
   - Tech-Talk 邮件列表
   - GitHub Issues
4. **本地社区**:
   - 实验室同事
   - EPICS 用户组

---

## 🚦 开始学习

**立即开始**：

```bash
# 1. 进入第1个月目录
cd LearningPath/Month1-C-Basics/

# 2. 阅读第1周教程
cat Week1-Basic-Syntax.md

# 3. 开始学习！
```

**祝你学习顺利！记住：**
- 🐢 慢慢来，不要急
- 💪 坚持就是胜利
- 🤝 寻求帮助不可耻
- 🎯 目标明确，脚踏实地

---

**下一步**: 打开 `Month1-C-Basics/Week1-Basic-Syntax.md` 开始第1周的学习
