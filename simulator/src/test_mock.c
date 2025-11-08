/**
 * test_mock.c - Mock Hardware Library Test Program
 *
 * 测试Mock库的基本功能
 */

#include "mockHardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PASS "\033[32m✓\033[0m"
#define FAIL "\033[31m✗\033[0m"

int test_count = 0;
int pass_count = 0;
int fail_count = 0;

void test_assert(int condition, const char *test_name)
{
    test_count++;
    if (condition) {
        printf("%s Test %d: %s\n", PASS, test_count, test_name);
        pass_count++;
    } else {
        printf("%s Test %d: %s\n", FAIL, test_count, test_name);
        fail_count++;
    }
}

int main(void)
{
    printf("==============================================\n");
    printf("Mock Hardware Library Test\n");
    printf("==============================================\n\n");

    // Test 1: SystemInit
    printf("Testing SystemInit...\n");
    int ret = SystemInit();
    test_assert(ret == 0, "SystemInit returns 0");

    // Test 2: GetBoardStatus
    printf("\nTesting GetBoardStatus...\n");
    int status = GetBoardStatus();
    test_assert(status == 1, "Board status is initialized");

    // Test 3: GetFirmwareVersion
    printf("\nTesting GetFirmwareVersion...\n");
    char version[64];
    ret = GetFirmwareVersion(version, sizeof(version));
    test_assert(ret == 0, "GetFirmwareVersion returns 0");
    printf("  Firmware version: %s\n", version);

    // Test 4: GetRfInfo
    printf("\nTesting GetRfInfo...\n");
    float Amp[8], Phase[8], Power[8];
    float VBPM[8], IBPM[8], VCRI[8], ICRI[8];
    int PSET[8];

    ret = GetRfInfo(Amp, Phase, Power, VBPM, IBPM, VCRI, ICRI, PSET);
    test_assert(ret == 0, "GetRfInfo returns 0");

    printf("  Channel 0:\n");
    printf("    Amplitude: %.3f V\n", Amp[0]);
    printf("    Phase:     %.1f deg\n", Phase[0]);
    printf("    Power:     %.1f W\n", Power[0]);

    test_assert(Amp[0] > 0.0 && Amp[0] < 10.0, "Amplitude in valid range");
    test_assert(Phase[0] >= -180.0 && Phase[0] <= 180.0, "Phase in valid range");
    test_assert(Power[0] > 0.0, "Power is positive");

    // Test 5: Multiple GetRfInfo calls (check data changes)
    printf("\nTesting data variation...\n");
    float Amp2[8];
    sleep(1);  // Wait for simulation time to advance
    GetRfInfo(Amp2, Phase, Power, VBPM, IBPM, VCRI, ICRI, PSET);

    // In sine mode, data should change
    int data_changed = (Amp[0] != Amp2[0]);
    test_assert(data_changed, "Data changes over time");

    if (data_changed) {
        printf("  Amp[0]: %.3f -> %.3f (delta: %.3f)\n",
               Amp[0], Amp2[0], Amp2[0] - Amp[0]);
    }

    // Test 6: GetTrigWaveform
    printf("\nTesting GetTrigWaveform...\n");
    float waveform[10000];
    int npts = 0;

    ret = GetTrigWaveform(0, waveform, &npts);
    test_assert(ret == 0, "GetTrigWaveform returns 0");
    test_assert(npts > 0, "Waveform has points");
    printf("  Waveform points: %d\n", npts);

    if (npts > 0) {
        printf("  First 5 points: %.3f, %.3f, %.3f, %.3f, %.3f\n",
               waveform[0], waveform[1], waveform[2], waveform[3], waveform[4]);
    }

    // Test 7: SetPhaseOffset
    printf("\nTesting SetPhaseOffset...\n");
    ret = SetPhaseOffset(0, 45.5);
    test_assert(ret == 0, "SetPhaseOffset returns 0");

    // Test 8: SetAmpOffset
    printf("\nTesting SetAmpOffset...\n");
    ret = SetAmpOffset(0, 0.5);
    test_assert(ret == 0, "SetAmpOffset returns 0");

    // Test 9: GetTemperature
    printf("\nTesting GetTemperature...\n");
    float temp;
    ret = GetTemperature(0, &temp);
    test_assert(ret == 0, "GetTemperature returns 0");
    printf("  Sensor 0 temperature: %.1f C\n", temp);
    test_assert(temp > 20.0 && temp < 60.0, "Temperature in reasonable range");

    // Test 10: GetPowerVoltage
    printf("\nTesting GetPowerVoltage...\n");
    float voltage;
    ret = GetPowerVoltage(0, &voltage);  // +5V rail
    test_assert(ret == 0, "GetPowerVoltage returns 0");
    printf("  +5V rail: %.2f V\n", voltage);
    test_assert(voltage > 4.5 && voltage < 5.5, "Voltage in valid range");

    // Test 11: SelfTest
    printf("\nTesting SelfTest...\n");
    ret = SelfTest();
    test_assert(ret == 0, "SelfTest passes");

    // Test 12: Error handling (invalid channel)
    printf("\nTesting error handling...\n");
    ret = GetChannelAmplitude(99, &Amp[0]);  // Invalid channel
    test_assert(ret == -1, "Invalid channel returns error");

    // Test 13: Mock statistics
    printf("\nTesting Mock_GetStatistics...\n");
    char stats[512];
    ret = Mock_GetStatistics(stats, sizeof(stats));
    test_assert(ret == 0, "Mock_GetStatistics returns 0");
    printf("%s\n", stats);

    // Summary
    printf("\n==============================================\n");
    printf("Test Summary\n");
    printf("==============================================\n");
    printf("Total:  %d tests\n", test_count);
    printf("Passed: %d tests (%s)\n", pass_count, pass_count == test_count ? PASS : "");
    printf("Failed: %d tests (%s)\n", fail_count, fail_count == 0 ? PASS : FAIL);
    printf("==============================================\n");

    if (fail_count == 0) {
        printf("\n%s All tests passed!\n\n", PASS);
        return 0;
    } else {
        printf("\n%s Some tests failed!\n\n", FAIL);
        return 1;
    }
}
