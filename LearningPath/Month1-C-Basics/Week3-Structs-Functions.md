# Week 3 - 结构体和函数

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 掌握结构体和函数的使用，为理解 EPICS 架构打好基础

**为什么重要？**
- EPICS 中所有的 Record 都是结构体
- Device Support 通过函数表组织代码
- 理解结构体是读懂 EPICS 源码的关键

---

## 第1天：函数基础（2小时）

### 为什么要用函数？

**没有函数的代码**（重复、混乱）：
```c
int main() {
    // 计算圆面积1
    float r1 = 5.0;
    float area1 = 3.14159 * r1 * r1;

    // 计算圆面积2
    float r2 = 10.0;
    float area2 = 3.14159 * r2 * r2;

    // 计算圆面积3
    float r3 = 15.0;
    float area3 = 3.14159 * r3 * r3;
}
```

**使用函数**（简洁、清晰）：
```c
float calculate_area(float radius) {
    return 3.14159 * radius * radius;
}

int main() {
    float area1 = calculate_area(5.0);
    float area2 = calculate_area(10.0);
    float area3 = calculate_area(15.0);
}
```

### 函数的定义和调用

```c
#include <stdio.h>

// 函数定义
返回类型 函数名(参数列表) {
    // 函数体
    return 返回值;
}

// 示例：计算两数之和
int add(int a, int b) {
    int sum = a + b;
    return sum;
}

int main() {
    int result = add(10, 20);  // 函数调用
    printf("Result: %d\n", result);
    return 0;
}
```

### 函数的组成部分

```c
int add(int a, int b)
│   │   └────┬────┘
│   │        └─ 参数列表（输入）
│   └─ 函数名
└─ 返回类型（输出）
```

### 无返回值的函数

```c
#include <stdio.h>

void print_header() {  // void 表示无返回值
    printf("=== BPM Monitor ===\n");
    printf("==================\n");
}

int main() {
    print_header();
    printf("Channel 0: 0.123 V\n");
    return 0;
}
```

### 有参数无返回值

```c
#include <stdio.h>

void print_amplitude(float amp, int channel) {
    printf("Channel %d: %.3f V\n", channel, amp);
}

int main() {
    print_amplitude(0.123, 0);
    print_amplitude(0.456, 1);
    print_amplitude(0.789, 2);
    return 0;
}
```

### 无参数有返回值

```c
#include <stdio.h>

float get_pi() {
    return 3.14159;
}

int main() {
    float pi = get_pi();
    printf("PI = %.5f\n", pi);
    return 0;
}
```

### 实践：BPM 数据验证

```c
#include <stdio.h>

// 检查幅度是否在有效范围
int is_valid_amplitude(float amp) {
    if (amp >= 0.0 && amp <= 1.0) {
        return 1;  // 有效
    } else {
        return 0;  // 无效
    }
}

// 打印状态
void print_status(int channel, float amp, int valid) {
    printf("Channel %d: %.3f V - ", channel, amp);
    if (valid) {
        printf("[OK]\n");
    } else {
        printf("[ERROR]\n");
    }
}

int main() {
    float amps[] = {0.5, 1.5, -0.1, 0.8};

    for (int i = 0; i < 4; i++) {
        int valid = is_valid_amplitude(amps[i]);
        print_status(i, amps[i], valid);
    }

    return 0;
}
```

### 练习3.1

编写函数计算数组的平均值：

```c
#include <stdio.h>

float calculate_average(float *arr, int size) {
    float sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum / size;
}

int main() {
    float data[] = {0.1, 0.2, 0.3, 0.4, 0.5};
    float avg = calculate_average(data, 5);
    printf("Average: %.2f\n", avg);
    return 0;
}
```

---

## 第2天：函数进阶（2小时）

### 函数声明（原型）

```c
#include <stdio.h>

// 函数声明（在 main 之前）
float calculate_area(float radius);
float calculate_circumference(float radius);

int main() {
    float r = 5.0;
    printf("Area: %.2f\n", calculate_area(r));
    printf("Circumference: %.2f\n", calculate_circumference(r));
    return 0;
}

// 函数定义（在 main 之后）
float calculate_area(float radius) {
    return 3.14159 * radius * radius;
}

float calculate_circumference(float radius) {
    return 2 * 3.14159 * radius;
}
```

### 递归函数

```c
#include <stdio.h>

// 递归计算阶乘
int factorial(int n) {
    if (n == 0 || n == 1) {
        return 1;  // 基本情况
    }
    return n * factorial(n - 1);  // 递归调用
}

int main() {
    printf("5! = %d\n", factorial(5));  // 120
    return 0;
}
```

**递归过程**：
```
factorial(5)
= 5 * factorial(4)
= 5 * (4 * factorial(3))
= 5 * (4 * (3 * factorial(2)))
= 5 * (4 * (3 * (2 * factorial(1))))
= 5 * (4 * (3 * (2 * 1)))
= 120
```

### 静态变量

```c
#include <stdio.h>

void count_calls() {
    static int counter = 0;  // 静态变量，只初始化一次
    counter++;
    printf("Function called %d times\n", counter);
}

int main() {
    count_calls();  // Function called 1 times
    count_calls();  // Function called 2 times
    count_calls();  // Function called 3 times
    return 0;
}
```

### 实践：FPGA 寄存器读取模拟

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 模拟 FPGA 寄存器值（每次读取略有变化）
float read_register(int address) {
    static int initialized = 0;

    if (!initialized) {
        srand(time(NULL));
        initialized = 1;
    }

    // 基础值 + 随机噪声
    float base_value = 0.5;
    float noise = (rand() % 100 - 50) / 10000.0;  // ±0.005
    return base_value + noise;
}

// 多次读取求平均（降噪）
float read_register_averaged(int address, int num_reads) {
    float sum = 0.0;
    for (int i = 0; i < num_reads; i++) {
        sum += read_register(address);
    }
    return sum / num_reads;
}

int main() {
    printf("Single read: %.5f\n", read_register(0x1000));
    printf("Averaged (10 reads): %.5f\n", read_register_averaged(0x1000, 10));
    printf("Averaged (100 reads): %.5f\n", read_register_averaged(0x1000, 100));
    return 0;
}
```

### 练习3.2

实现斐波那契数列（递归版和循环版）：

```c
#include <stdio.h>

// 递归版
int fibonacci_recursive(int n) {
    if (n <= 1) return n;
    return fibonacci_recursive(n-1) + fibonacci_recursive(n-2);
}

// 循环版（更高效）
int fibonacci_iterative(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1, c;
    for (int i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int main() {
    printf("Fibonacci (recursive): ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", fibonacci_recursive(i));
    }

    printf("\nFibonacci (iterative): ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", fibonacci_iterative(i));
    }
    printf("\n");

    return 0;
}
```

---

## 第3天：结构体基础（2小时）

### 什么是结构体？

结构体（struct）是**自定义的数据类型**，可以把不同类型的数据组合在一起。

**类比**：
- 学生信息：姓名（字符串）+ 年龄（整数）+ 成绩（浮点数）
- BPM 数据：通道（整数）+ 幅度（浮点数）+ 相位（浮点数）

### 定义结构体

```c
#include <stdio.h>

// 定义结构体类型
struct Point {
    float x;
    float y;
};

int main() {
    // 声明结构体变量
    struct Point p1;

    // 访问成员
    p1.x = 10.5;
    p1.y = 20.3;

    printf("Point: (%.1f, %.1f)\n", p1.x, p1.y);

    return 0;
}
```

### 结构体初始化

```c
#include <stdio.h>

struct Point {
    float x;
    float y;
};

int main() {
    // 方法1：声明后赋值
    struct Point p1;
    p1.x = 10.0;
    p1.y = 20.0;

    // 方法2：声明时初始化
    struct Point p2 = {30.0, 40.0};

    // 方法3：指定成员初始化
    struct Point p3 = {.y = 60.0, .x = 50.0};

    printf("p1: (%.1f, %.1f)\n", p1.x, p1.y);
    printf("p2: (%.1f, %.1f)\n", p2.x, p2.y);
    printf("p3: (%.1f, %.1f)\n", p3.x, p3.y);

    return 0;
}
```

### typedef 简化定义

```c
#include <stdio.h>

// 方法1：传统方式
struct Point {
    float x;
    float y;
};

// 使用时需要写 struct
struct Point p1;

// 方法2：使用 typedef
typedef struct {
    float x;
    float y;
} Point;

// 使用时不需要 struct
Point p2;
```

### 实践：BPM 数据结构

```c
#include <stdio.h>

typedef struct {
    int channel;
    float amplitude;
    float phase;
} BPMData;

void print_bpm_data(BPMData data) {
    printf("Channel %d: Amp=%.3fV, Phase=%.1f°\n",
           data.channel, data.amplitude, data.phase);
}

int main() {
    BPMData ch0 = {0, 0.123, 45.5};
    BPMData ch1 = {1, 0.456, 90.2};

    print_bpm_data(ch0);
    print_bpm_data(ch1);

    return 0;
}
```

### 结构体数组

```c
#include <stdio.h>

typedef struct {
    int channel;
    float amplitude;
} BPMChannel;

int main() {
    BPMChannel channels[4] = {
        {0, 0.123},
        {1, 0.456},
        {2, 0.789},
        {3, 0.234}
    };

    printf("=== BPM Channels ===\n");
    for (int i = 0; i < 4; i++) {
        printf("CH%d: %.3f V\n", channels[i].channel, channels[i].amplitude);
    }

    return 0;
}
```

### 练习3.3

定义学生结构体并计算平均分：

```c
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[50];
    int age;
    float score;
} Student;

float calculate_average_score(Student students[], int count) {
    float sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += students[i].score;
    }
    return sum / count;
}

int main() {
    Student students[3] = {
        {"Alice", 20, 85.5},
        {"Bob", 21, 90.0},
        {"Charlie", 19, 78.5}
    };

    printf("=== Student List ===\n");
    for (int i = 0; i < 3; i++) {
        printf("%s (Age %d): %.1f\n",
               students[i].name, students[i].age, students[i].score);
    }

    printf("\nAverage Score: %.1f\n", calculate_average_score(students, 3));

    return 0;
}
```

---

## 第4天：结构体与指针（2小时）

### 指向结构体的指针

```c
#include <stdio.h>

typedef struct {
    float x;
    float y;
} Point;

int main() {
    Point p1 = {10.0, 20.0};
    Point *ptr = &p1;

    // 方法1：使用点运算符
    printf("x = %.1f, y = %.1f\n", (*ptr).x, (*ptr).y);

    // 方法2：使用箭头运算符（常用）
    printf("x = %.1f, y = %.1f\n", ptr->x, ptr->y);

    return 0;
}
```

**关键符号**：
- `(*ptr).x`：先解引用，再访问成员
- `ptr->x`：直接通过指针访问成员（等价于上面）

### 通过指针修改结构体

```c
#include <stdio.h>

typedef struct {
    int channel;
    float amplitude;
} BPMData;

void update_amplitude(BPMData *data, float new_amp) {
    data->amplitude = new_amp;
}

int main() {
    BPMData ch0 = {0, 0.123};

    printf("Before: CH%d = %.3f V\n", ch0.channel, ch0.amplitude);

    update_amplitude(&ch0, 0.999);

    printf("After:  CH%d = %.3f V\n", ch0.channel, ch0.amplitude);

    return 0;
}
```

### 为什么用指针传递结构体？

```c
// ❌ 值传递：复制整个结构体（慢、浪费内存）
void process_data(BPMData data) {
    // 处理数据
}

// ✅ 指针传递：只传递地址（快、省内存）
void process_data(BPMData *data) {
    // 处理数据
}
```

### 实践：EPICS Record 模拟

```c
#include <stdio.h>

// 模拟 aiRecord（模拟输入记录）
typedef struct {
    char name[50];
    float val;      // 当前值
    int offset;     // 硬件地址偏移
    int channel;    // 通道号
} aiRecord;

// 模拟硬件读取
float read_hardware(int offset, int channel) {
    return 0.123 + channel * 0.1;  // 假数据
}

// Device Support 读取函数
void read_ai(aiRecord *record) {
    // 从硬件读取
    float raw_value = read_hardware(record->offset, record->channel);

    // 存储到 record
    record->val = raw_value;

    printf("Read %s: %.3f\n", record->name, record->val);
}

int main() {
    // 创建 record
    aiRecord rec = {
        .name = "BPM:CH0:Amp",
        .val = 0.0,
        .offset = 0x1000,
        .channel = 0
    };

    // 读取
    read_ai(&rec);

    return 0;
}
```

### 结构体嵌套

```c
#include <stdio.h>

typedef struct {
    float amplitude;
    float phase;
} ChannelData;

typedef struct {
    int bpm_id;
    ChannelData channels[4];
} BPMMonitor;

int main() {
    BPMMonitor bpm1 = {
        .bpm_id = 1,
        .channels = {
            {0.123, 45.0},
            {0.456, 90.0},
            {0.789, 135.0},
            {0.234, 180.0}
        }
    };

    printf("BPM ID: %d\n", bpm1.bpm_id);
    for (int i = 0; i < 4; i++) {
        printf("CH%d: Amp=%.3f, Phase=%.1f\n",
               i, bpm1.channels[i].amplitude, bpm1.channels[i].phase);
    }

    return 0;
}
```

### 练习3.4

实现结构体的深拷贝：

```c
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[50];
    int channel;
    float value;
} PVData;

void copy_pv_data(PVData *dest, PVData *src) {
    strcpy(dest->name, src->name);
    dest->channel = src->channel;
    dest->value = src->value;
}

int main() {
    PVData pv1 = {"BPM:CH0:Amp", 0, 0.123};
    PVData pv2;

    copy_pv_data(&pv2, &pv1);

    printf("PV1: %s, CH%d, %.3f\n", pv1.name, pv1.channel, pv1.value);
    printf("PV2: %s, CH%d, %.3f\n", pv2.name, pv2.channel, pv2.value);

    return 0;
}
```

---

## 第5天：函数指针和回调（2小时）

### 函数指针基础

```c
#include <stdio.h>

// 两个简单函数
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    // 声明函数指针
    int (*operation)(int, int);

    // 指向 add 函数
    operation = add;
    printf("10 + 5 = %d\n", operation(10, 5));

    // 指向 multiply 函数
    operation = multiply;
    printf("10 * 5 = %d\n", operation(10, 5));

    return 0;
}
```

**语法**：
```c
返回类型 (*指针名)(参数类型列表);

int (*func_ptr)(int, int);  // 指向返回 int、接受两个 int 的函数
```

### 回调函数

```c
#include <stdio.h>

// 回调函数类型
typedef void (*Callback)(int channel, float value);

// 数据处理函数，接受回调
void process_channels(float data[], int size, Callback callback) {
    for (int i = 0; i < size; i++) {
        callback(i, data[i]);  // 调用回调函数
    }
}

// 回调函数1：打印数据
void print_data(int channel, float value) {
    printf("CH%d: %.3f V\n", channel, value);
}

// 回调函数2：检查范围
void check_range(int channel, float value) {
    if (value < 0.1 || value > 1.0) {
        printf("CH%d: WARNING - out of range\n", channel);
    }
}

int main() {
    float amplitudes[] = {0.123, 0.456, 1.5, 0.05};

    printf("=== Print Data ===\n");
    process_channels(amplitudes, 4, print_data);

    printf("\n=== Check Range ===\n");
    process_channels(amplitudes, 4, check_range);

    return 0;
}
```

### EPICS Device Support 结构（简化）

```c
#include <stdio.h>

// 模拟 Record
typedef struct {
    char name[50];
    float val;
    void *dpvt;  // 设备私有数据
} Record;

// Device Support 函数指针表
typedef struct {
    long (*init_record)(Record *rec);
    long (*read)(Record *rec);
} DeviceSupport;

// 实际的设备支持函数
long my_init_record(Record *rec) {
    printf("Initializing record: %s\n", rec->name);
    return 0;
}

long my_read(Record *rec) {
    rec->val = 0.123;  // 从硬件读取
    printf("Read %s: %.3f\n", rec->name, rec->val);
    return 0;
}

// Device Support 表
DeviceSupport my_device_support = {
    .init_record = my_init_record,
    .read = my_read
};

int main() {
    Record rec = {.name = "BPM:CH0:Amp", .val = 0.0, .dpvt = NULL};

    // 通过函数指针调用
    my_device_support.init_record(&rec);
    my_device_support.read(&rec);

    return 0;
}
```

### 实践：简化的 EPICS 架构

```c
#include <stdio.h>
#include <stdlib.h>

// Record 结构
typedef struct {
    char name[50];
    float val;
    int hardware_addr;
} aiRecord;

// Device Support 函数指针表
typedef struct {
    long (*init)(aiRecord *);
    long (*read)(aiRecord *);
} DSET;

// 设备1：BPM
long bpm_init(aiRecord *rec) {
    printf("[BPM] Init: %s\n", rec->name);
    return 0;
}

long bpm_read(aiRecord *rec) {
    rec->val = 0.123;  // 模拟 BPM 数据
    printf("[BPM] Read: %s = %.3f\n", rec->name, rec->val);
    return 0;
}

DSET bpm_dset = {bpm_init, bpm_read};

// 设备2：温度传感器
long temp_init(aiRecord *rec) {
    printf("[TEMP] Init: %s\n", rec->name);
    return 0;
}

long temp_read(aiRecord *rec) {
    rec->val = 25.5;  // 模拟温度数据
    printf("[TEMP] Read: %s = %.1f\n", rec->name, rec->val);
    return 0;
}

DSET temp_dset = {temp_init, temp_read};

// 处理 record（统一接口）
void process_record(aiRecord *rec, DSET *dset) {
    dset->init(rec);
    dset->read(rec);
}

int main() {
    aiRecord bpm_rec = {.name = "BPM:CH0:Amp", .val = 0.0, .hardware_addr = 0x1000};
    aiRecord temp_rec = {.name = "ROOM:Temp", .val = 0.0, .hardware_addr = 0x2000};

    process_record(&bpm_rec, &bpm_dset);
    printf("\n");
    process_record(&temp_rec, &temp_dset);

    return 0;
}
```

### 练习3.5

实现一个简单的计算器（使用函数指针）：

```c
#include <stdio.h>

typedef float (*Operation)(float, float);

float add(float a, float b) { return a + b; }
float subtract(float a, float b) { return a - b; }
float multiply(float a, float b) { return a * b; }
float divide(float a, float b) { return a / b; }

void calculate(float a, float b, Operation op, char *op_name) {
    float result = op(a, b);
    printf("%.2f %s %.2f = %.2f\n", a, op_name, b, result);
}

int main() {
    float a = 10.0, b = 5.0;

    calculate(a, b, add, "+");
    calculate(a, b, subtract, "-");
    calculate(a, b, multiply, "*");
    calculate(a, b, divide, "/");

    return 0;
}
```

---

## 第6天：文件操作（2小时）

### 打开和关闭文件

```c
#include <stdio.h>

int main() {
    FILE *fp;

    // 打开文件（写模式）
    fp = fopen("test.txt", "w");

    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    // 写入文件
    fprintf(fp, "Hello, EPICS!\n");
    fprintf(fp, "BPM Amplitude: %.3f\n", 0.123);

    // 关闭文件
    fclose(fp);

    printf("File written successfully!\n");

    return 0;
}
```

**文件打开模式**：
- `"r"`：只读（文件必须存在）
- `"w"`：写入（覆盖原文件）
- `"a"`：追加（在文件末尾写入）
- `"r+"`：读写（文件必须存在）
- `"w+"`：读写（覆盖原文件）

### 读取文件

```c
#include <stdio.h>

int main() {
    FILE *fp;
    char line[100];

    fp = fopen("test.txt", "r");

    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    // 逐行读取
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }

    fclose(fp);

    return 0;
}
```

### 实践：保存 BPM 数据到文件

```c
#include <stdio.h>
#include <time.h>

typedef struct {
    int channel;
    float amplitude;
    float phase;
} BPMData;

void save_bpm_data(BPMData data[], int count, char *filename) {
    FILE *fp = fopen(filename, "w");

    if (fp == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    // 写入头部
    fprintf(fp, "=== BPM Data Log ===\n");
    fprintf(fp, "Time: %ld\n\n", time(NULL));
    fprintf(fp, "Channel, Amplitude(V), Phase(deg)\n");

    // 写入数据
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d, %.3f, %.1f\n",
                data[i].channel, data[i].amplitude, data[i].phase);
    }

    fclose(fp);
    printf("Data saved to %s\n", filename);
}

int main() {
    BPMData data[] = {
        {0, 0.123, 45.0},
        {1, 0.456, 90.0},
        {2, 0.789, 135.0},
        {3, 0.234, 180.0}
    };

    save_bpm_data(data, 4, "bpm_data.csv");

    return 0;
}
```

### 二进制文件

```c
#include <stdio.h>

typedef struct {
    int channel;
    float amplitude;
} BPMData;

int main() {
    BPMData data[] = {
        {0, 0.123},
        {1, 0.456},
        {2, 0.789}
    };

    FILE *fp;

    // 写入二进制文件
    fp = fopen("data.bin", "wb");
    fwrite(data, sizeof(BPMData), 3, fp);
    fclose(fp);

    // 读取二进制文件
    BPMData read_data[3];
    fp = fopen("data.bin", "rb");
    fread(read_data, sizeof(BPMData), 3, fp);
    fclose(fp);

    // 显示读取的数据
    for (int i = 0; i < 3; i++) {
        printf("CH%d: %.3f V\n", read_data[i].channel, read_data[i].amplitude);
    }

    return 0;
}
```

### 练习3.6

实现配置文件读取：

```c
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[50];
    float threshold_low;
    float threshold_high;
} Config;

int load_config(char *filename, Config *config) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) return -1;

    fscanf(fp, "name=%s\n", config->name);
    fscanf(fp, "threshold_low=%f\n", &config->threshold_low);
    fscanf(fp, "threshold_high=%f\n", &config->threshold_high);

    fclose(fp);
    return 0;
}

int main() {
    Config config;

    if (load_config("config.txt", &config) == 0) {
        printf("Loaded config:\n");
        printf("  Name: %s\n", config.name);
        printf("  Low: %.2f\n", config.threshold_low);
        printf("  High: %.2f\n", config.threshold_high);
    } else {
        printf("Failed to load config\n");
    }

    return 0;
}
```

---

## 第7天：综合项目（2小时）

### 项目：完整的 BPM 数据采集系统

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 数据结构定义
typedef struct {
    int channel;
    float amplitude;
    float phase;
    time_t timestamp;
} BPMSample;

typedef struct {
    char name[50];
    float threshold_low;
    float threshold_high;
    int num_channels;
} SystemConfig;

// 函数声明
void load_config(SystemConfig *config);
void acquire_data(BPMSample samples[], int count);
void process_data(BPMSample samples[], int count, SystemConfig *config);
void save_data(BPMSample samples[], int count, char *filename);
void print_summary(BPMSample samples[], int count);

// 主程序
int main() {
    SystemConfig config;
    BPMSample samples[4];

    printf("=== BPM Data Acquisition System ===\n\n");

    // 1. 加载配置
    load_config(&config);
    printf("Configuration loaded: %s\n", config.name);
    printf("Thresholds: %.2f - %.2f V\n\n", config.threshold_low, config.threshold_high);

    // 2. 采集数据
    printf("Acquiring data...\n");
    acquire_data(samples, config.num_channels);
    printf("Data acquired.\n\n");

    // 3. 处理数据
    process_data(samples, config.num_channels, &config);

    // 4. 保存数据
    save_data(samples, config.num_channels, "bpm_log.csv");

    // 5. 打印摘要
    print_summary(samples, config.num_channels);

    return 0;
}

// 加载配置
void load_config(SystemConfig *config) {
    strcpy(config->name, "BPM_Monitor_V1");
    config->threshold_low = 0.1;
    config->threshold_high = 1.0;
    config->num_channels = 4;
}

// 采集数据（模拟）
void acquire_data(BPMSample samples[], int count) {
    srand(time(NULL));

    for (int i = 0; i < count; i++) {
        samples[i].channel = i;
        samples[i].amplitude = 0.3 + (rand() % 100) / 100.0;
        samples[i].phase = (rand() % 360);
        samples[i].timestamp = time(NULL);
    }
}

// 处理数据
void process_data(BPMSample samples[], int count, SystemConfig *config) {
    printf("=== Data Processing ===\n");

    for (int i = 0; i < count; i++) {
        printf("CH%d: Amp=%.3fV, Phase=%.1f° ",
               samples[i].channel, samples[i].amplitude, samples[i].phase);

        if (samples[i].amplitude < config->threshold_low) {
            printf("[WARNING: Too Low]");
        } else if (samples[i].amplitude > config->threshold_high) {
            printf("[WARNING: Too High]");
        } else {
            printf("[OK]");
        }
        printf("\n");
    }
    printf("\n");
}

// 保存数据到文件
void save_data(BPMSample samples[], int count, char *filename) {
    FILE *fp = fopen(filename, "w");

    if (fp == NULL) {
        printf("Error: Cannot save data\n");
        return;
    }

    fprintf(fp, "Channel,Amplitude,Phase,Timestamp\n");

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d,%.3f,%.1f,%ld\n",
                samples[i].channel, samples[i].amplitude,
                samples[i].phase, samples[i].timestamp);
    }

    fclose(fp);
    printf("Data saved to %s\n\n", filename);
}

// 打印摘要
void print_summary(BPMSample samples[], int count) {
    float sum = 0.0;

    for (int i = 0; i < count; i++) {
        sum += samples[i].amplitude;
    }

    float average = sum / count;

    printf("=== Summary ===\n");
    printf("Total channels: %d\n", count);
    printf("Average amplitude: %.3f V\n", average);
}
```

---

## 本周知识点总结

**1. 函数**
```c
返回类型 函数名(参数) {
    // 函数体
    return 返回值;
}
```

**2. 结构体**
```c
typedef struct {
    int field1;
    float field2;
} MyStruct;

MyStruct s = {10, 3.14};
printf("%d, %.2f\n", s.field1, s.field2);
```

**3. 结构体指针**
```c
MyStruct *p = &s;
p->field1 = 20;  // 等价于 (*p).field1 = 20;
```

**4. 函数指针**
```c
int (*func_ptr)(int, int);
func_ptr = add;
int result = func_ptr(10, 20);
```

**5. 文件操作**
```c
FILE *fp = fopen("file.txt", "w");
fprintf(fp, "Hello\n");
fclose(fp);
```

---

## 自我检查清单

- [ ] 能够定义和调用函数
- [ ] 理解函数参数传递（值传递、指针传递）
- [ ] 能够定义和使用结构体
- [ ] 能够使用指针访问结构体成员
- [ ] 理解函数指针的概念
- [ ] 能够进行文件读写操作
- [ ] 理解 EPICS Device Support 的基本结构

---

## 与 EPICS 的联系

在 EPICS 中你会看到：

```c
// devBPMMonitor.c 中的结构体
typedef struct {
    strtype_t type;
    unsigned short offset;
    unsigned short channel;
} recordpara_t;

// Device Support 表（函数指针）
struct {
    long number;
    DEVSUPFUN init_record;
    DEVSUPFUN read;
} devAi = {
    6,
    init_record_ai,
    read_ai
};
```

现在你应该能完全理解这些代码了！

---

## 下一步

- **Week 4**: 综合练习和小项目
- **Month 2**: Linux 和 EPICS 基础

**恭喜完成 C 语言基础学习！** 🎉
