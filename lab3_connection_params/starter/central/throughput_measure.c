/*******************************************************************************
 * File: throughput_measure.c
 *
 * Description: Simple byte counter for throughput measurement.
 *              Accumulates bytes received and calculates kbps over 1-second
 *              intervals.
 ******************************************************************************/

#include "throughput_measure.h"

static volatile uint32_t total_bytes = 0;

void throughput_measure_init(void)
{
    total_bytes = 0;
}

void throughput_measure_add_bytes(uint32_t bytes)
{
    total_bytes += bytes;
}

uint32_t throughput_measure_get_kbps(void)
{
    uint32_t bytes = total_bytes;
    total_bytes = 0;
    return (bytes * 8) / 1000;
}
