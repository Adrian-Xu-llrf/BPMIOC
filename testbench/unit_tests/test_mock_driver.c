/**
 * @file test_mock_driver.c
 * @brief Unit tests for the mock hardware driver
 */

#include "test_framework.h"
#include "../mock_driver/mock_lowlevel.h"
#include <stdlib.h>

/* Test: Mock driver initialization */
TEST(mock_init) {
    mock_reset_state();
    MockStats* stats = mock_get_statistics();

    ASSERT_NOT_NULL(stats);
    ASSERT_EQ(0, stats->read_count);
    ASSERT_EQ(0, stats->write_count);
    ASSERT_EQ(0, stats->error_count);
}

/* Test: RF information retrieval */
TEST(mock_get_rf_info) {
    mock_reset_state();
    mock_set_rf_amplitude(3, 2.5);
    mock_set_rf_phase(3, 45.0);

    void* rf_info = GetRfInfo();
    ASSERT_NOT_NULL(rf_info);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->read_count);
}

/* Test: BPM position retrieval */
TEST(mock_get_position) {
    mock_reset_state();
    mock_set_bpm_position(1.0, 2.0, 3.0, 4.0);

    void* pos = GetxyPosition();
    ASSERT_NOT_NULL(pos);

    /* Verify position data structure */
    float* pos_data = (float*)pos;
    ASSERT_FLOAT_EQ(1.0, pos_data[0], 0.1);  /* Allow for jitter simulation */
    ASSERT_FLOAT_EQ(2.0, pos_data[1], 0.1);
    ASSERT_FLOAT_EQ(3.0, pos_data[2], 0.1);
    ASSERT_FLOAT_EQ(4.0, pos_data[3], 0.1);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->read_count);
}

/* Test: BPM parameter setting */
TEST(mock_set_bpm_params) {
    mock_reset_state();

    int ret;
    ret = SetBPMk1(1.5);
    ASSERT_EQ(0, ret);

    ret = SetBPMk2(2.0);
    ASSERT_EQ(0, ret);

    ret = SetBPMk3(2.5);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(3, stats->write_count);
}

/* Test: Error injection */
TEST(mock_error_injection) {
    mock_reset_state();
    mock_enable_error_injection(1);

    /* Try to set invalid value (negative) */
    int ret = SetBPMk1(-1.0);
    ASSERT_EQ(-1, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->error_count);

    /* Disable error injection */
    mock_enable_error_injection(0);
    ret = SetBPMk1(-1.0);
    ASSERT_EQ(0, ret);
}

/* Test: History trigger */
TEST(mock_history_trigger) {
    mock_reset_state();

    /* Enable history trigger */
    int ret = SetHistoryTrigger(1);
    ASSERT_EQ(0, ret);

    /* Check data ready */
    int ready = GetHistoryDataReady();
    ASSERT_EQ(1, ready);

    MockStats* stats = mock_get_statistics();
    ASSERT_TRUE(stats->trigger_count > 0);
}

/* Test: Waveform data retrieval */
TEST(mock_get_waveform) {
    mock_reset_state();

    /* Set some RF parameters */
    mock_set_rf_amplitude(3, 2.0);
    mock_set_rf_amplitude(4, 2.5);

    /* Get trigger data */
    void* trigger_data = GetTriggerAllData();
    ASSERT_NOT_NULL(trigger_data);

    MockStats* stats = mock_get_statistics();
    ASSERT_TRUE(stats->read_count > 0);
}

/* Test: Digital I/O */
TEST(mock_digital_io) {
    mock_reset_state();

    /* Set digital output */
    int ret = SetDO(5, 1);
    ASSERT_EQ(0, ret);

    /* Get digital input */
    int value = GetDI(5);
    ASSERT_TRUE(value >= 0);

    /* Test invalid channel */
    ret = SetDO(-1, 1);
    ASSERT_EQ(-1, ret);

    ret = SetDO(32, 1);
    ASSERT_EQ(-1, ret);
}

/* Test: LED control */
TEST(mock_led_control) {
    mock_reset_state();

    int ret = SetArmLedEnable(1);
    ASSERT_EQ(0, ret);

    ret = SetFanLedStatus(2);
    ASSERT_EQ(0, ret);

    int led0 = GetFPGA_LED0_RBK();
    ASSERT_TRUE(led0 >= 0);

    int led1 = GetFPGA_LED1_RBK();
    ASSERT_TRUE(led1 >= 0);
}

/* Test: ADC clock state */
TEST(mock_adc_clock) {
    mock_reset_state();

    int state = GetADclkState();
    ASSERT_EQ(1, state);  /* Should be OK in mock */
}

/* Test: White Rabbit timestamp */
TEST(mock_timestamp) {
    mock_reset_state();
    mock_set_timestamp(1234567890, 500000000);

    void* ts_data = GetTimestampData();
    ASSERT_NOT_NULL(ts_data);

    /* Save first timestamp values */
    uint32_t* ts1_ptr = (uint32_t*)ts_data;
    uint32_t sec1 = ts1_ptr[0];
    uint32_t nsec1 = ts1_ptr[1];

    /* Get second timestamp */
    void* ts_data2 = GetTimestampData();
    ASSERT_NOT_NULL(ts_data2);

    /* Read second timestamp values */
    uint32_t* ts2_ptr = (uint32_t*)ts_data2;
    uint32_t sec2 = ts2_ptr[0];
    uint32_t nsec2 = ts2_ptr[1];

    /* At least nanoseconds should have incremented */
    ASSERT_TRUE(nsec2 > nsec1 || sec2 > sec1);
}

/* Test: White Rabbit status */
TEST(mock_wr_status) {
    mock_reset_state();

    int status = GetWRStatus();
    ASSERT_EQ(1, status);  /* Always OK in mock */

    int ret = SetWRCaputureDataTrigger(1);
    ASSERT_EQ(0, ret);
}

/* Test: BPM phase value */
TEST(mock_bpm_phase) {
    mock_reset_state();

    void* phase_data = GetBPMPhaseValue();
    ASSERT_NOT_NULL(phase_data);

    MockStats* stats = mock_get_statistics();
    ASSERT_TRUE(stats->read_count > 0);
}

/* Test: XY limits setting */
TEST(mock_xy_limits) {
    mock_reset_state();

    int ret = SetBPMxyLimits(-10.0, 10.0, -10.0, 10.0);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->write_count);
}

/* Test: XY offset setting */
TEST(mock_xy_offset) {
    mock_reset_state();

    int ret = SetBPMxyOffset(0.5, -0.3);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->write_count);
}

/* Test: Kxy setting */
TEST(mock_kxy) {
    mock_reset_state();

    int ret = SetBPMkxy(1.2, 0.8);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->write_count);
}

/* Test: Phase offset setting */
TEST(mock_phase_offset) {
    mock_reset_state();

    int ret = SetBPMPhaseOffset(30.0);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->write_count);
}

/* Test: Trigger ratio setting */
TEST(mock_trigger_ratio) {
    mock_reset_state();

    int ret = SetTriggerExtractDataRatio(10);
    ASSERT_EQ(0, ret);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(1, stats->write_count);
}

/* Test: History channel data */
TEST(mock_history_channel_data) {
    mock_reset_state();

    /* Get valid channel */
    void* data = GetHistoryChannelData(0);
    ASSERT_NOT_NULL(data);

    /* Get invalid channel */
    void* data_invalid = GetHistoryChannelData(-1);
    ASSERT_NULL(data_invalid);

    data_invalid = GetHistoryChannelData(8);
    ASSERT_NULL(data_invalid);
}

/* Test: Statistics tracking */
TEST(mock_statistics) {
    mock_reset_state();

    /* Perform various operations */
    GetRfInfo();
    GetxyPosition();
    SetBPMk1(1.0);
    SetBPMk2(2.0);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(2, stats->read_count);
    ASSERT_EQ(2, stats->write_count);
    ASSERT_EQ(0, stats->error_count);
}

/* Test: Multiple RF channels */
TEST(mock_multiple_rf_channels) {
    mock_reset_state();

    /* Set amplitudes for different channels */
    int ch;
    for (ch = 3; ch <= 10; ch++) {
        mock_set_rf_amplitude(ch, 1.0 + ch * 0.5);
        mock_set_rf_phase(ch, ch * 15.0);
    }

    /* Get RF info */
    void* rf_info = GetRfInfo();
    ASSERT_NOT_NULL(rf_info);

    /* Verify we can call multiple times */
    rf_info = GetRfInfo();
    ASSERT_NOT_NULL(rf_info);

    MockStats* stats = mock_get_statistics();
    ASSERT_EQ(2, stats->read_count);
}

/* Main test runner */
int main(void) {
    TEST_SUITE_START("Mock Driver Tests");

    RUN_TEST(mock_init);
    RUN_TEST(mock_get_rf_info);
    RUN_TEST(mock_get_position);
    RUN_TEST(mock_set_bpm_params);
    RUN_TEST(mock_error_injection);
    RUN_TEST(mock_history_trigger);
    RUN_TEST(mock_get_waveform);
    RUN_TEST(mock_digital_io);
    RUN_TEST(mock_led_control);
    RUN_TEST(mock_adc_clock);
    RUN_TEST(mock_timestamp);
    RUN_TEST(mock_wr_status);
    RUN_TEST(mock_bpm_phase);
    RUN_TEST(mock_xy_limits);
    RUN_TEST(mock_xy_offset);
    RUN_TEST(mock_kxy);
    RUN_TEST(mock_phase_offset);
    RUN_TEST(mock_trigger_ratio);
    RUN_TEST(mock_history_channel_data);
    RUN_TEST(mock_statistics);
    RUN_TEST(mock_multiple_rf_channels);

    TEST_SUITE_END();

    return TEST_RETURN_CODE();
}
