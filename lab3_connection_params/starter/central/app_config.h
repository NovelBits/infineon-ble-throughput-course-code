#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*******************************************************************************
 * Bluetooth LE Throughput Course - Central Configuration
 *
 * Students: Change parameters below as directed in each lab exercise.
 * After changing, rebuild and reprogram the central board.
 ******************************************************************************/

/* ============================================================================
 * Lab 1: Setup — Device Identity
 * ============================================================================ */
#define APP_PERIPHERAL_NAME         "TPUT"
#define APP_DEVICE_NAME             "TPUT_C"
#define APP_DEVICE_NAME_LEN         (6)

/* Debug output (1 = enable printf, 0 = disable) */
#define APP_DEBUG_ENABLED           (1)

/* ============================================================================
 * Feature Gates — Enable features progressively through labs
 * Students: Set each to (1) when directed by the lab exercise.
 * ============================================================================ */

/* Lab 2: Set to 1 to enable PHY update request after connection */
#define APP_ENABLE_PHY_UPDATE           (1)

/* Lab 3: Set to 1 to enable connection parameter update after PHY update */
/* TODO: Change to (1) to enable connection parameter update */
#define APP_ENABLE_CONN_PARAM_UPDATE    (0)

/* Lab 4: Set to 1 to enable MTU exchange (uses APP_MTU_SIZE value) */
#define APP_ENABLE_MTU_EXCHANGE         (0)

/* ============================================================================
 * Lab 2: PHY Selection
 * ============================================================================ */
#define APP_PHY_1M                  (1)
#define APP_PHY_2M                  (2)

#define APP_SELECTED_PHY            APP_PHY_2M

/* ============================================================================
 * Lab 3: Link Layer Parameters
 * ============================================================================ */
/* TODO: Change to (6) for 7.5 ms connection interval */
#define APP_CONN_INTERVAL_MIN       (24)
/* TODO: Change to (6) for 7.5 ms connection interval */
#define APP_CONN_INTERVAL_MAX       (24)
#define APP_CONN_LATENCY            (0)
#define APP_SUPERVISION_TIMEOUT     (1000)

/* TODO: Change to (251) for maximum DLE */
#define APP_DLE_MAX_TX_BYTES        (27)
#define APP_DLE_MAX_TX_TIME         (0x848)

/* Scan parameters */
#define APP_SCAN_INTERVAL           (96)    /* 60ms in 0.625ms units */
#define APP_SCAN_WINDOW             (48)    /* 30ms in 0.625ms units */

/* ============================================================================
 * Lab 4: ATT/GATT Parameters
 * ============================================================================ */
#define APP_MTU_SIZE                (247)

/* ============================================================================
 * Lab 6: Measurement Parameters
 * ============================================================================ */
#define APP_THROUGHPUT_CALC_INTERVAL_MS  (1000)

/* ============================================================================
 * Notification data size constants
 * ============================================================================ */
#define APP_ATT_HEADER_SIZE         (3)
#define APP_NOTIFY_DATA_SIZE        (244)

/* ============================================================================
 * GATT Service UUIDs (must match peripheral)
 * ============================================================================ */
#define APP_THROUGHPUT_SERVICE_UUID  { 0xCC, 0x7B, 0xCB, 0x32, 0x07, 0x08, 0x17, 0xAF, \
                                      0xD3, 0x43, 0x1E, 0x5D, 0x20, 0x0D, 0xEC, 0x1A }

#define APP_NOTIFY_CHAR_UUID        { 0x1E, 0x25, 0x21, 0x59, 0x67, 0x84, 0x78, 0x9E, \
                                      0x30, 0x4D, 0xE9, 0x91, 0x81, 0x13, 0xB0, 0xF7 }

#define APP_WRITEME_CHAR_UUID       { 0xC7, 0x58, 0xCF, 0x70, 0xB3, 0xAF, 0xE4, 0xAD, \
                                      0x65, 0x44, 0xA3, 0x85, 0x26, 0x7B, 0x70, 0xD4 }

/* ============================================================================
 * OLED Display (controlled by USE_OLED_DISP define in Makefile)
 * ============================================================================ */
#define APP_OLED_DISPLAY_PAGES      (2)

#endif /* APP_CONFIG_H */
