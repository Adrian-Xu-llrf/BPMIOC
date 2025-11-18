# Week 4 - 综合练习

**学习时间**: 14 小时（每天 2 小时）

**本周目标**: 通过实际项目巩固前三周所学，准备进入 EPICS 开发

**重点**：
- 综合运用变量、指针、结构体、函数
- 完成多个实践项目
- 理解如何将 C 语言知识应用到 EPICS IOC

---

## 第1天：项目1 - 简单的数据采集系统（2小时）

### 项目需求

实现一个模拟的多通道数据采集系统，包含：
1. 采集4个通道的数据
2. 数据验证（范围检查）
3. 统计分析（最大值、最小值、平均值）
4. 数据存储到文件

### 完整代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_CHANNELS 4
#define THRESHOLD_LOW 0.0
#define THRESHOLD_HIGH 1.0

// 通道数据结构
typedef struct {
    int id;
    float value;
    int is_valid;
} ChannelData;

// 统计信息结构
typedef struct {
    float max;
    float min;
    float average;
    int valid_count;
    int invalid_count;
} Statistics;

// 函数声明
void acquire_data(ChannelData channels[], int count);
void validate_data(ChannelData channels[], int count);
void calculate_statistics(ChannelData channels[], int count, Statistics *stats);
void print_data(ChannelData channels[], int count);
void print_statistics(Statistics *stats);
void save_to_file(ChannelData channels[], int count, Statistics *stats);

int main() {
    ChannelData channels[NUM_CHANNELS];
    Statistics stats;

    printf("=== Multi-Channel Data Acquisition System ===\n\n");

    // 1. 采集数据
    printf("Step 1: Acquiring data...\n");
    acquire_data(channels, NUM_CHANNELS);
    printf("Done.\n\n");

    // 2. 验证数据
    printf("Step 2: Validating data...\n");
    validate_data(channels, NUM_CHANNELS);
    printf("Done.\n\n");

    // 3. 显示数据
    printf("Step 3: Channel Data\n");
    print_data(channels, NUM_CHANNELS);

    // 4. 计算统计
    printf("\nStep 4: Calculating statistics...\n");
    calculate_statistics(channels, NUM_CHANNELS, &stats);
    print_statistics(&stats);

    // 5. 保存到文件
    printf("\nStep 5: Saving to file...\n");
    save_to_file(channels, NUM_CHANNELS, &stats);

    printf("\n=== System completed successfully ===\n");

    return 0;
}

// 采集数据（模拟）
void acquire_data(ChannelData channels[], int count) {
    srand(time(NULL));

    for (int i = 0; i < count; i++) {
        channels[i].id = i;
        // 生成 -0.2 到 1.2 的随机值
        channels[i].value = -0.2 + (rand() % 140) / 100.0;
        channels[i].is_valid = 1;  // 初始标记为有效
    }
}

// 验证数据
void validate_data(ChannelData channels[], int count) {
    for (int i = 0; i < count; i++) {
        if (channels[i].value < THRESHOLD_LOW || channels[i].value > THRESHOLD_HIGH) {
            channels[i].is_valid = 0;
        }
    }
}

// 计算统计信息
void calculate_statistics(ChannelData channels[], int count, Statistics *stats) {
    stats->max = channels[0].value;
    stats->min = channels[0].value;
    stats->valid_count = 0;
    stats->invalid_count = 0;

    float sum = 0.0;
    int valid_sum_count = 0;

    for (int i = 0; i < count; i++) {
        // 统计有效/无效数量
        if (channels[i].is_valid) {
            stats->valid_count++;
            sum += channels[i].value;
            valid_sum_count++;

            // 更新最大最小值（只考虑有效值）
            if (channels[i].value > stats->max) {
                stats->max = channels[i].value;
            }
            if (channels[i].value < stats->min) {
                stats->min = channels[i].value;
            }
        } else {
            stats->invalid_count++;
        }
    }

    // 计算平均值
    if (valid_sum_count > 0) {
        stats->average = sum / valid_sum_count;
    } else {
        stats->average = 0.0;
    }
}

// 打印数据
void print_data(ChannelData channels[], int count) {
    printf("┌─────────┬───────────┬──────────┐\n");
    printf("│ Channel │   Value   │  Status  │\n");
    printf("├─────────┼───────────┼──────────┤\n");

    for (int i = 0; i < count; i++) {
        printf("│   %2d    │  %6.3f   │   %s   │\n",
               channels[i].id,
               channels[i].value,
               channels[i].is_valid ? "OK  " : "FAIL");
    }

    printf("└─────────┴───────────┴──────────┘\n");
}

// 打印统计信息
void print_statistics(Statistics *stats) {
    printf("Statistics:\n");
    printf("  Valid channels:   %d\n", stats->valid_count);
    printf("  Invalid channels: %d\n", stats->invalid_count);
    printf("  Maximum value:    %.3f\n", stats->max);
    printf("  Minimum value:    %.3f\n", stats->min);
    printf("  Average value:    %.3f\n", stats->average);
}

// 保存到文件
void save_to_file(ChannelData channels[], int count, Statistics *stats) {
    FILE *fp = fopen("data_log.txt", "w");

    if (fp == NULL) {
        printf("Error: Cannot create file\n");
        return;
    }

    // 写入时间戳
    time_t now = time(NULL);
    fprintf(fp, "Data Acquisition Log\n");
    fprintf(fp, "Timestamp: %s\n", ctime(&now));

    // 写入数据
    fprintf(fp, "\nChannel Data:\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "CH%d: %.3f [%s]\n",
                channels[i].id,
                channels[i].value,
                channels[i].is_valid ? "VALID" : "INVALID");
    }

    // 写入统计
    fprintf(fp, "\nStatistics:\n");
    fprintf(fp, "  Valid:   %d\n", stats->valid_count);
    fprintf(fp, "  Invalid: %d\n", stats->invalid_count);
    fprintf(fp, "  Max:     %.3f\n", stats->max);
    fprintf(fp, "  Min:     %.3f\n", stats->min);
    fprintf(fp, "  Average: %.3f\n", stats->average);

    fclose(fp);
    printf("Data saved to data_log.txt\n");
}
```

### 练习1

扩展程序功能：
1. 添加数据趋势分析（连续3次数据上升或下降）
2. 添加报警功能（值超出范围时发出警告）
3. 支持从文件读取阈值配置

---

## 第2天：项目2 - 命令行解析器（2小时）

### 项目需求

实现一个简单的命令行界面，支持以下命令：
- `help` - 显示帮助信息
- `read <channel>` - 读取指定通道数据
- `write <channel> <value>` - 写入数据到通道
- `status` - 显示所有通道状态
- `quit` - 退出程序

### 完整代码

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_CHANNELS 4
#define MAX_COMMAND_LEN 100

// 通道结构
typedef struct {
    int id;
    float value;
    int enabled;
} Channel;

// 命令处理函数类型
typedef void (*CommandHandler)(Channel channels[], char *args);

// 命令结构
typedef struct {
    char name[20];
    char description[100];
    CommandHandler handler;
} Command;

// 全局通道数据
Channel g_channels[NUM_CHANNELS];

// 函数声明
void init_channels();
void cmd_help(Channel channels[], char *args);
void cmd_read(Channel channels[], char *args);
void cmd_write(Channel channels[], char *args);
void cmd_status(Channel channels[], char *args);
void cmd_quit(Channel channels[], char *args);
void process_command(char *input, Command commands[], int num_commands);

// 命令表
Command g_commands[] = {
    {"help", "Display this help message", cmd_help},
    {"read", "Read channel value (usage: read <channel>)", cmd_read},
    {"write", "Write channel value (usage: write <channel> <value>)", cmd_write},
    {"status", "Display all channel status", cmd_status},
    {"quit", "Exit the program", cmd_quit}
};

int g_running = 1;  // 程序运行标志

int main() {
    char input[MAX_COMMAND_LEN];

    init_channels();

    printf("=== Channel Control System ===\n");
    printf("Type 'help' for available commands\n\n");

    while (g_running) {
        printf("> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // 移除换行符
        input[strcspn(input, "\n")] = 0;

        // 跳过空命令
        if (strlen(input) == 0) {
            continue;
        }

        process_command(input, g_commands, 5);
    }

    printf("\nGoodbye!\n");

    return 0;
}

// 初始化通道
void init_channels() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        g_channels[i].id = i;
        g_channels[i].value = 0.0;
        g_channels[i].enabled = 1;
    }
}

// 处理命令
void process_command(char *input, Command commands[], int num_commands) {
    char cmd[20];
    char args[MAX_COMMAND_LEN];

    // 分离命令和参数
    int n = sscanf(input, "%s %[^\n]", cmd, args);

    if (n < 1) {
        return;
    }

    if (n == 1) {
        args[0] = '\0';  // 无参数
    }

    // 查找并执行命令
    int found = 0;
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            commands[i].handler(g_channels, args);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'help' for available commands\n");
    }
}

// 命令实现
void cmd_help(Channel channels[], char *args) {
    printf("\nAvailable commands:\n");
    for (int i = 0; i < 5; i++) {
        printf("  %-10s - %s\n", g_commands[i].name, g_commands[i].description);
    }
    printf("\n");
}

void cmd_read(Channel channels[], char *args) {
    int channel;

    if (sscanf(args, "%d", &channel) != 1) {
        printf("Error: Please specify channel number (0-%d)\n", NUM_CHANNELS - 1);
        return;
    }

    if (channel < 0 || channel >= NUM_CHANNELS) {
        printf("Error: Invalid channel %d\n", channel);
        return;
    }

    printf("Channel %d: %.3f [%s]\n",
           channel,
           channels[channel].value,
           channels[channel].enabled ? "Enabled" : "Disabled");
}

void cmd_write(Channel channels[], char *args) {
    int channel;
    float value;

    if (sscanf(args, "%d %f", &channel, &value) != 2) {
        printf("Error: Usage: write <channel> <value>\n");
        return;
    }

    if (channel < 0 || channel >= NUM_CHANNELS) {
        printf("Error: Invalid channel %d\n", channel);
        return;
    }

    if (!channels[channel].enabled) {
        printf("Error: Channel %d is disabled\n", channel);
        return;
    }

    channels[channel].value = value;
    printf("Channel %d set to %.3f\n", channel, value);
}

void cmd_status(Channel channels[], char *args) {
    printf("\n┌─────────┬───────────┬──────────┐\n");
    printf("│ Channel │   Value   │  Status  │\n");
    printf("├─────────┼───────────┼──────────┤\n");

    for (int i = 0; i < NUM_CHANNELS; i++) {
        printf("│   %2d    │  %6.3f   │ %-8s │\n",
               channels[i].id,
               channels[i].value,
               channels[i].enabled ? "Enabled" : "Disabled");
    }

    printf("└─────────┴───────────┴──────────┘\n\n");
}

void cmd_quit(Channel channels[], char *args) {
    printf("Exiting...\n");
    g_running = 0;
}
```

### 练习2

扩展功能：
1. 添加 `enable <channel>` 和 `disable <channel>` 命令
2. 添加 `readall` 命令读取所有通道
3. 添加 `save` 和 `load` 命令保存/加载通道状态

---

## 第3天：项目3 - 简化的 EPICS Record 系统（2小时）

### 项目需求

模拟 EPICS 的 Record 处理机制：
1. 定义 Record 结构
2. 实现 Device Support 接口
3. 模拟硬件读写
4. 处理多个 Record

### 完整代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 模拟硬件寄存器
float g_hardware_registers[16] = {0};

// Record 类型
typedef enum {
    RECORD_AI,  // 模拟输入
    RECORD_AO   // 模拟输出
} RecordType;

// Record 结构
typedef struct {
    char name[50];
    RecordType type;
    float val;
    int hardware_addr;
    void *dset;  // Device Support
} Record;

// Device Support 函数指针表
typedef struct {
    long (*init_record)(Record *rec);
    long (*process)(Record *rec);
} DSET;

// 函数声明
long ai_init_record(Record *rec);
long ai_process(Record *rec);
long ao_init_record(Record *rec);
long ao_process(Record *rec);
float read_hardware(int addr);
void write_hardware(int addr, float value);
void simulate_hardware();

// Device Support 表
DSET ai_dset = {ai_init_record, ai_process};
DSET ao_dset = {ao_init_record, ao_process};

int main() {
    // 初始化硬件
    simulate_hardware();

    // 创建 Record
    Record records[6] = {
        {"BPM:CH0:Amp", RECORD_AI, 0.0, 0, &ai_dset},
        {"BPM:CH1:Amp", RECORD_AI, 0.0, 1, &ai_dset},
        {"BPM:CH2:Amp", RECORD_AI, 0.0, 2, &ai_dset},
        {"BPM:CH3:Amp", RECORD_AI, 0.0, 3, &ai_dset},
        {"BPM:Gain:Set", RECORD_AO, 0.0, 8, &ao_dset},
        {"BPM:Offset:Set", RECORD_AO, 0.0, 9, &ao_dset}
    };

    printf("=== Simplified EPICS Record System ===\n\n");

    // 初始化所有 Record
    printf("Initializing records...\n");
    for (int i = 0; i < 6; i++) {
        DSET *dset = (DSET *)records[i].dset;
        dset->init_record(&records[i]);
    }
    printf("\n");

    // 处理输入 Record
    printf("Processing AI records...\n");
    for (int i = 0; i < 4; i++) {
        DSET *dset = (DSET *)records[i].dset;
        dset->process(&records[i]);
        printf("  %s = %.3f\n", records[i].name, records[i].val);
    }
    printf("\n");

    // 写入输出 Record
    printf("Writing to AO records...\n");
    records[4].val = 1.5;  // BPM:Gain:Set
    records[5].val = 0.25; // BPM:Offset:Set

    for (int i = 4; i < 6; i++) {
        DSET *dset = (DSET *)records[i].dset;
        dset->process(&records[i]);
    }
    printf("\n");

    // 再次读取输入（验证写入）
    printf("Reading AI records again...\n");
    for (int i = 0; i < 4; i++) {
        DSET *dset = (DSET *)records[i].dset;
        dset->process(&records[i]);
        printf("  %s = %.3f\n", records[i].name, records[i].val);
    }

    return 0;
}

// AI Record 初始化
long ai_init_record(Record *rec) {
    printf("  [AI] Init: %s (addr=0x%04X)\n", rec->name, rec->hardware_addr);
    return 0;
}

// AI Record 处理（读取）
long ai_process(Record *rec) {
    rec->val = read_hardware(rec->hardware_addr);
    return 0;
}

// AO Record 初始化
long ao_init_record(Record *rec) {
    printf("  [AO] Init: %s (addr=0x%04X)\n", rec->name, rec->hardware_addr);
    return 0;
}

// AO Record 处理（写入）
long ao_process(Record *rec) {
    write_hardware(rec->hardware_addr, rec->val);
    printf("  %s = %.3f written to addr 0x%04X\n",
           rec->name, rec->val, rec->hardware_addr);
    return 0;
}

// 读取硬件寄存器
float read_hardware(int addr) {
    if (addr >= 0 && addr < 16) {
        return g_hardware_registers[addr];
    }
    return 0.0;
}

// 写入硬件寄存器
void write_hardware(int addr, float value) {
    if (addr >= 0 && addr < 16) {
        g_hardware_registers[addr] = value;
    }
}

// 模拟硬件初始值
void simulate_hardware() {
    g_hardware_registers[0] = 0.123;  // CH0 Amp
    g_hardware_registers[1] = 0.456;  // CH1 Amp
    g_hardware_registers[2] = 0.789;  // CH2 Amp
    g_hardware_registers[3] = 0.234;  // CH3 Amp
    g_hardware_registers[8] = 1.0;    // Gain
    g_hardware_registers[9] = 0.0;    // Offset
}
```

### 练习3

扩展功能：
1. 添加 BI/BO Record 支持（数字量输入输出）
2. 实现 Record 扫描（定期自动更新）
3. 添加数据转换功能（原始值 → 工程单位）

---

## 第4天：项目4 - 内存管理练习（2小时）

### 项目需求

实现一个动态缓冲区管理系统：
1. 动态分配不同大小的缓冲区
2. 缓冲区读写操作
3. 内存泄漏检测
4. 缓冲区池管理

### 完整代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 缓冲区结构
typedef struct {
    int id;
    float *data;
    int size;
    int used;
    int allocated;
} Buffer;

// 缓冲区池
typedef struct {
    Buffer *buffers;
    int count;
    int capacity;
    int total_allocated;
    int total_freed;
} BufferPool;

// 函数声明
BufferPool* create_pool(int capacity);
void destroy_pool(BufferPool *pool);
int allocate_buffer(BufferPool *pool, int size);
void free_buffer(BufferPool *pool, int id);
void write_to_buffer(BufferPool *pool, int id, float *data, int count);
void read_from_buffer(BufferPool *pool, int id, float *data, int count);
void print_pool_status(BufferPool *pool);
void check_memory_leaks(BufferPool *pool);

int main() {
    printf("=== Buffer Management System ===\n\n");

    // 创建缓冲区池
    BufferPool *pool = create_pool(10);

    // 分配几个缓冲区
    printf("Allocating buffers...\n");
    int buf1 = allocate_buffer(pool, 100);
    int buf2 = allocate_buffer(pool, 200);
    int buf3 = allocate_buffer(pool, 50);
    printf("Allocated: buf1=%d, buf2=%d, buf3=%d\n\n", buf1, buf2, buf3);

    // 写入数据
    printf("Writing data to buffers...\n");
    float test_data[10] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    write_to_buffer(pool, buf1, test_data, 10);
    printf("\n");

    // 读取数据
    printf("Reading data from buffer...\n");
    float read_data[10];
    read_from_buffer(pool, buf1, read_data, 10);
    printf("Data: ");
    for (int i = 0; i < 10; i++) {
        printf("%.1f ", read_data[i]);
    }
    printf("\n\n");

    // 显示状态
    print_pool_status(pool);

    // 释放一个缓冲区
    printf("\nFreeing buffer %d...\n", buf2);
    free_buffer(pool, buf2);

    // 再次显示状态
    print_pool_status(pool);

    // 检查内存泄漏
    printf("\nChecking for memory leaks...\n");
    check_memory_leaks(pool);

    // 销毁池
    destroy_pool(pool);

    printf("\n=== Program completed ===\n");

    return 0;
}

// 创建缓冲区池
BufferPool* create_pool(int capacity) {
    BufferPool *pool = (BufferPool *)malloc(sizeof(BufferPool));

    if (pool == NULL) {
        printf("Error: Cannot allocate pool\n");
        return NULL;
    }

    pool->buffers = (Buffer *)calloc(capacity, sizeof(Buffer));
    pool->count = 0;
    pool->capacity = capacity;
    pool->total_allocated = 0;
    pool->total_freed = 0;

    return pool;
}

// 销毁缓冲区池
void destroy_pool(BufferPool *pool) {
    if (pool == NULL) return;

    // 释放所有缓冲区
    for (int i = 0; i < pool->count; i++) {
        if (pool->buffers[i].allocated && pool->buffers[i].data != NULL) {
            free(pool->buffers[i].data);
        }
    }

    free(pool->buffers);
    free(pool);
}

// 分配缓冲区
int allocate_buffer(BufferPool *pool, int size) {
    if (pool->count >= pool->capacity) {
        printf("Error: Pool is full\n");
        return -1;
    }

    int id = pool->count;

    pool->buffers[id].id = id;
    pool->buffers[id].size = size;
    pool->buffers[id].used = 0;
    pool->buffers[id].allocated = 1;
    pool->buffers[id].data = (float *)calloc(size, sizeof(float));

    if (pool->buffers[id].data == NULL) {
        printf("Error: Cannot allocate buffer data\n");
        return -1;
    }

    pool->count++;
    pool->total_allocated++;

    printf("  Buffer %d allocated (%d floats, %lu bytes)\n",
           id, size, size * sizeof(float));

    return id;
}

// 释放缓冲区
void free_buffer(BufferPool *pool, int id) {
    if (id < 0 || id >= pool->count) {
        printf("Error: Invalid buffer ID\n");
        return;
    }

    if (!pool->buffers[id].allocated) {
        printf("Error: Buffer %d already freed\n", id);
        return;
    }

    free(pool->buffers[id].data);
    pool->buffers[id].data = NULL;
    pool->buffers[id].allocated = 0;
    pool->total_freed++;

    printf("  Buffer %d freed\n", id);
}

// 写入缓冲区
void write_to_buffer(BufferPool *pool, int id, float *data, int count) {
    if (id < 0 || id >= pool->count) {
        printf("Error: Invalid buffer ID\n");
        return;
    }

    if (!pool->buffers[id].allocated) {
        printf("Error: Buffer %d not allocated\n", id);
        return;
    }

    if (count > pool->buffers[id].size) {
        printf("Error: Data size exceeds buffer size\n");
        return;
    }

    memcpy(pool->buffers[id].data, data, count * sizeof(float));
    pool->buffers[id].used = count;

    printf("  Written %d floats to buffer %d\n", count, id);
}

// 读取缓冲区
void read_from_buffer(BufferPool *pool, int id, float *data, int count) {
    if (id < 0 || id >= pool->count) {
        printf("Error: Invalid buffer ID\n");
        return;
    }

    if (!pool->buffers[id].allocated) {
        printf("Error: Buffer %d not allocated\n", id);
        return;
    }

    int read_count = (count < pool->buffers[id].used) ? count : pool->buffers[id].used;

    memcpy(data, pool->buffers[id].data, read_count * sizeof(float));

    printf("  Read %d floats from buffer %d\n", read_count, id);
}

// 打印池状态
void print_pool_status(BufferPool *pool) {
    printf("\n=== Buffer Pool Status ===\n");
    printf("Total buffers: %d / %d\n", pool->count, pool->capacity);
    printf("Total allocated: %d\n", pool->total_allocated);
    printf("Total freed: %d\n", pool->total_freed);

    printf("\nBuffer Details:\n");
    printf("┌────┬──────────┬──────┬──────┬───────────┐\n");
    printf("│ ID │   Size   │ Used │ Size │  Status   │\n");
    printf("├────┼──────────┼──────┼──────┼───────────┤\n");

    for (int i = 0; i < pool->count; i++) {
        printf("│ %2d │ %8d │ %4d │ %4d │ %-9s │\n",
               pool->buffers[i].id,
               pool->buffers[i].size,
               pool->buffers[i].used,
               pool->buffers[i].size,
               pool->buffers[i].allocated ? "Allocated" : "Freed");
    }

    printf("└────┴──────────┴──────┴──────┴───────────┘\n");
}

// 检查内存泄漏
void check_memory_leaks(BufferPool *pool) {
    int leaks = 0;

    for (int i = 0; i < pool->count; i++) {
        if (pool->buffers[i].allocated) {
            leaks++;
        }
    }

    if (leaks > 0) {
        printf("  WARNING: %d buffer(s) not freed!\n", leaks);
        printf("  Please free all buffers before exit.\n");
    } else {
        printf("  No memory leaks detected.\n");
    }
}
```

### 练习4

扩展功能：
1. 实现缓冲区自动扩容
2. 添加缓冲区使用统计
3. 实现缓冲区重用机制

---

## 第5-6天：综合大项目 - BPM 监控系统（4小时）

### 项目需求

实现一个完整的 BPM 监控系统，包含：
1. 数据采集模块
2. 数据处理模块
3. 命令行界面
4. 文件存储
5. 统计分析

### 项目结构

```
bpm_monitor/
├── main.c
├── bpm_data.h
├── bpm_data.c
├── hardware.h
├── hardware.c
├── commands.h
└── commands.c
```

### bpm_data.h

```c
#ifndef BPM_DATA_H
#define BPM_DATA_H

#include <time.h>

#define NUM_CHANNELS 4

typedef struct {
    int channel;
    float amplitude;
    float phase;
    time_t timestamp;
    int valid;
} BPMSample;

typedef struct {
    BPMSample samples[NUM_CHANNELS];
    float avg_amplitude;
    float max_amplitude;
    float min_amplitude;
} BPMSnapshot;

typedef struct {
    BPMSnapshot *snapshots;
    int count;
    int capacity;
} BPMHistory;

// 函数声明
BPMHistory* create_history(int capacity);
void destroy_history(BPMHistory *history);
void add_snapshot(BPMHistory *history, BPMSnapshot *snapshot);
void calculate_snapshot_stats(BPMSnapshot *snapshot);
void print_snapshot(BPMSnapshot *snapshot);
void save_history(BPMHistory *history, const char *filename);

#endif
```

### bpm_data.c

```c
#include "bpm_data.h"
#include <stdio.h>
#include <stdlib.h>

BPMHistory* create_history(int capacity) {
    BPMHistory *history = (BPMHistory *)malloc(sizeof(BPMHistory));
    history->snapshots = (BPMSnapshot *)calloc(capacity, sizeof(BPMSnapshot));
    history->count = 0;
    history->capacity = capacity;
    return history;
}

void destroy_history(BPMHistory *history) {
    if (history) {
        free(history->snapshots);
        free(history);
    }
}

void add_snapshot(BPMHistory *history, BPMSnapshot *snapshot) {
    if (history->count < history->capacity) {
        history->snapshots[history->count++] = *snapshot;
    }
}

void calculate_snapshot_stats(BPMSnapshot *snapshot) {
    float sum = 0.0;
    int valid_count = 0;

    snapshot->max_amplitude = snapshot->samples[0].amplitude;
    snapshot->min_amplitude = snapshot->samples[0].amplitude;

    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (snapshot->samples[i].valid) {
            sum += snapshot->samples[i].amplitude;
            valid_count++;

            if (snapshot->samples[i].amplitude > snapshot->max_amplitude) {
                snapshot->max_amplitude = snapshot->samples[i].amplitude;
            }
            if (snapshot->samples[i].amplitude < snapshot->min_amplitude) {
                snapshot->min_amplitude = snapshot->samples[i].amplitude;
            }
        }
    }

    snapshot->avg_amplitude = (valid_count > 0) ? sum / valid_count : 0.0;
}

void print_snapshot(BPMSnapshot *snapshot) {
    printf("\n=== BPM Snapshot ===\n");

    for (int i = 0; i < NUM_CHANNELS; i++) {
        printf("CH%d: Amp=%.3fV, Phase=%.1f° [%s]\n",
               snapshot->samples[i].channel,
               snapshot->samples[i].amplitude,
               snapshot->samples[i].phase,
               snapshot->samples[i].valid ? "OK" : "INVALID");
    }

    printf("\nStatistics:\n");
    printf("  Average: %.3f V\n", snapshot->avg_amplitude);
    printf("  Maximum: %.3f V\n", snapshot->max_amplitude);
    printf("  Minimum: %.3f V\n", snapshot->min_amplitude);
}

void save_history(BPMHistory *history, const char *filename) {
    FILE *fp = fopen(filename, "w");

    if (!fp) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }

    fprintf(fp, "Timestamp,Channel,Amplitude,Phase,Valid\n");

    for (int i = 0; i < history->count; i++) {
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            fprintf(fp, "%ld,%d,%.3f,%.1f,%d\n",
                    history->snapshots[i].samples[ch].timestamp,
                    history->snapshots[i].samples[ch].channel,
                    history->snapshots[i].samples[ch].amplitude,
                    history->snapshots[i].samples[ch].phase,
                    history->snapshots[i].samples[ch].valid);
        }
    }

    fclose(fp);
    printf("History saved to %s (%d snapshots)\n", filename, history->count);
}
```

### hardware.h

```c
#ifndef HARDWARE_H
#define HARDWARE_H

void init_hardware(void);
float read_amplitude(int channel);
float read_phase(int channel);
void write_register(int addr, float value);

#endif
```

### hardware.c

```c
#include "hardware.h"
#include <stdlib.h>
#include <time.h>

static int g_initialized = 0;

void init_hardware(void) {
    if (!g_initialized) {
        srand(time(NULL));
        g_initialized = 1;
    }
}

float read_amplitude(int channel) {
    // 模拟硬件读取（基础值 + 噪声）
    float base = 0.5;
    float noise = (rand() % 100 - 50) / 1000.0;  // ±0.05
    return base + noise;
}

float read_phase(int channel) {
    // 模拟相位读取
    return channel * 90.0 + (rand() % 20 - 10);  // channel * 90° ± 10°
}

void write_register(int addr, float value) {
    // 模拟写入硬件寄存器
    printf("  [HW] Write addr 0x%04X = %.3f\n", addr, value);
}
```

### commands.h

```c
#ifndef COMMANDS_H
#define COMMANDS_H

#include "bpm_data.h"

void process_commands(BPMHistory *history);

#endif
```

### commands.c

```c
#include "commands.h"
#include "hardware.h"
#include <stdio.h>
#include <string.h>

static void cmd_acquire(BPMHistory *history);
static void cmd_status(BPMHistory *history);
static void cmd_history(BPMHistory *history);
static void cmd_save(BPMHistory *history);
static void cmd_help(void);

void process_commands(BPMHistory *history) {
    char input[100];
    int running = 1;

    printf("\nBPM Monitor Command Interface\n");
    printf("Type 'help' for commands\n\n");

    while (running) {
        printf("bpm> ");

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;

        if (strcmp(input, "acquire") == 0) {
            cmd_acquire(history);
        } else if (strcmp(input, "status") == 0) {
            cmd_status(history);
        } else if (strcmp(input, "history") == 0) {
            cmd_history(history);
        } else if (strcmp(input, "save") == 0) {
            cmd_save(history);
        } else if (strcmp(input, "help") == 0) {
            cmd_help();
        } else if (strcmp(input, "quit") == 0) {
            running = 0;
        } else {
            printf("Unknown command: %s\n", input);
        }
    }
}

static void cmd_acquire(BPMHistory *history) {
    BPMSnapshot snapshot;
    time_t now = time(NULL);

    printf("Acquiring data...\n");

    for (int i = 0; i < NUM_CHANNELS; i++) {
        snapshot.samples[i].channel = i;
        snapshot.samples[i].amplitude = read_amplitude(i);
        snapshot.samples[i].phase = read_phase(i);
        snapshot.samples[i].timestamp = now;
        snapshot.samples[i].valid = (snapshot.samples[i].amplitude >= 0.0 &&
                                     snapshot.samples[i].amplitude <= 1.0);
    }

    calculate_snapshot_stats(&snapshot);
    add_snapshot(history, &snapshot);

    printf("Data acquired.\n");
}

static void cmd_status(BPMHistory *history) {
    if (history->count == 0) {
        printf("No data available. Use 'acquire' first.\n");
        return;
    }

    BPMSnapshot *latest = &history->snapshots[history->count - 1];
    print_snapshot(latest);
}

static void cmd_history(BPMHistory *history) {
    printf("\nHistory: %d / %d snapshots\n", history->count, history->capacity);

    if (history->count == 0) {
        printf("No data available.\n");
        return;
    }

    printf("\n┌───────┬─────────────┬─────────────┬─────────────┐\n");
    printf("│  No.  │   Average   │   Maximum   │   Minimum   │\n");
    printf("├───────┼─────────────┼─────────────┼─────────────┤\n");

    for (int i = 0; i < history->count; i++) {
        printf("│  %3d  │   %.3f V   │   %.3f V   │   %.3f V   │\n",
               i + 1,
               history->snapshots[i].avg_amplitude,
               history->snapshots[i].max_amplitude,
               history->snapshots[i].min_amplitude);
    }

    printf("└───────┴─────────────┴─────────────┴─────────────┘\n");
}

static void cmd_save(BPMHistory *history) {
    if (history->count == 0) {
        printf("No data to save.\n");
        return;
    }

    save_history(history, "bpm_history.csv");
}

static void cmd_help(void) {
    printf("\nAvailable commands:\n");
    printf("  acquire  - Acquire new BPM data\n");
    printf("  status   - Show latest snapshot\n");
    printf("  history  - Show all snapshots\n");
    printf("  save     - Save history to file\n");
    printf("  help     - Show this message\n");
    printf("  quit     - Exit program\n");
}
```

### main.c

```c
#include <stdio.h>
#include "bpm_data.h"
#include "hardware.h"
#include "commands.h"

int main() {
    printf("=== BPM Monitoring System ===\n");
    printf("Version 1.0\n\n");

    // 初始化硬件
    init_hardware();

    // 创建历史记录
    BPMHistory *history = create_history(100);

    // 进入命令循环
    process_commands(history);

    // 清理
    destroy_history(history);

    printf("\nGoodbye!\n");

    return 0;
}
```

### 编译和运行

```bash
# 编译
gcc -o bpm_monitor main.c bpm_data.c hardware.c commands.c

# 运行
./bpm_monitor
```

---

## 第7天：总结和测试（2小时）

### 本月学习总结

**已掌握的知识**：
1. C 语言基础语法
2. 变量、数据类型、运算符
3. 控制结构（if, for, while）
4. 数组
5. 指针（最重要！）
6. 结构体
7. 函数
8. 文件操作
9. 动态内存管理

**与 EPICS 的联系**：
- Record 就是结构体
- Device Support 就是函数指针表
- dpvt 就是指针
- 读写硬件就是指针操作

### 综合测试题

**1. 基础题**
```c
// 下面代码的输出是什么？
int x = 10;
int *p = &x;
*p = 20;
printf("%d\n", x);
```

**2. 指针题**
```c
// 实现函数交换两个数
void swap(int *a, int *b) {
    // 你的代码
}
```

**3. 结构体题**
```c
// 定义一个学生结构体，包含姓名、年龄、成绩
// 实现函数计算平均分
```

**4. 文件操作题**
```c
// 编写程序读取一个文本文件，统计行数和字数
```

**5. 综合题**
```c
// 实现一个简单的通讯录程序
// - 添加联系人
// - 查找联系人
// - 删除联系人
// - 保存到文件
```

### 学习建议

**继续深入**：
1. 阅读优秀的 C 代码
2. 多写代码练习
3. 尝试修改 BPMIOC 项目代码

**下一步**：
- 开始学习 Linux 基础
- 安装 EPICS Base
- 运行第一个 IOC

---

## 总结

恭喜完成第1个月的学习！你已经掌握了：
- ✅ C 语言基础
- ✅ 指针的使用
- ✅ 结构体和函数
- ✅ 基本的编程思维

**现在你可以**：
- 读懂简单的 EPICS 代码
- 编写基本的 C 程序
- 理解 EPICS 的架构概念

**下个月你将学习**：
- Linux 命令行
- EPICS 安装和配置
- 运行和修改 IOC

继续加油！💪
