# Week 2 - 指针详解

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 理解指针的概念和使用方法（C 语言最重要也是最难的部分）

**为什么指针很重要？**
- EPICS IOC 大量使用指针
- 理解指针才能理解 Device Support 的代码
- 指针是操作硬件寄存器的基础

---

## 第1天：理解指针的概念（2小时）

### 什么是指针？

**类比1：门牌号**

```
你的家       →  实际的房子（变量）
门牌号       →  地址
告诉别人门牌号 →  传递指针
```

**类比2：快递**

```
包裹内容     →  变量的值
收货地址     →  变量的地址
快递单号     →  指针
```

### 内存地址

计算机的内存就像一排编号的盒子：

```
地址:    0x1000   0x1004   0x1008   0x100C
内容:    [  10  ] [  20  ] [  30  ] [  40  ]
变量:       a         b         c         d
```

每个变量都有：
1. **值**（内容）：10, 20, 30, 40
2. **地址**（位置）：0x1000, 0x1004, ...

### 第一个指针程序

```c
#include <stdio.h>

int main() {
    int age = 25;        // 普通变量
    int *p;              // 指针变量（指向 int 的指针）

    p = &age;            // p 存储 age 的地址

    printf("age = %d\n", age);           // 输出值：25
    printf("&age = %p\n", &age);         // 输出地址：0x7fff...
    printf("p = %p\n", p);               // 输出指针的值（即 age 的地址）
    printf("*p = %d\n", *p);             // 输出指针指向的值：25

    return 0;
}
```

**输出示例**：
```
age = 25
&age = 0x7ffeefbff5ac
p = 0x7ffeefbff5ac
*p = 25
```

### 关键符号

| 符号 | 名称 | 作用 | 示例 |
|------|------|------|------|
| `&` | 取地址符 | 获取变量的地址 | `&age` |
| `*` | 解引用符（声明时） | 声明指针变量 | `int *p;` |
| `*` | 解引用符（使用时） | 访问指针指向的值 | `*p` |

### 图解理解

```
int age = 25;
int *p = &age;

内存图：
                 age
地址 0x1000:   [  25  ]
                  ↑
                  |
                  | (p 指向这里)
                  |
地址 0x2000:   [0x1000]
                  p

p      →  存储 age 的地址 (0x1000)
*p     →  访问地址 0x1000 的内容 (25)
&age   →  age 的地址 (0x1000)
```

### 实践：指针基础

```c
#include <stdio.h>

int main() {
    int x = 10;
    int *ptr;

    ptr = &x;

    printf("=== 变量 x ===\n");
    printf("值: %d\n", x);
    printf("地址: %p\n", &x);

    printf("\n=== 指针 ptr ===\n");
    printf("指针的值（x的地址）: %p\n", ptr);
    printf("指针指向的值: %d\n", *ptr);
    printf("指针自己的地址: %p\n", &ptr);

    return 0;
}
```

### 练习2.1

完成以下程序，理解指针：

```c
#include <stdio.h>

int main() {
    float voltage = 3.14;
    float *p_voltage;

    p_voltage = &voltage;

    printf("voltage = %.2f\n", voltage);
    printf("&voltage = %p\n", &voltage);
    printf("p_voltage = %p\n", p_voltage);
    printf("*p_voltage = %.2f\n", *p_voltage);

    // 通过指针修改值
    *p_voltage = 5.0;
    printf("\nAfter *p_voltage = 5.0:\n");
    printf("voltage = %.2f\n", voltage);

    return 0;
}
```

---

## 第2天：通过指针修改值（2小时）

### 指针的威力：间接修改

```c
#include <stdio.h>

int main() {
    int count = 10;
    int *p = &count;

    printf("Before: count = %d\n", count);

    *p = 20;  // 通过指针修改 count 的值

    printf("After: count = %d\n", count);

    return 0;
}
```

**输出**：
```
Before: count = 10
After: count = 20
```

### 为什么要用指针？

**场景1：函数需要修改变量**

```c
#include <stdio.h>

// ❌ 错误的方式：不能修改原变量
void increment_wrong(int x) {
    x = x + 1;  // 只修改了副本
}

// ✅ 正确的方式：使用指针
void increment_correct(int *p) {
    *p = *p + 1;  // 修改了原变量
}

int main() {
    int count = 10;

    increment_wrong(count);
    printf("After increment_wrong: %d\n", count);  // 还是 10

    increment_correct(&count);
    printf("After increment_correct: %d\n", count);  // 变成 11

    return 0;
}
```

### 函数参数：值传递 vs 指针传递

```c
#include <stdio.h>

// 值传递：不能修改原变量
void swap_wrong(int a, int b) {
    int temp = a;
    a = b;
    b = temp;
}

// 指针传递：可以修改原变量
void swap_correct(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int x = 10, y = 20;

    printf("Before: x=%d, y=%d\n", x, y);

    swap_wrong(x, y);
    printf("After swap_wrong: x=%d, y=%d\n", x, y);

    swap_correct(&x, &y);
    printf("After swap_correct: x=%d, y=%d\n", x, y);

    return 0;
}
```

**输出**：
```
Before: x=10, y=20
After swap_wrong: x=10, y=20
After swap_correct: x=20, y=10
```

### 实践：BPM 数据处理

```c
#include <stdio.h>

// 通过指针修改 BPM 数据
void process_amplitude(float *amp) {
    // 检查范围
    if (*amp < 0.0) {
        *amp = 0.0;  // 负值置为 0
    }
    if (*amp > 1.0) {
        *amp = 1.0;  // 超过 1V 限制为 1V
    }
}

int main() {
    float ch0_amp = -0.5;
    float ch1_amp = 0.5;
    float ch2_amp = 1.5;

    printf("Before processing:\n");
    printf("CH0: %.2f V\n", ch0_amp);
    printf("CH1: %.2f V\n", ch1_amp);
    printf("CH2: %.2f V\n", ch2_amp);

    process_amplitude(&ch0_amp);
    process_amplitude(&ch1_amp);
    process_amplitude(&ch2_amp);

    printf("\nAfter processing:\n");
    printf("CH0: %.2f V\n", ch0_amp);
    printf("CH1: %.2f V\n", ch1_amp);
    printf("CH2: %.2f V\n", ch2_amp);

    return 0;
}
```

### 练习2.2

编写函数计算圆的面积和周长：

```c
#include <stdio.h>

#define PI 3.14159

void calculate_circle(float radius, float *area, float *circumference) {
    *area = PI * radius * radius;
    *circumference = 2 * PI * radius;
}

int main() {
    float r, a, c;

    printf("Enter radius: ");
    scanf("%f", &r);

    calculate_circle(r, &a, &c);

    printf("Area: %.2f\n", a);
    printf("Circumference: %.2f\n", c);

    return 0;
}
```

---

## 第3天：指针与数组（2小时）

### 数组名就是指针！

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    printf("arr = %p\n", arr);        // 数组名 = 首元素地址
    printf("&arr[0] = %p\n", &arr[0]);  // 首元素地址

    // arr 和 &arr[0] 是相同的！

    return 0;
}
```

### 用指针访问数组

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int *p = arr;  // p 指向数组首元素

    // 三种访问方式
    printf("arr[0] = %d\n", arr[0]);    // 方式1：数组下标
    printf("*arr = %d\n", *arr);        // 方式2：解引用数组名
    printf("*p = %d\n", *p);            // 方式3：解引用指针

    // 访问第二个元素
    printf("\narr[1] = %d\n", arr[1]);
    printf("*(arr+1) = %d\n", *(arr+1));
    printf("*(p+1) = %d\n", *(p+1));

    return 0;
}
```

### 指针运算

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int *p = arr;

    printf("p points to: %d\n", *p);      // 10

    p++;  // 指针向后移动一个元素
    printf("After p++: %d\n", *p);        // 20

    p += 2;  // 向后移动两个元素
    printf("After p+=2: %d\n", *p);       // 40

    p--;  // 向前移动一个元素
    printf("After p--: %d\n", *p);        // 30

    return 0;
}
```

### 图解：指针与数组

```
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;

内存布局：
地址:     0x1000  0x1004  0x1008  0x100C  0x1010
内容:     [ 10 ]  [ 20 ]  [ 30 ]  [ 40 ]  [ 50 ]
索引:      arr[0]  arr[1]  arr[2]  arr[3]  arr[4]
指针:      *p      *(p+1)  *(p+2)  *(p+3)  *(p+4)
          ↑
          p

p + 1  →  地址向后移动 4 字节（一个 int）
*(p+1) →  访问下一个元素的值
```

### 用指针遍历数组

```c
#include <stdio.h>

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int *p;

    // 方法1：使用数组下标
    printf("Method 1:\n");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }

    // 方法2：使用指针
    printf("\n\nMethod 2:\n");
    for (p = arr; p < arr + 5; p++) {
        printf("%d ", *p);
    }

    printf("\n");
    return 0;
}
```

### 实践：BPM 4通道数据处理

```c
#include <stdio.h>

void print_amplitudes(float *amps, int size) {
    float *p;
    int index = 0;

    for (p = amps; p < amps + size; p++) {
        printf("Channel %d: %.3f V\n", index++, *p);
    }
}

float calculate_average(float *amps, int size) {
    float sum = 0.0;
    float *p;

    for (p = amps; p < amps + size; p++) {
        sum += *p;
    }

    return sum / size;
}

int main() {
    float channels[4] = {0.123, 0.456, 0.789, 0.234};

    printf("BPM Channel Amplitudes:\n");
    print_amplitudes(channels, 4);

    printf("\nAverage: %.3f V\n", calculate_average(channels, 4));

    return 0;
}
```

### 练习2.3

用指针实现数组反转：

```c
#include <stdio.h>

void reverse_array(int *arr, int size) {
    int *start = arr;
    int *end = arr + size - 1;
    int temp;

    while (start < end) {
        // 交换
        temp = *start;
        *start = *end;
        *end = temp;

        start++;
        end--;
    }
}

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int i;

    printf("Before: ");
    for (i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }

    reverse_array(arr, 5);

    printf("\nAfter:  ");
    for (i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}
```

---

## 第4天：指针与字符串（2小时）

### 字符串的本质

在 C 语言中，字符串就是字符数组，以 `'\0'` 结尾：

```c
char str[] = "Hello";

// 等价于：
char str[] = {'H', 'e', 'l', 'l', 'o', '\0'};

内存布局：
['H']['e']['l']['l']['o']['\0']
  0    1    2    3    4    5
```

### 字符串指针

```c
#include <stdio.h>

int main() {
    char str[] = "Hello";
    char *p = str;

    // 输出整个字符串
    printf("%s\n", str);  // Hello
    printf("%s\n", p);    // Hello

    // 输出第一个字符
    printf("%c\n", str[0]);  // H
    printf("%c\n", *p);      // H

    // 输出第二个字符
    printf("%c\n", str[1]);  // e
    printf("%c\n", *(p+1));  // e

    return 0;
}
```

### 遍历字符串

```c
#include <stdio.h>

int main() {
    char str[] = "BPM";
    char *p;

    // 方法1：数组方式
    printf("Method 1: ");
    for (int i = 0; str[i] != '\0'; i++) {
        printf("%c ", str[i]);
    }

    // 方法2：指针方式
    printf("\nMethod 2: ");
    for (p = str; *p != '\0'; p++) {
        printf("%c ", *p);
    }

    printf("\n");
    return 0;
}
```

### 字符串函数

```c
#include <stdio.h>
#include <string.h>  // 字符串函数库

int main() {
    char str1[20] = "Hello";
    char str2[] = "World";

    // strlen: 字符串长度
    printf("Length: %lu\n", strlen(str1));

    // strcpy: 字符串复制
    strcpy(str1, str2);
    printf("After strcpy: %s\n", str1);  // World

    // strcat: 字符串连接
    strcpy(str1, "Hello");
    strcat(str1, " World");
    printf("After strcat: %s\n", str1);  // Hello World

    // strcmp: 字符串比较
    if (strcmp(str1, "Hello World") == 0) {
        printf("Strings are equal\n");
    }

    return 0;
}
```

### 实践：解析 EPICS INP 字段

模拟解析类似 `"@AMP:0 ch=2"` 的字符串：

```c
#include <stdio.h>
#include <string.h>

void parse_inp_field(char *inp, char *type, int *offset, int *channel) {
    char *p;

    // 跳过 '@'
    p = inp + 1;

    // 提取类型（到 ':' 为止）
    int i = 0;
    while (*p != ':' && *p != '\0') {
        type[i++] = *p++;
    }
    type[i] = '\0';

    // 跳过 ':'
    p++;

    // 提取 offset
    *offset = 0;
    while (*p >= '0' && *p <= '9') {
        *offset = *offset * 10 + (*p - '0');
        p++;
    }

    // 跳过空格
    while (*p == ' ') p++;

    // 提取 channel
    if (strncmp(p, "ch=", 3) == 0) {
        p += 3;
        *channel = *p - '0';
    }
}

int main() {
    char inp[] = "@AMP:0 ch=2";
    char type[10];
    int offset, channel;

    parse_inp_field(inp, type, &offset, &channel);

    printf("Type: %s\n", type);
    printf("Offset: %d\n", offset);
    printf("Channel: %d\n", channel);

    return 0;
}
```

### 练习2.4

实现字符串长度函数（不用 strlen）：

```c
#include <stdio.h>

int my_strlen(char *str) {
    int count = 0;
    while (*str != '\0') {
        count++;
        str++;
    }
    return count;
}

int main() {
    char str[] = "Hello World";
    printf("Length: %d\n", my_strlen(str));
    return 0;
}
```

---

## 第5天：多级指针和指针数组（2小时）

### 指向指针的指针

```c
#include <stdio.h>

int main() {
    int x = 10;
    int *p = &x;      // p 指向 x
    int **pp = &p;    // pp 指向 p（指向指针的指针）

    printf("x = %d\n", x);
    printf("*p = %d\n", *p);
    printf("**pp = %d\n", **pp);

    // 通过 pp 修改 x
    **pp = 20;
    printf("After **pp = 20:\n");
    printf("x = %d\n", x);

    return 0;
}
```

**图解**：
```
        x          p          pp
     [ 10 ]  ← [&x]    ← [&p]
    0x1000     0x2000    0x3000

*p    = x 的值 = 10
**pp  = *(*pp) = *(p) = x = 10
```

### 指针数组

```c
#include <stdio.h>

int main() {
    char *names[] = {
        "Channel_0",
        "Channel_1",
        "Channel_2",
        "Channel_3"
    };

    for (int i = 0; i < 4; i++) {
        printf("%s\n", names[i]);
    }

    return 0;
}
```

**内存布局**：
```
names[0] → "Channel_0"
names[1] → "Channel_1"
names[2] → "Channel_2"
names[3] → "Channel_3"
```

### 实践：BPM PV 名称管理

```c
#include <stdio.h>

int main() {
    char *pv_names[] = {
        "BPM:CH0:Amp",
        "BPM:CH1:Amp",
        "BPM:CH2:Amp",
        "BPM:CH3:Amp"
    };

    float amplitudes[] = {0.123, 0.456, 0.789, 0.234};

    printf("=== BPM Data ===\n");
    for (int i = 0; i < 4; i++) {
        printf("%-15s: %.3f V\n", pv_names[i], amplitudes[i]);
    }

    return 0;
}
```

### 函数指针（预览）

```c
#include <stdio.h>

// 两个简单函数
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    // 函数指针
    int (*operation)(int, int);

    operation = add;
    printf("10 + 5 = %d\n", operation(10, 5));

    operation = subtract;
    printf("10 - 5 = %d\n", operation(10, 5));

    return 0;
}
```

**EPICS 中的应用**：
```c
// Device Support 结构体（简化版）
struct {
    long (*read)(void *record);   // 函数指针
    long (*write)(void *record);
} device_support;
```

### 练习2.5

创建一个简单的命令解析器：

```c
#include <stdio.h>
#include <string.h>

void cmd_help() {
    printf("Available commands: help, status, quit\n");
}

void cmd_status() {
    printf("System status: OK\n");
}

void cmd_quit() {
    printf("Quitting...\n");
}

int main() {
    char *commands[] = {"help", "status", "quit"};
    void (*functions[])() = {cmd_help, cmd_status, cmd_quit};
    char input[20];

    while (1) {
        printf("> ");
        scanf("%s", input);

        int found = 0;
        for (int i = 0; i < 3; i++) {
            if (strcmp(input, commands[i]) == 0) {
                functions[i]();
                found = 1;
                if (i == 2) return 0;  // quit
                break;
            }
        }

        if (!found) {
            printf("Unknown command\n");
        }
    }

    return 0;
}
```

---

## 第6天：动态内存分配（2小时）

### 为什么需要动态内存？

**静态分配**（编译时确定大小）：
```c
int arr[100];  // 固定大小
```

**动态分配**（运行时确定大小）：
```c
int size;
printf("Enter size: ");
scanf("%d", &size);

int *arr = malloc(size * sizeof(int));  // 灵活大小
```

### malloc 和 free

```c
#include <stdio.h>
#include <stdlib.h>  // malloc, free

int main() {
    int *arr;
    int size;

    printf("Enter array size: ");
    scanf("%d", &size);

    // 分配内存
    arr = (int *)malloc(size * sizeof(int));

    if (arr == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 使用内存
    for (int i = 0; i < size; i++) {
        arr[i] = i * 10;
    }

    // 打印
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    // 释放内存（重要！）
    free(arr);

    return 0;
}
```

### calloc 和 realloc

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int *arr;

    // calloc: 分配并初始化为 0
    arr = (int *)calloc(5, sizeof(int));
    printf("After calloc: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);  // 全是 0
    }

    // realloc: 重新分配大小
    arr = (int *)realloc(arr, 10 * sizeof(int));
    printf("\nAfter realloc to 10 elements\n");

    free(arr);
    return 0;
}
```

### 动态内存常见错误

**1. 内存泄漏**
```c
void bad_function() {
    int *p = malloc(100 * sizeof(int));
    // 忘记 free(p)
}  // 内存泄漏！
```

**2. 重复释放**
```c
int *p = malloc(100 * sizeof(int));
free(p);
free(p);  // ❌ 错误：重复释放
```

**3. 使用已释放的内存**
```c
int *p = malloc(100 * sizeof(int));
free(p);
*p = 10;  // ❌ 错误：使用已释放的内存
```

### 实践：动态 BPM 数据缓冲区

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int channel;
    float amplitude;
    float phase;
} BPMData;

int main() {
    int num_samples;
    BPMData *buffer;

    printf("Enter number of samples: ");
    scanf("%d", &num_samples);

    // 分配内存
    buffer = (BPMData *)malloc(num_samples * sizeof(BPMData));

    if (buffer == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // 填充数据
    for (int i = 0; i < num_samples; i++) {
        buffer[i].channel = i % 4;
        buffer[i].amplitude = 0.1 + i * 0.01;
        buffer[i].phase = i * 10.0;
    }

    // 显示数据
    printf("\nBPM Data Buffer:\n");
    for (int i = 0; i < num_samples; i++) {
        printf("Sample %d - CH%d: Amp=%.3f, Phase=%.1f\n",
               i, buffer[i].channel, buffer[i].amplitude, buffer[i].phase);
    }

    // 释放内存
    free(buffer);

    return 0;
}
```

### 练习2.6

实现动态字符串复制：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* my_strdup(char *src) {
    int len = strlen(src);
    char *dest = (char *)malloc((len + 1) * sizeof(char));

    if (dest == NULL) {
        return NULL;
    }

    strcpy(dest, src);
    return dest;
}

int main() {
    char *original = "Hello, EPICS!";
    char *copy = my_strdup(original);

    printf("Original: %s\n", original);
    printf("Copy: %s\n", copy);

    free(copy);
    return 0;
}
```

---

## 第7天：综合练习和复习（2小时）

### 综合项目：简化版 EPICS Record 读取

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 模拟硬件寄存器读取
float read_fpga_register(int address) {
    // 模拟：返回一个假值
    return 0.123 + address * 0.1;
}

// 模拟 Record 结构
typedef struct {
    char name[50];
    float value;
    int hardware_address;
} Record;

// 读取函数（Device Support）
void read_record(Record *rec) {
    // 从 FPGA 读取原始值
    float raw_value = read_fpga_register(rec->hardware_address);

    // 存储到 record
    rec->value = raw_value;

    printf("Read %s: %.3f (from address 0x%04X)\n",
           rec->name, rec->value, rec->hardware_address);
}

// 写入函数（Device Support）
void write_record(Record *rec, float value) {
    rec->value = value;
    // 这里应该写入 FPGA，现在只是打印
    printf("Write %s: %.3f (to address 0x%04X)\n",
           rec->name, rec->value, rec->hardware_address);
}

int main() {
    // 创建 4 个 record
    Record records[4];

    // 初始化
    for (int i = 0; i < 4; i++) {
        sprintf(records[i].name, "BPM:CH%d:Amp", i);
        records[i].hardware_address = 0x1000 + i * 4;
        records[i].value = 0.0;
    }

    // 读取所有 record
    printf("=== Reading Records ===\n");
    for (int i = 0; i < 4; i++) {
        read_record(&records[i]);
    }

    // 写入一个 record
    printf("\n=== Writing Record ===\n");
    write_record(&records[0], 0.999);

    return 0;
}
```

### 本周知识点总结

**1. 指针基础**
```c
int x = 10;
int *p = &x;    // p 存储 x 的地址
*p = 20;        // 通过指针修改 x
```

**2. 指针与函数**
```c
void modify(int *p) {
    *p = 100;   // 可以修改原变量
}
```

**3. 指针与数组**
```c
int arr[5];
int *p = arr;   // 数组名就是指针
*(p+2) == arr[2]  // 指针运算
```

**4. 指针与字符串**
```c
char str[] = "Hello";
char *p = str;
while (*p != '\0') p++;  // 遍历字符串
```

**5. 动态内存**
```c
int *p = malloc(n * sizeof(int));
free(p);  // 必须释放
```

---

## 自我检查清单

完成本周学习后，你应该能够：

- [ ] 理解指针的概念（地址、解引用）
- [ ] 使用 & 和 * 操作符
- [ ] 通过指针修改变量的值
- [ ] 用指针作为函数参数
- [ ] 用指针访问数组元素
- [ ] 理解指针运算
- [ ] 使用 malloc/free 分配和释放内存
- [ ] 理解 EPICS 代码中的指针用法

### 测试题

**1. 下面程序输出什么？**
```c
int x = 10;
int *p = &x;
*p = 20;
printf("%d\n", x);
```

**2. 找出错误**
```c
int *p;
*p = 100;  // 错在哪里？
```

**3. 完成函数**
```c
// 找出数组中的最大值
int find_max(int *arr, int size) {
    // 你的代码
}
```

---

## 常见错误和解决方法

**1. 野指针**
```c
int *p;        // ❌ 未初始化
*p = 10;       // 危险！

int x;
int *p = &x;   // ✅ 正确
*p = 10;
```

**2. 指针越界**
```c
int arr[5];
int *p = arr;
p += 10;      // ❌ 越界
*p = 100;     // 危险！
```

**3. 忘记 free**
```c
void func() {
    int *p = malloc(100 * sizeof(int));
    // ... 使用 p ...
    free(p);  // ✅ 不要忘记！
}
```

---

## 与 EPICS 的联系

在 EPICS IOC 代码中，你会看到大量指针：

```c
// devBPMMonitor.c 中的例子
static long read_ai(aiRecord *record)  // 指针参数
{
    recordpara_t *priv = (recordpara_t *)record->dpvt;  // 类型转换
    float value = ReadData(priv->offset, ...);  // 通过指针访问成员
    record->val = value;  // 通过指针修改 record
    return 2;
}
```

现在你应该能理解这些代码了！

---

## 下一步

完成本周学习后，继续学习：
- **Week 3**: 结构体和函数（组织更复杂的数据）
- 建议：多练习指针，这是 C 语言最重要的部分

**记住**：
- 指针一开始很难，但非常重要
- 多画内存图帮助理解
- 所有的 EPICS 代码都在用指针
- 慢慢来，多练习

加油！💪
