/**
 * generate_sample_data.c - Tool to generate sample RF data file
 *
 * 用法: ./generate_sample_data <output_file> [duration] [interval]
 */

#include "fileReplay.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *filename = "sample_rf_data.csv";
    double duration = 10.0;   // 默认10秒
    double interval = 0.1;    // 默认100ms

    if (argc >= 2) {
        filename = argv[1];
    }

    if (argc >= 3) {
        duration = atof(argv[2]);
    }

    if (argc >= 4) {
        interval = atof(argv[3]);
    }

    printf("Generating sample RF data file\n");
    printf("  Output file: %s\n", filename);
    printf("  Duration: %.1f seconds\n", duration);
    printf("  Interval: %.3f seconds\n", interval);
    printf("\n");

    // 初始化fileReplay模块
    FileReplay_Init();

    // 生成示例文件
    int status = FileReplay_GenerateSampleFile(filename, duration, interval);

    if (status == 0) {
        printf("\n✓ Sample data file generated successfully!\n");
        printf("\nTo use this file:\n");
        printf("  export BPMIOC_SIM_MODE=2\n");
        printf("  export BPMIOC_DATA_FILE=%s\n", filename);
        return 0;
    } else {
        printf("\n✗ Failed to generate sample data file\n");
        return 1;
    }
}
