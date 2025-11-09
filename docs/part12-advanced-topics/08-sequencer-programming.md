# Sequencer编程

> **目标**: 使用State Notation Language编写状态机
> **难度**: ⭐⭐⭐⭐
> **预计时间**: 3-5天

## SNL基础

### 简单状态机

```c
program sncExample

int running;
assign running to "TEST:Running";

ss mainSequence {
    state idle {
        when (running == 1) {
            printf("Starting...\n");
        } state active
    }
    
    state active {
        when (running == 0) {
            printf("Stopping...\n");
        } state idle
        
        when (delay(1.0)) {
            printf("Active for 1 second\n");
        } state active
    }
}
```

### 多状态集

```c
program multiSS

float temperature;
assign temperature to "TEST:Temperature";
monitor temperature;

int alarm;
assign alarm to "TEST:Alarm";

ss monitor {
    state normal {
        when (temperature > 70) {
            alarm = 1;
            pvPut(alarm);
        } state warning
    }
    
    state warning {
        when (temperature < 65) {
            alarm = 0;
            pvPut(alarm);
        } state normal
    }
}

ss logger {
    state logging {
        when (delay(5.0)) {
            printf("Temperature: %.1f\n", temperature);
        } state logging
    }
}
```

## PV操作

### PV绑定

```c
int setpoint;
assign setpoint to "TEST:Setpoint";
monitor setpoint;  // 监控变化

float readback;
assign readback to "TEST:Readback";

// 写入PV
setpoint = 100;
pvPut(setpoint);

// 读取PV
pvGet(readback);
```

### PV数组

```c
float waveform[1024];
assign waveform to "TEST:Waveform";

// 读取数组
pvGet(waveform);

// 写入数组
for (i = 0; i < 1024; i++) {
    waveform[i] = sin(2 * 3.14159 * i / 1024);
}
pvPut(waveform);
```

## 实战示例：控制循环

```c
program temperatureControl

// PV定义
float temperature;
assign temperature to "LLRF:Temp:Sensor";
monitor temperature;

float setpoint;
assign setpoint to "LLRF:Temp:Setpoint";
monitor setpoint;

float output;
assign output to "LLRF:Temp:Output";

int enable;
assign enable to "LLRF:Temp:Enable";
monitor enable;

// 控制参数
float kp = 1.0;
float ki = 0.1;
float kd = 0.01;

float integral = 0.0;
float last_error = 0.0;

ss control {
    state disabled {
        when (enable == 1) {
            printf("Control enabled\n");
            integral = 0.0;
            last_error = 0.0;
        } state enabled
    }
    
    state enabled {
        when (enable == 0) {
            printf("Control disabled\n");
            output = 0.0;
            pvPut(output);
        } state disabled
        
        when (delay(0.1)) {
            // PID控制算法
            float error = setpoint - temperature;
            integral += error * 0.1;
            float derivative = (error - last_error) / 0.1;
            
            output = kp * error + ki * integral + kd * derivative;
            
            // 限幅
            if (output > 100.0) output = 100.0;
            if (output < 0.0) output = 0.0;
            
            pvPut(output);
            last_error = error;
        } state enabled
    }
}
```

## 编译Sequencer

```makefile
# Makefile
PROD_IOC = myApp

DBD += myApp.dbd

# 添加Sequencer支持
myApp_DBD += base.dbd
myApp_DBD += sncExample.dbd  # 由sncExample.st生成

myApp_SRCS += myApp_registerRecordDeviceDriver.cpp
myApp_SRCS += sncExample.st  # SNL源文件

myApp_LIBS += seq pv
myApp_LIBS += $(EPICS_BASE_IOC_LIBS)
```

## st.cmd中加载

```bash
# st.cmd
dbLoadDatabase("dbd/myApp.dbd")
myApp_registerRecordDeviceDriver(pdbbase)

# 加载数据库
dbLoadRecords("db/test.db")

iocInit()

# 启动Sequencer程序
seq sncExample
```

## 🔗 相关文档

- [03-database-design.md](./03-database-design.md)
- [08-sequencer-programming.md](./08-sequencer-programming.md)
