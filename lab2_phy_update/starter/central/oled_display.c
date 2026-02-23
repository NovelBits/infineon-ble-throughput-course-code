/*******************************************************************************
 * File: oled_display.c
 *
 * Description: High-level OLED display page manager for the throughput central.
 *              Page 1: Main throughput view (large kbps + key params)
 *              Page 2: Detailed connection parameters
 *              Idle:   "SCANNING..." or connection status
 ******************************************************************************/

#include "oled_display.h"
#include "user_oled.h"
#include "user_btn.h"
#include "throughput_central.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Static Variables
 ******************************************************************************/
static oled_page_t current_page = OLED_PAGE_THROUGHPUT;
static oled_page_t drawn_page = OLED_PAGE_COUNT; /* Sentinel: forces full draw on first update */
static uint32_t last_throughput_kbps = 0;

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void draw_page_throughput(void);
static void draw_page_details(void);
static void draw_page_idle(void);
static void button_handler(void);
static const char* phy_to_string(uint8_t phy);

/*******************************************************************************
 * Function Name: oled_display_init
 *
 * Summary:
 *   Initialize the display page manager, configure the user button for page
 *   cycling, and draw the initial idle screen.
 ******************************************************************************/
void oled_display_init(void)
{
    current_page = OLED_PAGE_THROUGHPUT;
    last_throughput_kbps = 0;

    /* Configure button to cycle display pages */
    configure_user_btn(button_handler);

    /* Draw initial idle/scanning screen */
    draw_page_idle();
}

/*******************************************************************************
 * Function Name: oled_display_update
 *
 * Summary:
 *   Redraws the current display page with the latest connection state data.
 *   Called from throughput_calc_task and connection event handlers.
 ******************************************************************************/
void oled_display_update(void)
{
    if (!conn_state.connected)
    {
        draw_page_idle();
        return;
    }

    switch (current_page)
    {
    case OLED_PAGE_THROUGHPUT:
        draw_page_throughput();
        break;
    case OLED_PAGE_DETAILS:
        draw_page_details();
        break;
    default:
        draw_page_throughput();
        break;
    }
}

/*******************************************************************************
 * Function Name: oled_display_force_redraw
 *
 * Summary:
 *   Forces a full redraw on the next update (used after disconnect/reconnect).
 ******************************************************************************/
void oled_display_force_redraw(void)
{
    drawn_page = OLED_PAGE_COUNT;
}

/*******************************************************************************
 * Function Name: oled_display_set_throughput
 *
 * Summary:
 *   Stores the latest throughput value for display rendering.
 ******************************************************************************/
void oled_display_set_throughput(uint32_t kbps)
{
    last_throughput_kbps = kbps;
}

/*******************************************************************************
 * Function Name: oled_display_next_page
 *
 * Summary:
 *   Cycles to the next display page and redraws.
 ******************************************************************************/
void oled_display_next_page(void)
{
    current_page = (oled_page_t)((current_page + 1) % OLED_PAGE_COUNT);
    drawn_page = OLED_PAGE_COUNT; /* Force full redraw on page switch */
    oled_display_update();
}

/*******************************************************************************
 * Page 1: Main Throughput View
 *
 * Layout (128x64, 8 lines of 5x8 text, lines 2-3 use 12x16 large text):
 *   Line 0: "BLE THROUGHPUT COURSE"
 *   Line 1: "---------------------"
 *   Lines 2-3: Large throughput number (e.g. "1234")
 *   Line 4: "kbps"
 *   Line 5: "PHY: 2M    CI: 30ms"
 *   Line 6: "MTU: 247   DLE: 251"
 *   Line 7: "RSSI: -42 dBm"
 ******************************************************************************/
static void draw_page_throughput(void)
{
    char buf[22];

    /* Draw static elements only on page switch */
    if (drawn_page != OLED_PAGE_THROUGHPUT)
    {
        oled_clear_screen();
        oled_printText(0, 0, "BLE THROUGHPUT COURSE");
        oled_printText(1, 0, "---------------------");
        drawn_page = OLED_PAGE_THROUGHPUT;
    }

    /* Lines 2-3: Large throughput number (variable width, clear before redraw) */
    oled_clearLine(2);
    oled_clearLine(3);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)last_throughput_kbps);
    uint8_t num_len = strlen(buf);
    /* Total width: large digits + 2px gap + small "kbps" (4 chars * 6px) */
    uint8_t total_width = num_len * 13 + 2 + 4 * 6;
    uint8_t x_pos = (128 - total_width) / 2;
    if (x_pos > 127) x_pos = 0;
    oled_printLargeText(2, x_pos, buf);
    /* "kbps" in small text on line 3 (bottom half of large text), after the number */
    oled_printText(3, x_pos + num_len * 13 + 2, "kbps");

    /* Line 5: PHY and Connection Interval (padded to overwrite previous content) */
    snprintf(buf, sizeof(buf), "PHY:%-3s CI:%.0fms  ",
             phy_to_string(conn_state.tx_phy),
             conn_state.conn_interval);
    oled_printText(5, 0, buf);

    /* Line 6: MTU and DLE */
    snprintf(buf, sizeof(buf), "MTU:%-4d DLE:%-3d ",
             conn_state.mtu, conn_state.dle_tx_bytes);
    oled_printText(6, 0, buf);

    /* Line 7: RSSI (padded for variable width) */
    snprintf(buf, sizeof(buf), "RSSI: %-4d dBm  ", conn_state.rssi);
    oled_printText(7, 0, buf);
}

/*******************************************************************************
 * Page 2: Detailed Connection Parameters
 *
 * Layout:
 *   Line 0: "CONNECTION DETAILS"
 *   Line 1: "---------------------"
 *   Line 2: "PHY:  <value>"
 *   Line 3: "CI:   <value> ms"
 *   Line 4: "MTU:  <value> bytes"
 *   Line 5: "DLE:  <value> bytes"
 *   Line 6: "Payload: <value> B"
 *   Line 7: "RSSI: <value> dBm"
 ******************************************************************************/
static void draw_page_details(void)
{
    char buf[22];
    uint16_t payload = (conn_state.mtu > APP_ATT_HEADER_SIZE)
                     ? (conn_state.mtu - APP_ATT_HEADER_SIZE) : 0;

    /* Draw static elements only on page switch */
    if (drawn_page != OLED_PAGE_DETAILS)
    {
        oled_clear_screen();
        oled_printText(0, 0, "CONNECTION DETAILS");
        oled_printText(1, 0, "---------------------");
        drawn_page = OLED_PAGE_DETAILS;
    }

    /* All parameter lines use padded format to overwrite previous values */
    snprintf(buf, sizeof(buf), "PHY:  %-4s", phy_to_string(conn_state.tx_phy));
    oled_printText(2, 0, buf);

    snprintf(buf, sizeof(buf), "CI:   %-8.2f ms", conn_state.conn_interval);
    oled_printText(3, 0, buf);

    snprintf(buf, sizeof(buf), "MTU:  %-3d bytes ", conn_state.mtu);
    oled_printText(4, 0, buf);

    snprintf(buf, sizeof(buf), "DLE:  %-3d bytes ", conn_state.dle_tx_bytes);
    oled_printText(5, 0, buf);

    snprintf(buf, sizeof(buf), "Payload: %-3d B  ", payload);
    oled_printText(6, 0, buf);

    snprintf(buf, sizeof(buf), "RSSI: %-4d dBm  ", conn_state.rssi);
    oled_printText(7, 0, buf);
}

/*******************************************************************************
 * Idle/Scanning State
 *
 * Shows large "SCANNING" text when disconnected.
 ******************************************************************************/
static void draw_page_idle(void)
{
    /* Only redraw if not already showing idle screen */
    if (drawn_page != OLED_PAGE_COUNT)
    {
        oled_clear_screen();
        oled_printText(0, 0, "BLE Throughput Demo");
        oled_printText(1, 0, "---------------------");
        oled_printText(3, 18, "SCANNING...");
        oled_printText(5, 6, "Waiting for");
        oled_printText(6, 6, "peripheral");
        drawn_page = OLED_PAGE_COUNT; /* Use OLED_PAGE_COUNT as idle sentinel */
    }
}

/*******************************************************************************
 * Button Handler
 *
 * Called from user_btn task context when button is pressed.
 * Cycles the OLED display page.
 ******************************************************************************/
static void button_handler(void)
{
    oled_display_next_page();
}

/*******************************************************************************
 * PHY to string helper
 ******************************************************************************/
static const char* phy_to_string(uint8_t phy)
{
    switch (phy)
    {
    case 1: return "1M";
    case 2: return "2M";
    case 3: return "S2";
    case 4: return "S8";
    default: return "--";
    }
}
