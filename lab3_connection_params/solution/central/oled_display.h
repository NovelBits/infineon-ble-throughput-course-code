/*******************************************************************************
 * File: oled_display.h
 *
 * Description: High-level OLED display page manager for the throughput central.
 *              Manages two display pages (main throughput + detailed params)
 *              and an idle/scanning state. Button press cycles pages.
 ******************************************************************************/

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>

/* Display pages */
typedef enum {
    OLED_PAGE_THROUGHPUT = 0,   /* Main throughput view with large kbps */
    OLED_PAGE_DETAILS,          /* Detailed connection parameters */
    OLED_PAGE_COUNT
} oled_page_t;

/* Initialize the display manager (call after oled_init + oled_clear_screen) */
void oled_display_init(void);

/* Update the current display page with latest data */
void oled_display_update(void);

/* Set the current throughput value (called from throughput_calc_task) */
void oled_display_set_throughput(uint32_t kbps);

/* Cycle to the next display page (called from button handler) */
void oled_display_next_page(void);

/* Force a full redraw on the next update (e.g. after reconnect) */
void oled_display_force_redraw(void);

#endif /* OLED_DISPLAY_H */
