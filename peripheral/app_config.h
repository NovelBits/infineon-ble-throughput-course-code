#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*******************************************************************************
 * Bluetooth LE Throughput Course - Peripheral Configuration
 *
 * Students: Change parameters below as directed in each lab exercise.
 * After changing, rebuild and reprogram the peripheral board.
 ******************************************************************************/

/* ============================================================================
 * Lab 1: Setup — Device Identity
 * ============================================================================ */
#define APP_DEVICE_NAME             "TPUT"
#define APP_DEVICE_NAME_LEN         (4)

/* Debug output (1 = enable printf, 0 = disable) */
#define APP_DEBUG_ENABLED           (1)

/* ============================================================================
 * Feature Gates — Enable features progressively through labs
 * Students: Set each to (1) when directed by the lab exercise.
 * ============================================================================ */

/* Lab 2: Set to 1 to enable PHY update request after connection */
#define APP_ENABLE_PHY_UPDATE           (0)

/* Lab 3: Set to 1 to enable CI optimization after PHY update */
#define APP_ENABLE_CI_OPTIMIZATION      (0)

/* Lab 4: Set to 1 to enable MTU exchange (uses APP_MTU_SIZE value) */
#define APP_ENABLE_MTU_EXCHANGE         (0)

/* ============================================================================
 * Lab 2: PHY Selection
 * Change APP_SELECTED_PHY to test different Physical Layer modes.
 * Options: 1 = 1M PHY, 2 = 2M PHY
 * ============================================================================ */
#define APP_PHY_1M                  (1)
#define APP_PHY_2M                  (2)

#define APP_SELECTED_PHY            APP_PHY_1M

/* ============================================================================
 * Lab 3: Link Layer Parameters
 * Connection interval in units of 1.25ms (e.g., 24 = 30ms, 6 = 7.5ms)
 * DLE: Data Length Extension max TX bytes (27-251)
 * ============================================================================ */
#define APP_CONN_INTERVAL_MIN       (24)    /* 30.0 ms */
#define APP_CONN_INTERVAL_MAX       (24)    /* 30.0 ms */
#define APP_CONN_LATENCY            (0)
#define APP_SUPERVISION_TIMEOUT     (1000)  /* 10 seconds (in 10ms units) */

/* Lab 3: Target connection interval when CI optimization is enabled */
#define APP_CI_TARGET_MIN           (6)     /* 7.5 ms */
#define APP_CI_TARGET_MAX           (6)     /* 7.5 ms */

#define APP_DLE_MAX_TX_BYTES        (27)
#define APP_DLE_MAX_TX_TIME         (0x848) /* Max TX time for 251 bytes at 1M */

/* ============================================================================
 * Lab 4: ATT/GATT Parameters
 * MTU size for data exchange (23-247)
 * ============================================================================ */
#define APP_MTU_SIZE                (247)

/* ============================================================================
 * Lab 6: Measurement Parameters
 * ============================================================================ */
#define APP_THROUGHPUT_CALC_INTERVAL_MS  (1000)  /* Calculate throughput every N ms */

/* ============================================================================
 * Notification data size constants (derived from MTU)
 * Do not modify these directly — change APP_MTU_SIZE above instead.
 * ============================================================================ */
#define APP_ATT_HEADER_SIZE         (3)
#define APP_NOTIFY_DATA_SIZE        (244)   /* Optimal for 251-byte LL payload */

/* ============================================================================
 * GATT Service UUIDs (must match central)
 * Do not modify these.
 * ============================================================================ */
#define APP_THROUGHPUT_SERVICE_UUID  { 0xCC, 0x7B, 0xCB, 0x32, 0x07, 0x08, 0x17, 0xAF, \
                                      0xD3, 0x43, 0x1E, 0x5D, 0x20, 0x0D, 0xEC, 0x1A }

#define APP_NOTIFY_CHAR_UUID        { 0x1E, 0x25, 0x21, 0x59, 0x67, 0x84, 0x78, 0x9E, \
                                      0x30, 0x4D, 0xE9, 0x91, 0x81, 0x13, 0xB0, 0xF7 }

#define APP_WRITEME_CHAR_UUID       { 0xC7, 0x58, 0xCF, 0x70, 0xB3, 0xAF, 0xE4, 0xAD, \
                                      0x65, 0x44, 0xA3, 0x85, 0x26, 0x7B, 0x70, 0xD4 }

#endif /* APP_CONFIG_H */
