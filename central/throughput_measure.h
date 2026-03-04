#ifndef THROUGHPUT_MEASURE_H
#define THROUGHPUT_MEASURE_H

#include <stdint.h>
#include "app_config.h"

void throughput_measure_init(void);
void throughput_measure_add_bytes(uint32_t bytes);
uint32_t throughput_measure_get_kbps(void);
uint32_t throughput_measure_get_avg_kbps(void);

#endif /* THROUGHPUT_MEASURE_H */
