# 自定义Record类型

> **目标**: 开发自定义Record类型
> **难度**: ⭐⭐⭐⭐⭐
> **预计时间**: 2周

## Record类型基础

### Record结构定义

```c
// myRecord.h
typedef struct myRecord {
    char name[61];         // Record名称
    char desc[41];         // 描述
    epicsEnum16 scan;      // 扫描机制
    epicsEnum16 pini;      // 初始化处理
    epicsInt16 phas;       // 处理阶段
    char evnt[40];         // 事件名
    epicsInt16 tse;        // 时间戳事件
    DBLINK tsel;           // 时间戳链接
    epicsEnum16 dtyp;      // 设备类型
    epicsInt16 disv;       // 禁用值
    epicsInt16 disa;       // 禁用
    DBLINK sdis;           // 扫描禁用输入
    epicsMutexId mlok;     // Monitor锁
    ELLLIST mlis;          // Monitor列表
    ELLLIST bklnk;         // 反向链接
    epicsUInt8 disp;       // 禁用putField
    epicsUInt8 proc;       // 强制处理
    epicsEnum16 stat;      // 告警状态
    epicsEnum16 sevr;      // 告警严重性
    epicsEnum16 nsta;      // 新告警状态
    epicsEnum16 nsev;      // 新告警严重性
    epicsEnum16 acks;      // 告警确认严重性
    epicsEnum16 ackt;      // 告警确认传递
    epicsEnum16 diss;      // 禁用告警严重性
    epicsUInt8 lcnt;       // 锁计数
    epicsUInt8 pact;       // Record活动
    epicsUInt8 putf;       // dbPutField process
    epicsUInt8 rpro;       // 重新处理
    struct asgMember *asp; // 访问安全组
    struct processNotify *ppn; // 处理通知
    struct processNotifyRecord *ppnr;
    struct scan_element *spvt; // 扫描私有数据
    struct typed_rset *rset;   // Record支持入口表
    unambiguous_dset *dset;    // 设备支持入口表
    void *dpvt;            // 设备私有数据
    struct dbRecordType *rdes; // Record描述地址
    struct lockRecord *lset;   // 锁集合
    epicsEnum16 prio;      // 调度优先级
    epicsUInt8 tpro;       // 追踪处理
    epicsUInt8 bkpt;       // 断点
    epicsUInt8 udf;        // 未定义
    epicsEnum16 udfs;      // 未定义告警严重性
    epicsTimeStamp time;   // 时间戳
    DBLINK flnk;           // 前向链接
    
    // 自定义字段
    epicsFloat64 val;      // 当前值
    epicsFloat64 oval;     // 旧值
    DBLINK inp;            // 输入链接
    epicsFloat64 hihi;     // 高高限
    epicsFloat64 lolo;     // 低低限
    epicsFloat64 high;     // 高限
    epicsFloat64 low;      // 低限
    epicsEnum16 hhsv;      // 高高严重性
    epicsEnum16 llsv;      // 低低严重性
    epicsEnum16 hsv;       // 高严重性
    epicsEnum16 lsv;       // 低严重性
    epicsFloat64 hyst;     // 告警死区
    epicsFloat64 adel;     // Archive死区
    epicsFloat64 mdel;     // Monitor死区
} myRecord;
```

### Record定义文件

```
# myRecord.dbd
recordtype(my) {
    include "dbCommon.dbd"
    field(VAL, DBF_DOUBLE) {
        prompt("Current Value")
        asl(ASL0)
        pp(TRUE)
    }
    field(INP, DBF_INLINK) {
        prompt("Input Specification")
        promptgroup("40 - Input")
        interest(1)
    }
    field(HIHI, DBF_DOUBLE) {
        prompt("Hihi Alarm Limit")
        promptgroup("70 - Alarm")
        pp(TRUE)
        interest(1)
    }
    field(LOLO, DBF_DOUBLE) {
        prompt("Lolo Alarm Limit")
        promptgroup("70 - Alarm")
        pp(TRUE)
        interest(1)
    }
}
```

## Record支持表(RSET)

```c
// myRecordRSET.c
static long init_record(myRecord *prec, int pass);
static long process(myRecord *prec);

rset myRSET = {
    RSETNUMBER,
    NULL,                // report
    NULL,                // init
    init_record,         // init_record
    process,             // process
    NULL,                // special
    NULL,                // get_value
    NULL,                // cvt_dbaddr
    NULL,                // get_array_info
    NULL,                // put_array_info
    NULL,                // get_units
    NULL,                // get_precision
    NULL,                // get_enum_str
    NULL,                // get_enum_strs
    NULL,                // put_enum_str
    NULL,                // get_graphic_double
    NULL,                // get_control_double
    NULL                 // get_alarm_double
};

epicsExportAddress(rset, myRSET);

static long init_record(myRecord *prec, int pass) {
    if (pass == 0) return 0;
    
    // 初始化Record
    prec->udf = 0;
    
    return 0;
}

static long process(myRecord *prec) {
    long status;
    
    prec->pact = TRUE;
    
    // 读取输入
    status = dbGetLink(&prec->inp, DBR_DOUBLE, &prec->val, 0, 0);
    
    // 检查告警
    recGblCheckDeadband(&prec->mdel, prec->val, prec->oval,
                        &prec->mlst, &prec->mpst);
    
    // 时间戳
    recGblGetTimeStamp(prec);
    
    // 检查值变化
    if (prec->val != prec->oval) {
        db_post_events(prec, &prec->val, DBE_VALUE | DBE_LOG);
        prec->oval = prec->val;
    }
    
    // 处理前向链接
    recGblFwdLink(prec);
    
    prec->pact = FALSE;
    return 0;
}
```

## 编译自定义Record

```makefile
# Makefile
LIBRARY_IOC += mySupport

DBD += mySupport.dbd

mySupport_SRCS += myRecord.c
mySupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# 在应用的Makefile中
myApp_DBD += mySupport.dbd
myApp_LIBS += mySupport
```

## 使用自定义Record

```
# test.db
record(my, "TEST:MyRecord") {
    field(DESC, "Custom Record Test")
    field(SCAN, "1 second")
    field(INP, "TEST:Value")
    field(HIHI, "100")
    field(LOLO, "-100")
}
```

## 🔗 相关文档

- [03-database-design.md](./03-database-design.md)
- [05-thread-safety.md](./05-thread-safety.md)
