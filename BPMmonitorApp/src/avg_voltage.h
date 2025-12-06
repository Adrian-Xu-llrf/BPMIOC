#ifndef AVG_VOLTAGE_H
#define AVG_VOLTAGE_H

#ifdef __cplusplus
extern "C" {
#endif

void configure_avg_window(int start, int stop, int background_start, int background_stop);
void get_avg_window(int *start, int *stop, int *background_start, int *background_stop);
void set_avg_enabled(int enabled);
int is_avg_enabled(void);
float update_avg_voltage(int ch_id, float *data, int len);
float get_avg_voltage(int ch_id);

#ifdef __cplusplus
}
#endif

#endif // AVG_VOLTAGE_H
