#ifndef THROUGHPUT_CENTRAL_H
#define THROUGHPUT_CENTRAL_H

#include "wiced_bt_stack.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "app_config.h"
#include <FreeRTOS.h>
#include <task.h>

/* Connection state information */
typedef struct {
    wiced_bt_device_address_t   remote_addr;
    uint16_t                    conn_id;
    uint16_t                    mtu;
    double                      conn_interval;
    uint8_t                     rx_phy;
    uint8_t                     tx_phy;
    uint16_t                    dle_tx_bytes;
    int8_t                      rssi;
    wiced_bool_t                connected;
} conn_state_info_t;

/* Discovered GATT handles */
typedef struct {
    uint16_t    service_start;
    uint16_t    service_end;
    uint16_t    notify_char_handle;
    uint16_t    notify_cccd_handle;
    uint16_t    writeme_char_handle;
} gatt_handles_t;

extern conn_state_info_t conn_state;
extern gatt_handles_t gatt_handles;
extern TaskHandle_t throughput_task_handle;

wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                                  wiced_bt_management_evt_data_t *p_event_data);
void throughput_calc_task(void *pvParam);

#endif /* THROUGHPUT_CENTRAL_H */
