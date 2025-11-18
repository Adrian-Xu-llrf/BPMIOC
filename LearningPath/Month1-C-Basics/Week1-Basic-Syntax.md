# Week 1 - C 语言基础语法

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 掌握 C 语言最基础的概念，能够编写和运行简单的程序

---

## 第1天：认识 C 语言和第一个程序（2小时）

### 什么是 C 语言？

C 语言是一种**编程语言**，就像英语、中文是人类的语言一样，C 语言是用来和计算机"对话"的语言。

**为什么要学 C 语言？**
- EPICS IOC 的核心代码都是用 C 写的
- C 语言接近硬件，适合控制 FPGA 这类硬件设备
- 学会 C 语言，你就能理解和修改 IOC 代码

### 第一个程序：Hello World

```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

**逐行解释**：

```c
#include <stdio.h>
```
- `#include`：告诉编译器"我要用一个库"
- `stdio.h`：标准输入输出库（**st**an**d**ard **i**nput/**o**utput）
- 类比：就像你要用计算器，需要先拿出计算器一样

```c
int main() {
```
- `int`：这个函数会返回一个整数（**int**eger）
- `main`：程序的入口，程序从这里开始执行
- `()`：参数列表（现在是空的）
- `{`：函数体的开始

```c
    printf("Hello, World!\n");
```
- `printf`：**print** **f**ormatted，格式化输出函数
- `"Hello, World!"`：要输出的字符串
- `\n`：换行符（**n**ew line）
- `;`：语句结束符（非常重要！）

```c
    return 0;
```
- `return`：返回
- `0`：返回值为 0，表示程序正常结束

```c
}
```
- 函数体的结束

### 实践：编写、编译、运行

**步骤1：创建文件**
```bash
$ cd ~
$ mkdir c_learning
$ cd c_learning
$ nano hello.c
```

**步骤2：输入代码**（自己打，不要复制粘贴！）
```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

**步骤3：保存并退出**
- 按 `Ctrl + O`（保存）
- 按 `Enter`（确认文件名）
- 按 `Ctrl + X`（退出）

**步骤4：编译**
```bash
$ gcc hello.c -o hello
```
- `gcc`：GNU C Compiler（编译器）
- `hello.c`：源文件
- `-o hello`：输出文件名为 hello

**步骤5：运行**
```bash
$ ./hello
Hello, World!
```

**恭喜！你已经运行了第一个 C 程序！**

### 练习1.1
修改程序，让它输出你的名字：
```c
#include <stdio.h>

int main() {
    printf("My name is 张三\n");
    printf("I am learning C language!\n");
    return 0;
}
```

---

## 第2天：变量和数据类型（2小时）

### 什么是变量？

变量就是一个**有名字的盒子**，用来存储数据。

```c
#include <stdio.h>

int main() {
    int age;        // 声明一个整数变量
    age = 25;       // 给变量赋值

    printf("My age is %d\n", age);
    return 0;
}
```

**输出**：
```
My age is 25
```

### 基本数据类型

| 类型 | 关键字 | 大小 | 范围 | 示例 |
|------|--------|------|------|------|
| 整数 | `int` | 4字节 | -2147483648 到 2147483647 | `int count = 10;` |
| 浮点数 | `float` | 4字节 | ±3.4E-38 到 ±3.4E+38 | `float voltage = 3.14;` |
| 双精度浮点 | `double` | 8字节 | 更高精度 | `double pi = 3.141592653589793;` |
| 字符 | `char` | 1字节 | -128 到 127 | `char grade = 'A';` |

### 变量命名规则

**可以**：
- 使用字母、数字、下划线
- 以字母或下划线开头
- 例如：`age`, `my_age`, `age2`

**不可以**：
- 以数字开头（`2age` ❌）
- 使用关键字（`int` ❌）
- 包含空格（`my age` ❌）

**好的命名习惯**：
```c
int channel_count;      // 清晰易懂
int temp;               // 简短明了
int bpm_amplitude;      // 有意义的名字
```

**不好的命名**：
```c
int a;                  // 太简单，不知道是什么
int x123;               // 无意义
int MyVeryLongVariableName;  // 太长
```

### 实践：计算器程序

```c
#include <stdio.h>

int main() {
    int num1, num2, sum;

    num1 = 10;
    num2 = 20;
    sum = num1 + num2;

    printf("num1 = %d\n", num1);
    printf("num2 = %d\n", num2);
    printf("sum = %d\n", sum);

    return 0;
}
```

**输出**：
```
num1 = 10
num2 = 20
sum = 30
```

### 格式化输出

```c
int age = 25;
float voltage = 3.14;
char grade = 'A';

printf("Age: %d\n", age);           // %d 输出整数
printf("Voltage: %f\n", voltage);   // %f 输出浮点数
printf("Grade: %c\n", grade);       // %c 输出字符
printf("Voltage: %.2f\n", voltage); // %.2f 保留2位小数
```

**输出**：
```
Age: 25
Voltage: 3.140000
Grade: A
Voltage: 3.14
```

### 练习1.2

编写程序计算圆的面积：
```c
#include <stdio.h>

int main() {
    float radius, area;
    float pi = 3.14159;

    radius = 5.0;
    area = pi * radius * radius;

    printf("Radius: %.2f\n", radius);
    printf("Area: %.2f\n", area);

    return 0;
}
```

---

## 第3天：输入和运算符（2小时）

### 接收用户输入

```c
#include <stdio.h>

int main() {
    int age;

    printf("Please enter your age: ");
    scanf("%d", &age);

    printf("You are %d years old.\n", age);

    return 0;
}
```

**重点**：
- `scanf`：读取输入
- `"%d"`：期望输入一个整数
- `&age`：取地址符（第2周会详细讲）

### 算术运算符

```c
int a = 10, b = 3;
int sum, diff, prod, quot, rem;

sum = a + b;    // 加法：13
diff = a - b;   // 减法：7
prod = a * b;   // 乘法：30
quot = a / b;   // 除法：3（整数除法）
rem = a % b;    // 取模（求余数）：1
```

### 注意：整数除法

```c
int result1 = 10 / 3;       // 结果是 3（不是 3.333...）
float result2 = 10.0 / 3.0; // 结果是 3.333...
```

### 复合赋值运算符

```c
int x = 10;

x += 5;   // 等价于 x = x + 5;  结果：15
x -= 3;   // 等价于 x = x - 3;  结果：12
x *= 2;   // 等价于 x = x * 2;  结果：24
x /= 4;   // 等价于 x = x / 4;  结果：6
x++;      // 等价于 x = x + 1;  结果：7
x--;      // 等价于 x = x - 1;  结果：6
```

### 实践：简单计算器

```c
#include <stdio.h>

int main() {
    float num1, num2;
    float sum, diff, prod, quot;

    printf("Enter first number: ");
    scanf("%f", &num1);

    printf("Enter second number: ");
    scanf("%f", &num2);

    sum = num1 + num2;
    diff = num1 - num2;
    prod = num1 * num2;
    quot = num1 / num2;

    printf("\nResults:\n");
    printf("%.2f + %.2f = %.2f\n", num1, num2, sum);
    printf("%.2f - %.2f = %.2f\n", num1, num2, diff);
    printf("%.2f * %.2f = %.2f\n", num1, num2, prod);
    printf("%.2f / %.2f = %.2f\n", num1, num2, quot);

    return 0;
}
```

### 练习1.3

编写程序计算温度转换（摄氏度转华氏度）：
- 公式：F = C × 9/5 + 32

```c
#include <stdio.h>

int main() {
    float celsius, fahrenheit;

    printf("Enter temperature in Celsius: ");
    scanf("%f", &celsius);

    fahrenheit = celsius * 9.0 / 5.0 + 32.0;

    printf("%.2f°C = %.2f°F\n", celsius, fahrenheit);

    return 0;
}
```

---

## 第4天：条件语句 if-else（2小时）

### if 语句

```c
#include <stdio.h>

int main() {
    int age;

    printf("Enter your age: ");
    scanf("%d", &age);

    if (age >= 18) {
        printf("You are an adult.\n");
    }

    return 0;
}
```

### if-else 语句

```c
#include <stdio.h>

int main() {
    int number;

    printf("Enter a number: ");
    scanf("%d", &number);

    if (number % 2 == 0) {
        printf("%d is even.\n", number);
    } else {
        printf("%d is odd.\n", number);
    }

    return 0;
}
```

### 比较运算符

```c
int a = 10, b = 20;

a == b   // 等于：false (0)
a != b   // 不等于：true (1)
a > b    // 大于：false
a < b    // 小于：true
a >= b   // 大于等于：false
a <= b   // 小于等于：true
```

### if-else if-else 语句

```c
#include <stdio.h>

int main() {
    int score;

    printf("Enter your score: ");
    scanf("%d", &score);

    if (score >= 90) {
        printf("Grade: A\n");
    } else if (score >= 80) {
        printf("Grade: B\n");
    } else if (score >= 70) {
        printf("Grade: C\n");
    } else if (score >= 60) {
        printf("Grade: D\n");
    } else {
        printf("Grade: F\n");
    }

    return 0;
}
```

### 逻辑运算符

```c
int a = 10, b = 20;

// 逻辑与（AND）：两个条件都为真
if (a > 5 && b > 15) {
    printf("Both conditions are true\n");
}

// 逻辑或（OR）：至少一个条件为真
if (a > 15 || b > 15) {
    printf("At least one condition is true\n");
}

// 逻辑非（NOT）：取反
if (!(a > 15)) {
    printf("a is not greater than 15\n");
}
```

### 实践：BPM 幅度检查程序

```c
#include <stdio.h>

int main() {
    float amplitude;

    printf("Enter BPM amplitude (Volts): ");
    scanf("%f", &amplitude);

    if (amplitude < 0.0) {
        printf("Error: Amplitude cannot be negative!\n");
    } else if (amplitude >= 0.0 && amplitude < 0.1) {
        printf("Warning: Signal too weak\n");
    } else if (amplitude >= 0.1 && amplitude <= 1.0) {
        printf("OK: Normal signal level\n");
    } else {
        printf("Warning: Signal too strong\n");
    }

    return 0;
}
```

### 练习1.4

编写程序判断一个年份是否为闰年：
- 能被4整除但不能被100整除，或者能被400整除

```c
#include <stdio.h>

int main() {
    int year;

    printf("Enter a year: ");
    scanf("%d", &year);

    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        printf("%d is a leap year.\n", year);
    } else {
        printf("%d is not a leap year.\n", year);
    }

    return 0;
}
```

---

## 第5天：循环语句（2小时）

### while 循环

```c
#include <stdio.h>

int main() {
    int count = 1;

    while (count <= 5) {
        printf("Count: %d\n", count);
        count++;
    }

    return 0;
}
```

**输出**：
```
Count: 1
Count: 2
Count: 3
Count: 4
Count: 5
```

### for 循环

```c
#include <stdio.h>

int main() {
    int i;

    for (i = 1; i <= 5; i++) {
        printf("i = %d\n", i);
    }

    return 0;
}
```

**for 循环结构**：
```c
for (初始化; 条件; 更新) {
    // 循环体
}
```

### do-while 循环

```c
#include <stdio.h>

int main() {
    int count = 1;

    do {
        printf("Count: %d\n", count);
        count++;
    } while (count <= 5);

    return 0;
}
```

**区别**：`do-while` 至少执行一次

### 实践：计算总和

```c
#include <stdio.h>

int main() {
    int n, i, sum = 0;

    printf("Enter n: ");
    scanf("%d", &n);

    for (i = 1; i <= n; i++) {
        sum += i;
    }

    printf("Sum of 1 to %d = %d\n", n, sum);

    return 0;
}
```

### break 和 continue

```c
// break: 跳出循环
for (i = 1; i <= 10; i++) {
    if (i == 5) {
        break;  // 循环到 i=5 时停止
    }
    printf("%d ", i);
}
// 输出：1 2 3 4

// continue: 跳过本次循环
for (i = 1; i <= 10; i++) {
    if (i == 5) {
        continue;  // 跳过 i=5
    }
    printf("%d ", i);
}
// 输出：1 2 3 4 6 7 8 9 10
```

### 实践：BPM 数据采集模拟

```c
#include <stdio.h>

int main() {
    int channel;
    float amplitude;

    printf("Simulating BPM data acquisition...\n\n");

    for (channel = 0; channel < 4; channel++) {
        printf("Enter amplitude for channel %d: ", channel);
        scanf("%f", &amplitude);

        if (amplitude < 0) {
            printf("Error: Invalid amplitude! Stopping.\n");
            break;
        }

        printf("Channel %d: %.3f V\n\n", channel, amplitude);
    }

    return 0;
}
```

### 练习1.5

编写程序计算阶乘（n!）：
```c
#include <stdio.h>

int main() {
    int n, i;
    long long factorial = 1;

    printf("Enter a number: ");
    scanf("%d", &n);

    if (n < 0) {
        printf("Error: Factorial of negative number doesn't exist.\n");
    } else {
        for (i = 1; i <= n; i++) {
            factorial *= i;
        }
        printf("%d! = %lld\n", n, factorial);
    }

    return 0;
}
```

---

## 第6天：数组（2小时）

### 什么是数组？

数组是**相同类型数据的集合**，就像一排编号的盒子。

```c
int numbers[5];  // 声明一个包含5个整数的数组
```

### 数组的初始化

```c
// 方法1：声明后赋值
int arr[5];
arr[0] = 10;
arr[1] = 20;
arr[2] = 30;
arr[3] = 40;
arr[4] = 50;

// 方法2：声明时初始化
int arr[5] = {10, 20, 30, 40, 50};

// 方法3：部分初始化（其余为0）
int arr[5] = {10, 20};  // {10, 20, 0, 0, 0}

// 方法4：自动推断大小
int arr[] = {10, 20, 30, 40, 50};  // 大小为5
```

### 访问数组元素

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    printf("arr[0] = %d\n", arr[0]);  // 第一个元素：10
    printf("arr[2] = %d\n", arr[2]);  // 第三个元素：30
    printf("arr[4] = %d\n", arr[4]);  // 第五个元素：50

    return 0;
}
```

**重点**：数组下标从 0 开始！

### 遍历数组

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int i;

    printf("Array elements:\n");
    for (i = 0; i < 5; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    return 0;
}
```

### 实践：BPM 4通道数据

```c
#include <stdio.h>

int main() {
    float amplitudes[4];  // 4个通道的幅度
    int i;
    float sum = 0.0, average;

    // 读取4个通道的数据
    printf("Enter amplitudes for 4 channels:\n");
    for (i = 0; i < 4; i++) {
        printf("Channel %d: ", i);
        scanf("%f", &amplitudes[i]);
        sum += amplitudes[i];
    }

    // 计算平均值
    average = sum / 4.0;

    // 显示结果
    printf("\nResults:\n");
    for (i = 0; i < 4; i++) {
        printf("Channel %d: %.3f V\n", i, amplitudes[i]);
    }
    printf("Average: %.3f V\n", average);

    return 0;
}
```

### 多维数组

```c
// 二维数组：类似表格
int matrix[3][4];  // 3行4列

// 初始化
int matrix[3][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
};

// 访问元素
int value = matrix[1][2];  // 第2行第3列：7
```

### 练习1.6

编写程序找出数组中的最大值和最小值：

```c
#include <stdio.h>

int main() {
    int arr[10];
    int i, max, min;

    printf("Enter 10 numbers:\n");
    for (i = 0; i < 10; i++) {
        printf("Number %d: ", i + 1);
        scanf("%d", &arr[i]);
    }

    max = min = arr[0];

    for (i = 1; i < 10; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
        if (arr[i] < min) {
            min = arr[i];
        }
    }

    printf("\nMaximum: %d\n", max);
    printf("Minimum: %d\n", min);

    return 0;
}
```

---

## 第7天：综合练习和复习（2小时）

### 综合项目：简单的 BPM 监控系统

```c
#include <stdio.h>

#define NUM_CHANNELS 4
#define THRESHOLD_LOW 0.1
#define THRESHOLD_HIGH 1.0

int main() {
    float amplitudes[NUM_CHANNELS];
    int i;
    float sum = 0.0, average;
    int warning_count = 0;

    printf("=== BPM Monitoring System ===\n\n");

    // 1. 采集数据
    printf("Enter amplitudes for %d channels:\n", NUM_CHANNELS);
    for (i = 0; i < NUM_CHANNELS; i++) {
        printf("Channel %d (V): ", i);
        scanf("%f", &amplitudes[i]);

        // 验证输入
        if (amplitudes[i] < 0) {
            printf("Error: Invalid amplitude!\n");
            return 1;
        }

        sum += amplitudes[i];
    }

    // 2. 计算平均值
    average = sum / NUM_CHANNELS;

    // 3. 显示数据
    printf("\n=== Measurement Results ===\n");
    for (i = 0; i < NUM_CHANNELS; i++) {
        printf("Channel %d: %.3f V ", i, amplitudes[i]);

        // 检查警告
        if (amplitudes[i] < THRESHOLD_LOW) {
            printf("[WARNING: Too Low]");
            warning_count++;
        } else if (amplitudes[i] > THRESHOLD_HIGH) {
            printf("[WARNING: Too High]");
            warning_count++;
        } else {
            printf("[OK]");
        }
        printf("\n");
    }

    // 4. 显示统计信息
    printf("\n=== Statistics ===\n");
    printf("Average: %.3f V\n", average);
    printf("Total warnings: %d\n", warning_count);

    if (warning_count == 0) {
        printf("Status: All channels normal\n");
    } else {
        printf("Status: Check warnings above\n");
    }

    return 0;
}
```

### 本周知识点总结

**1. 程序结构**
```c
#include <stdio.h>  // 包含头文件

int main() {        // 主函数
    // 代码
    return 0;       // 返回值
}
```

**2. 数据类型**
- `int`：整数
- `float`：浮点数
- `double`：双精度浮点数
- `char`：字符

**3. 运算符**
- 算术：`+`, `-`, `*`, `/`, `%`
- 比较：`==`, `!=`, `>`, `<`, `>=`, `<=`
- 逻辑：`&&`, `||`, `!`

**4. 控制结构**
- 条件：`if`, `else if`, `else`
- 循环：`for`, `while`, `do-while`
- 跳转：`break`, `continue`

**5. 数组**
- 声明：`int arr[10];`
- 初始化：`int arr[] = {1, 2, 3};`
- 访问：`arr[0]`, `arr[i]`

---

## 自我检查清单

完成本周学习后，你应该能够：

- [ ] 编写、编译、运行简单的 C 程序
- [ ] 声明和使用变量（int, float, char）
- [ ] 使用 printf 和 scanf 进行输入输出
- [ ] 使用 if-else 进行条件判断
- [ ] 使用 for/while 循环
- [ ] 声明和使用一维数组
- [ ] 理解程序的基本结构

### 测试题

**1. 下面程序的输出是什么？**
```c
int x = 10;
int y = 20;
int z = x + y;
printf("%d\n", z);
```

**2. 编写程序输出 1-100 中所有的偶数**

**3. 编写程序反转一个数组**
- 输入：{1, 2, 3, 4, 5}
- 输出：{5, 4, 3, 2, 1}

---

## 常见错误和解决方法

**1. 忘记分号**
```c
printf("Hello")  // ❌ 错误
printf("Hello"); // ✅ 正确
```

**2. 数组越界**
```c
int arr[5];
arr[5] = 10;  // ❌ 错误：下标应该是 0-4
arr[4] = 10;  // ✅ 正确
```

**3. scanf 忘记 &**
```c
int x;
scanf("%d", x);   // ❌ 错误
scanf("%d", &x);  // ✅ 正确
```

**4. 整数除法**
```c
int result = 5 / 2;        // 结果是 2（不是 2.5）
float result = 5.0 / 2.0;  // 结果是 2.5
```

---

## 下一步

完成本周学习后，继续学习：
- **Week 2**: 指针详解（C 语言最重要的概念）
- 建议：把本周的程序都自己打一遍，确保理解每一行代码

**记住**：
- 不要急于求成
- 多动手写代码
- 遇到问题先自己调试
- 不懂就查资料或提问

加油！💪
