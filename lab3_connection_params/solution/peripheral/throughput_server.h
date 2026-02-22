/*******************************************************************************
 * File Name: throughput_server.h
 *
 * Description: Header for the Bluetooth LE Throughput Peripheral GATT server.
 *
 *******************************************************************************
 * Copyright 2024, Novel Bits, LLC. Based on Infineon Technologies AG examples.
 ******************************************************************************/

#ifndef THROUGHPUT_SERVER_H
#define THROUGHPUT_SERVER_H

#include "wiced_bt_stack.h"
#include "cybsp.h"
#include <FreeRTOS.h>
#include <task.h>
#include "app_bt_utils.h"
#include "wiced_bt_ble.h"
#include "app_config.h"

/*******************************************************************************
 *        Macro Definitions
 ******************************************************************************/
#define CONN_INTERVAL_MULTIPLIER        (1.25f)
#define FREQUENCY                       (10000)
#define TIMER_INTERRUPT_PRIORITY        (3)

/* Data packet sizes when 247 <= ATT MTU <= 498 */
#define DATA_PACKET_SIZE_1              (244u)
#define DATA_PACKET_SIZE_2              (495u)
#define ATT_HEADER                      (3u)

/* This enumeration combines the advertising, connection states from two different
 * callbacks to maintain the status in a single state variable */
typedef enum
{
    APP_BT_ADV_OFF_CONN_OFF,
    APP_BT_ADV_ON_CONN_OFF,
    APP_BT_ADV_OFF_CONN_ON
} app_bt_adv_conn_mode_t;

/*******************************************************************************
 *        Structures
 ******************************************************************************/
typedef struct
{
    wiced_bt_device_address_t             remote_addr;   /* remote peer device address */
    uint16_t                              conn_id;       /* connection ID referenced by the stack */
    uint16_t                              mtu;           /* MTU exchanged after connection */
    double                                conn_interval; /* connection interval negotiated */
    wiced_bt_ble_host_phy_preferences_t   rx_phy;        /* RX PHY selected */
    wiced_bt_ble_host_phy_preferences_t   tx_phy;        /* TX PHY selected */
} conn_state_info_t;

extern TaskHandle_t get_throughput_task_handle;
extern TaskHandle_t notif_send_task_handle;

/*******************************************************************************
 *        Function Prototypes
 ******************************************************************************/

/* Callback function for Bluetooth stack management type events */
wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                    wiced_bt_management_evt_data_t *p_event_data);
void get_throughput_task(void *pvParam);
void send_notification_task(void *pvParam);

#endif /* THROUGHPUT_SERVER_H */
