/*******************************************************************************
 * File: throughput_measure.c
 *
 * Description: Byte counter for throughput measurement with running average.
 *              Accumulates bytes received and calculates kbps over 1-second
 *              intervals. Maintains a circular buffer for computing the
 *              running average over APP_THROUGHPUT_AVG_SAMPLES samples.
 ******************************************************************************/

#include "throughput_measure.h"
#include <string.h>

static volatile uint32_t total_bytes = 0;

/* Circular buffer for running average */
static uint32_t kbps_history[APP_THROUGHPUT_AVG_SAMPLES];
static uint32_t history_index = 0;
static uint32_t history_count = 0;

void throughput_measure_init(void)
{
    total_bytes = 0;
    memset(kbps_history, 0, sizeof(kbps_history));
    history_index = 0;
    history_count = 0;
}

void throughput_measure_add_bytes(uint32_t bytes)
{
    total_bytes += bytes;
}

uint32_t throughput_measure_get_kbps(void)
{
    uint32_t bytes = total_bytes;
    total_bytes = 0;
    uint32_t kbps = (bytes * 8) / 1000;

    /* Store in circular buffer */
    kbps_history[history_index] = kbps;
    history_index = (history_index + 1) % APP_THROUGHPUT_AVG_SAMPLES;
    if (history_count < APP_THROUGHPUT_AVG_SAMPLES)
    {
        history_count++;
    }

    return kbps;
}

uint32_t throughput_measure_get_avg_kbps(void)
{
    if (history_count == 0)
    {
        return 0;
    }

    uint32_t sum = 0;
    for (uint32_t i = 0; i < history_count; i++)
    {
        sum += kbps_history[i];
    }
    return sum / history_count;
}
