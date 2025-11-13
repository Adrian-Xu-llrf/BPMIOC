/**
 * @file mock_lowlevel.h
 * @brief Mock implementation of low-level hardware driver interface
 *
 * This header defines the mock driver interface that simulates the real
 * liblowlevel.so hardware driver for testing purposes.
 */

#ifndef MOCK_LOWLEVEL_H
#define MOCK_LOWLEVEL_H

#include <stdint.h>

/* Test control functions */
void mock_set_test_mode(int mode);
void mock_reset_state(void);
void mock_set_rf_amplitude(int channel, float value);
void mock_set_rf_phase(int channel, float value);
void mock_set_bpm_position(float x1, float y1, float x2, float y2);
void mock_enable_error_injection(int enable);
void mock_set_timestamp(uint32_t sec, uint32_t nsec);

/* Simulated hardware functions - matching real driver API */
void* GetRfInfo(void);
void* GetBPMPhaseValue(void);
void* GetxyPosition(void);

int SetBPMk1(float value);
int SetBPMk2(float value);
int SetBPMk3(float value);
int SetBPMPhaseOffset(float value);
int SetBPMkxy(float kx, float ky);
int SetBPMxyOffset(float x_offset, float y_offset);
int SetBPMxyLimits(float x_min, float x_max, float y_min, float y_max);

int SetHistoryTrigger(int enable);
int SetTriggerExtractDataRatio(int ratio);
int GetHistoryDataReady(void);
void* GetTriggerAllData(void);
void* GetHistoryChannelData(int channel);

int GetDI(int channel);
int SetDO(int channel, int value);
int GetFPGA_LED0_RBK(void);
int GetFPGA_LED1_RBK(void);
int SetArmLedEnable(int enable);
int SetFanLedStatus(int status);
int GetADclkState(void);

int GetWRStatus(void);
int SetWRCaputureDataTrigger(int enable);
void* GetTimestampData(void);

/* Statistics and verification functions */
typedef struct {
    int read_count;
    int write_count;
    int error_count;
    int trigger_count;
} MockStats;

MockStats* mock_get_statistics(void);
int mock_verify_call_sequence(const char* expected_sequence);

#endif /* MOCK_LOWLEVEL_H */
