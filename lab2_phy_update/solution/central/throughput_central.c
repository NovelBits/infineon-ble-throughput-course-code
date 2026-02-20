/*******************************************************************************
 * File: throughput_central.c
 *
 * Description: Bluetooth LE GATT client that scans for the throughput
 *              peripheral, connects, discovers services, enables notifications,
 *              and measures received data throughput.
 ******************************************************************************/

#include "throughput_central.h"
#include "throughput_measure.h"
#include "app_bt_utils.h"
#include "app_config.h"
#include "cyhal.h"
#include "wiced_bt_l2c.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_memory.h"
#include <string.h>
#include <stdio.h>

#ifdef USE_OLED_DISP
#include "oled_display.h"
#endif

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
conn_state_info_t conn_state = {0};
gatt_handles_t gatt_handles = {0};
TaskHandle_t throughput_task_handle = NULL;

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* UUIDs for service discovery */
static const uint8_t throughput_service_uuid[16] = APP_THROUGHPUT_SERVICE_UUID;
static const uint8_t notify_char_uuid[16] = APP_NOTIFY_CHAR_UUID;
static const uint8_t writeme_char_uuid[16] = APP_WRITEME_CHAR_UUID;

/* 1-second timer for throughput calculation */
static cyhal_timer_t throughput_timer_obj;
static const cyhal_timer_cfg_t throughput_timer_cfg = {
    .compare_value = 0,
    .period = 9999,
    .direction = CYHAL_TIMER_DIR_UP,
    .is_compare = false,
    .is_continuous = true,
    .value = 0
};

/* Discovery state machine */
typedef enum {
    DISCOVERY_STATE_IDLE,
    DISCOVERY_STATE_SERVICE,
    DISCOVERY_STATE_CHAR,
    DISCOVERY_STATE_CCCD,
    DISCOVERY_STATE_DONE
} discovery_state_t;

static discovery_state_t disc_state = DISCOVERY_STATE_IDLE;

/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void app_start_scan(void);
static void app_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
                                   uint8_t *p_adv_data);
static wiced_bt_gatt_status_t app_gatt_event_handler(wiced_bt_gatt_evt_t event,
                                                      wiced_bt_gatt_event_data_t *p_event_data);
static void app_throughput_timer_callb(void *callback_arg, cyhal_timer_event_t event);
static void app_start_service_discovery(uint16_t conn_id);
static void app_start_char_discovery(uint16_t conn_id);
static void app_start_cccd_discovery(uint16_t conn_id);
static void app_enable_notifications(uint16_t conn_id);
static void app_request_phy_update(void);
static void app_handle_discovery_result(wiced_bt_gatt_event_data_t *p_event_data);
static void app_handle_operation_complete(wiced_bt_gatt_event_data_t *p_event_data);
static void app_handle_discovery_complete(wiced_bt_gatt_event_data_t *p_event_data);

/*******************************************************************************
 * Function Name: app_bt_management_callback
 *
 * Summary:
 *   Bluetooth Management event handler. Handles stack init, PHY updates,
 *   connection parameter updates, and DLE updates.
 ******************************************************************************/
wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                                  wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_bt_dev_status_t result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t local_addr;

    printf("[BTM] Event: %s\r\n", get_bt_event_name(event));

    switch (event)
    {
    case BTM_ENABLED_EVT:
        if (p_event_data->enabled.status == WICED_BT_SUCCESS)
        {
            /* Read and print local Bluetooth address */
            wiced_bt_dev_read_local_addr(local_addr);
            printf("Local Bluetooth Address: ");
            print_bd_address(local_addr);
            printf("\r\n");

            /* Disable pairing */
            wiced_bt_set_pairable_mode(WICED_FALSE, WICED_FALSE);

            /* Register GATT event handler */
            wiced_bt_gatt_register(app_gatt_event_handler);

            /* Initialize throughput measurement */
            throughput_measure_init();

            /* Initialize 1-second throughput timer (10kHz, period 9999 = 1s) */
            cy_rslt_t rslt;
            rslt = cyhal_timer_init(&throughput_timer_obj, NC, NULL);
            if (rslt == CY_RSLT_SUCCESS)
            {
                rslt = cyhal_timer_configure(&throughput_timer_obj, &throughput_timer_cfg);
            }
            if (rslt == CY_RSLT_SUCCESS)
            {
                rslt = cyhal_timer_set_frequency(&throughput_timer_obj, 10000);
            }
            if (rslt == CY_RSLT_SUCCESS)
            {
                cyhal_timer_register_callback(&throughput_timer_obj,
                                               app_throughput_timer_callb, NULL);
                cyhal_timer_enable_event(&throughput_timer_obj,
                                          CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true);
            }

            /* Start scanning for the peripheral */
            app_start_scan();
        }
        else
        {
            printf("Bluetooth stack init failed: %d\r\n", p_event_data->enabled.status);
        }
        break;

    case BTM_BLE_PHY_UPDATE_EVT:
        conn_state.rx_phy = p_event_data->ble_phy_update_event.rx_phy;
        conn_state.tx_phy = p_event_data->ble_phy_update_event.tx_phy;
        printf("PHY updated: TX=%dM, RX=%dM\r\n", conn_state.tx_phy, conn_state.rx_phy);

#if APP_ENABLE_CONN_PARAM_UPDATE
        /* After PHY update, request connection parameter update */
        {
            wiced_bt_ble_pref_conn_params_t conn_params = {
                .conn_interval_min = APP_CONN_INTERVAL_MIN,
                .conn_interval_max = APP_CONN_INTERVAL_MAX,
                .conn_latency = APP_CONN_LATENCY,
                .conn_supervision_timeout = APP_SUPERVISION_TIMEOUT,
                .min_ce_length = 0,
                .max_ce_length = 0
            };
            wiced_bt_l2cap_update_ble_conn_params(conn_state.remote_addr, &conn_params);
        }
#endif

#if !APP_ENABLE_MTU_EXCHANGE
        /* When MTU exchange is disabled, discovery is started after PHY update */
        if (disc_state == DISCOVERY_STATE_IDLE)
        {
            disc_state = DISCOVERY_STATE_SERVICE;
            app_start_service_discovery(conn_state.conn_id);
        }
#endif

#ifdef USE_OLED_DISP
        oled_display_update();
#endif
        break;

    case BTM_BLE_CONNECTION_PARAM_UPDATE:
        conn_state.conn_interval = p_event_data->ble_connection_param_update.conn_interval * 1.25;
        printf("Connection interval updated: %.2f ms\r\n", conn_state.conn_interval);

#ifdef USE_OLED_DISP
        oled_display_update();
#endif
        break;

    case BTM_BLE_DATA_LENGTH_UPDATE_EVENT:
        conn_state.dle_tx_bytes = p_event_data->ble_data_length_update_event.max_tx_octets;
        printf("DLE updated: TX max octets=%d\r\n", conn_state.dle_tx_bytes);

#ifdef USE_OLED_DISP
        oled_display_update();
#endif
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
    case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
    case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
    case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        result = WICED_BT_SUCCESS;
        break;

    case BTM_BLE_SCAN_STATE_CHANGED_EVT:
        printf("Scan state changed: %d\r\n",
               p_event_data->ble_scan_state_changed);
        break;

    default:
        break;
    }

    return result;
}

/*******************************************************************************
 * Function Name: app_start_scan
 *
 * Summary:
 *   Starts Bluetooth LE scanning to find the throughput peripheral.
 ******************************************************************************/
static void app_start_scan(void)
{
    wiced_bt_ble_scan_type_t scan_type = BTM_BLE_SCAN_TYPE_HIGH_DUTY;

    printf("Starting scan for \"%s\"...\r\n", APP_PERIPHERAL_NAME);

    /* Set scan parameters */
    wiced_bt_ble_observe(WICED_TRUE, 0, app_scan_result_cback);
}

/*******************************************************************************
 * Function Name: app_scan_result_cback
 *
 * Summary:
 *   Scan result callback. Searches advertisement data for the target peripheral
 *   device name. When found, stops scanning and initiates a connection.
 ******************************************************************************/
static void app_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
                                   uint8_t *p_adv_data)
{
    uint8_t length = 0;
    uint8_t *p_data = NULL;

    if (p_scan_result == NULL)
    {
        /* Scan complete */
        printf("Scan complete.\r\n");
        return;
    }

    /* Search for the complete or shortened local name in advertisement data */
    p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, &length);

    if (p_data == NULL)
    {
        p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                    BTM_BLE_ADVERT_TYPE_NAME_SHORT, &length);
    }

    if (p_data == NULL)
    {
        return;
    }

    /* Compare the found name with our target peripheral name */
    if ((length == strlen(APP_PERIPHERAL_NAME)) &&
        (memcmp(p_data, APP_PERIPHERAL_NAME, length) == 0))
    {
        printf("Found target peripheral: ");
        print_bd_address(p_scan_result->remote_bd_addr);
        printf(" RSSI: %d\r\n", p_scan_result->rssi);

        /* Stop scanning */
        wiced_bt_ble_observe(WICED_FALSE, 0, NULL);

        /* Initiate connection */
        wiced_bool_t connect_status;
        connect_status = wiced_bt_gatt_le_connect(
            p_scan_result->remote_bd_addr,
            p_scan_result->ble_addr_type,
            BLE_CONN_MODE_HIGH_DUTY,
            WICED_FALSE);

        if (connect_status)
        {
            printf("Connecting...\r\n");
        }
        else
        {
            printf("Connection initiation failed. Restarting scan.\r\n");
            app_start_scan();
        }
    }
}

/*******************************************************************************
 * Function Name: app_gatt_event_handler
 *
 * Summary:
 *   GATT event handler. Manages connection, disconnection, discovery,
 *   notifications, and MTU exchange.
 ******************************************************************************/
static wiced_bt_gatt_status_t app_gatt_event_handler(wiced_bt_gatt_evt_t event,
                                                      wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        if (p_event_data->connection_status.connected)
        {
            /* Connected */
            conn_state.conn_id = p_event_data->connection_status.conn_id;
            memcpy(conn_state.remote_addr,
                   p_event_data->connection_status.bd_addr,
                   BD_ADDR_LEN);
            conn_state.connected = WICED_TRUE;
            conn_state.mtu = 23; /* Default MTU before exchange */

            printf("Connected! conn_id=%d, peer: ", conn_state.conn_id);
            print_bd_address(conn_state.remote_addr);
            printf("\r\n");

            /* Reset throughput measurement */
            throughput_measure_init();

            /* Start the 1-second throughput timer */
            cyhal_timer_start(&throughput_timer_obj);

#if APP_ENABLE_MTU_EXCHANGE
            /* Request MTU exchange */
            wiced_bt_gatt_client_configure_mtu(conn_state.conn_id, APP_MTU_SIZE);
            printf("Requested MTU: %d\r\n", APP_MTU_SIZE);
#else
            /* No MTU exchange — use default 23. Start discovery directly. */
            printf("Using default MTU: 23\r\n");
            conn_state.mtu = 23;
#if APP_ENABLE_PHY_UPDATE
            app_request_phy_update();
#else
            disc_state = DISCOVERY_STATE_SERVICE;
            app_start_service_discovery(conn_state.conn_id);
#endif
#endif

#ifdef USE_OLED_DISP
            oled_display_update();
#endif
        }
        else
        {
            /* Disconnected */
            printf("Disconnected. Reason: %s\r\n",
                   get_bt_gatt_disconn_reason_name(
                       p_event_data->connection_status.reason));

            /* Reset state */
            conn_state.conn_id = 0;
            conn_state.connected = WICED_FALSE;
            conn_state.mtu = 0;
            conn_state.conn_interval = 0;
            conn_state.rx_phy = 0;
            conn_state.tx_phy = 0;
            conn_state.dle_tx_bytes = 0;
            conn_state.rssi = 0;
            memset(conn_state.remote_addr, 0, BD_ADDR_LEN);
            memset(&gatt_handles, 0, sizeof(gatt_handles));
            disc_state = DISCOVERY_STATE_IDLE;

            /* Stop throughput timer */
            cyhal_timer_stop(&throughput_timer_obj);

            /* Reset throughput measurement */
            throughput_measure_init();

#ifdef USE_OLED_DISP
            oled_display_update();
#endif

            /* Restart scanning */
            printf("Restarting scan...\r\n");
            app_start_scan();
        }
        break;

    case GATT_DISCOVERY_RESULT_EVT:
        app_handle_discovery_result(p_event_data);
        break;

    case GATT_DISCOVERY_CPLT_EVT:
        app_handle_discovery_complete(p_event_data);
        break;

    case GATT_OPERATION_CPLT_EVT:
        app_handle_operation_complete(p_event_data);
        break;

    case GATT_GET_RESPONSE_BUFFER_EVT:
        p_event_data->buffer_request.buffer.p_app_rsp_buffer =
            (uint8_t *)wiced_bt_get_buffer(p_event_data->buffer_request.len_requested);
        p_event_data->buffer_request.buffer.p_app_ctxt = (void *)wiced_bt_free_buffer;
        break;

    case GATT_APP_BUFFER_TRANSMITTED_EVT:
        {
            void (*pfn_free)(uint8_t *) =
                (void (*)(uint8_t *))p_event_data->buffer_xmitted.p_app_ctxt;
            if (pfn_free)
            {
                pfn_free(p_event_data->buffer_xmitted.p_app_data);
            }
        }
        break;

    default:
        break;
    }

    return status;
}

/*******************************************************************************
 * Function Name: app_handle_discovery_result
 *
 * Summary:
 *   Processes GATT discovery results: services, characteristics, and
 *   descriptors (CCCD).
 ******************************************************************************/
static void app_handle_discovery_result(wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_discovery_result_t *p_disc_result = &p_event_data->discovery_result;

    switch (p_disc_result->discovery_type)
    {
    case GATT_DISCOVER_SERVICES_BY_UUID:
        {
            wiced_bt_gatt_group_value_t *p_group = &p_disc_result->discovery_data.group_value;
            gatt_handles.service_start = p_group->s_handle;
            gatt_handles.service_end = p_group->e_handle;
            printf("Service discovered: start=0x%04X, end=0x%04X\r\n",
                   gatt_handles.service_start, gatt_handles.service_end);
        }
        break;

    case GATT_DISCOVER_CHARACTERISTICS:
        {
            wiced_bt_gatt_char_declaration_t *p_char =
                &p_disc_result->discovery_data.characteristic_declaration;

            /* Compare the 128-bit UUID to find our characteristics */
            if (memcmp(p_char->char_uuid.uu.uuid128, notify_char_uuid, 16) == 0)
            {
                gatt_handles.notify_char_handle = p_char->val_handle;
                printf("Notify characteristic found: handle=0x%04X\r\n",
                       gatt_handles.notify_char_handle);
            }
            else if (memcmp(p_char->char_uuid.uu.uuid128, writeme_char_uuid, 16) == 0)
            {
                gatt_handles.writeme_char_handle = p_char->val_handle;
                printf("WriteMe characteristic found: handle=0x%04X\r\n",
                       gatt_handles.writeme_char_handle);
            }
        }
        break;

    case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
        {
            wiced_bt_gatt_char_descr_info_t *p_descr =
                &p_disc_result->discovery_data.char_descr_info;

            /* Look for Client Characteristic Configuration Descriptor (0x2902) */
            if (p_descr->type.uu.uuid16 == 0x2902)
            {
                gatt_handles.notify_cccd_handle = p_descr->handle;
                printf("CCCD found: handle=0x%04X\r\n", gatt_handles.notify_cccd_handle);
            }
        }
        break;

    default:
        break;
    }
}

/*******************************************************************************
 * Function Name: app_handle_discovery_complete
 *
 * Summary:
 *   Called when a discovery phase completes. Advances the discovery state
 *   machine through service -> characteristic -> descriptor -> enable
 *   notifications.
 ******************************************************************************/
static void app_handle_discovery_complete(wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_discovery_complete_t *p_disc_cplt = &p_event_data->discovery_complete;

    printf("Discovery complete: type=%d, conn_id=%d, status=%d\r\n",
           p_disc_cplt->discovery_type, p_disc_cplt->conn_id,
           p_disc_cplt->status);

    switch (disc_state)
    {
    case DISCOVERY_STATE_SERVICE:
        if (gatt_handles.service_start != 0 && gatt_handles.service_end != 0)
        {
            /* Service found, now discover characteristics */
            disc_state = DISCOVERY_STATE_CHAR;
            app_start_char_discovery(conn_state.conn_id);
        }
        else
        {
            printf("Throughput service not found!\r\n");
            disc_state = DISCOVERY_STATE_IDLE;
        }
        break;

    case DISCOVERY_STATE_CHAR:
        if (gatt_handles.notify_char_handle != 0)
        {
            /* Characteristics found, now discover CCCD */
            disc_state = DISCOVERY_STATE_CCCD;
            app_start_cccd_discovery(conn_state.conn_id);
        }
        else
        {
            printf("Notify characteristic not found!\r\n");
            disc_state = DISCOVERY_STATE_IDLE;
        }
        break;

    case DISCOVERY_STATE_CCCD:
        if (gatt_handles.notify_cccd_handle != 0)
        {
            /* CCCD found, enable notifications */
            disc_state = DISCOVERY_STATE_DONE;
            app_enable_notifications(conn_state.conn_id);
        }
        else
        {
            printf("CCCD not found!\r\n");
            disc_state = DISCOVERY_STATE_IDLE;
        }
        break;

    default:
        break;
    }
}

/*******************************************************************************
 * Function Name: app_handle_operation_complete
 *
 * Summary:
 *   Handles GATT operation completion events: MTU exchange, write completion,
 *   and incoming notifications.
 ******************************************************************************/
static void app_handle_operation_complete(wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_operation_complete_t *p_op_cplt = &p_event_data->operation_complete;

    switch (p_op_cplt->op)
    {
    case GATTC_OPTYPE_CONFIG_MTU:
        conn_state.mtu = p_op_cplt->response_data.mtu;
        printf("MTU exchanged: %d\r\n", conn_state.mtu);

#if APP_ENABLE_PHY_UPDATE
        /* After MTU exchange, request PHY update */
        app_request_phy_update();
#endif

        /* DLE is configured via wiced_bt_cfg_settings.default_le_tx_data_length
         * in the stack configuration. The controller applies DLE automatically
         * after connection. See BTM_BLE_DATA_LENGTH_UPDATE_EVENT for status. */

        /* Start GATT service discovery */
        disc_state = DISCOVERY_STATE_SERVICE;
        app_start_service_discovery(conn_state.conn_id);
        break;

    case GATTC_OPTYPE_WRITE_WITH_RSP:
    case GATTC_OPTYPE_WRITE_NO_RSP:
        printf("Write complete: status=%s\r\n",
               get_bt_gatt_status_name(p_op_cplt->status));

        if (p_op_cplt->status == WICED_BT_GATT_SUCCESS)
        {
            printf("Notifications enabled! Waiting for data...\r\n");

#ifdef USE_OLED_DISP
            oled_display_update();
#endif
        }
        break;

    case GATTC_OPTYPE_NOTIFICATION:
        /* Received notification data - count bytes for throughput measurement */
        throughput_measure_add_bytes(p_op_cplt->response_data.att_value.len);
        break;

    case GATTC_OPTYPE_INDICATION:
        /* Received indication data - count bytes for throughput measurement */
        throughput_measure_add_bytes(p_op_cplt->response_data.att_value.len);
        break;

    default:
        printf("Operation complete: op=%d, status=%s\r\n",
               p_op_cplt->op,
               get_bt_gatt_status_name(p_op_cplt->status));
        break;
    }
}

/*******************************************************************************
 * Function Name: app_start_service_discovery
 *
 * Summary:
 *   Initiates GATT service discovery by 128-bit UUID.
 ******************************************************************************/
static void app_start_service_discovery(uint16_t conn_id)
{
    wiced_bt_gatt_discovery_param_t disc_param;

    memset(&disc_param, 0, sizeof(disc_param));
    disc_param.s_handle = 0x0001;
    disc_param.e_handle = 0xFFFF;
    disc_param.uuid.len = LEN_UUID_128;
    memcpy(disc_param.uuid.uu.uuid128, throughput_service_uuid, 16);

    printf("Starting service discovery...\r\n");
    wiced_bt_gatt_status_t status = wiced_bt_gatt_client_send_discover(
        conn_id, GATT_DISCOVER_SERVICES_BY_UUID, &disc_param);

    if (status != WICED_BT_GATT_SUCCESS)
    {
        printf("Service discovery failed: %s\r\n", get_bt_gatt_status_name(status));
    }
}

/*******************************************************************************
 * Function Name: app_start_char_discovery
 *
 * Summary:
 *   Discovers characteristics within the throughput service handle range.
 ******************************************************************************/
static void app_start_char_discovery(uint16_t conn_id)
{
    wiced_bt_gatt_discovery_param_t disc_param;

    memset(&disc_param, 0, sizeof(disc_param));
    disc_param.s_handle = gatt_handles.service_start;
    disc_param.e_handle = gatt_handles.service_end;

    printf("Starting characteristic discovery (0x%04X - 0x%04X)...\r\n",
           disc_param.s_handle, disc_param.e_handle);
    wiced_bt_gatt_status_t status = wiced_bt_gatt_client_send_discover(
        conn_id, GATT_DISCOVER_CHARACTERISTICS, &disc_param);

    if (status != WICED_BT_GATT_SUCCESS)
    {
        printf("Characteristic discovery failed: %s\r\n",
               get_bt_gatt_status_name(status));
    }
}

/*******************************************************************************
 * Function Name: app_start_cccd_discovery
 *
 * Summary:
 *   Discovers the Client Characteristic Configuration Descriptor for the
 *   notify characteristic.
 ******************************************************************************/
static void app_start_cccd_discovery(uint16_t conn_id)
{
    wiced_bt_gatt_discovery_param_t disc_param;

    memset(&disc_param, 0, sizeof(disc_param));
    /* CCCD is expected right after the characteristic value handle */
    disc_param.s_handle = gatt_handles.notify_char_handle + 1;
    /* Search up to the next characteristic or end of service */
    if (gatt_handles.writeme_char_handle > gatt_handles.notify_char_handle)
    {
        disc_param.e_handle = gatt_handles.writeme_char_handle - 1;
    }
    else
    {
        disc_param.e_handle = gatt_handles.service_end;
    }

    printf("Starting CCCD discovery (0x%04X - 0x%04X)...\r\n",
           disc_param.s_handle, disc_param.e_handle);
    wiced_bt_gatt_status_t status = wiced_bt_gatt_client_send_discover(
        conn_id, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, &disc_param);

    if (status != WICED_BT_GATT_SUCCESS)
    {
        printf("CCCD discovery failed: %s\r\n", get_bt_gatt_status_name(status));
    }
}

/*******************************************************************************
 * Function Name: app_enable_notifications
 *
 * Summary:
 *   Writes 0x0001 to the CCCD handle to enable notifications from the
 *   peripheral.
 ******************************************************************************/
static void app_enable_notifications(uint16_t conn_id)
{
    uint8_t cccd_val[2] = {0x01, 0x00}; /* Enable notifications */

    wiced_bt_gatt_write_hdr_t write_hdr;
    write_hdr.handle = gatt_handles.notify_cccd_handle;
    write_hdr.offset = 0;
    write_hdr.len = 2;
    write_hdr.auth_req = GATT_AUTH_REQ_NONE;

    printf("Enabling notifications (CCCD handle=0x%04X)...\r\n",
           gatt_handles.notify_cccd_handle);

    wiced_bt_gatt_status_t status = wiced_bt_gatt_client_send_write(
        conn_id, GATT_REQ_WRITE, &write_hdr, cccd_val, NULL);

    if (status != WICED_BT_GATT_SUCCESS)
    {
        printf("Enable notifications failed: %s\r\n",
               get_bt_gatt_status_name(status));
    }
}

/*******************************************************************************
 * Function Name: app_request_phy_update
 *
 * Summary:
 *   Requests a PHY update based on APP_SELECTED_PHY configuration.
 ******************************************************************************/
static void app_request_phy_update(void)
{
    wiced_bt_ble_phy_preferences_t phy_pref;

    memset(&phy_pref, 0, sizeof(phy_pref));
    memcpy(phy_pref.remote_bd_addr, conn_state.remote_addr, BD_ADDR_LEN);

    if (APP_SELECTED_PHY == APP_PHY_2M)
    {
        phy_pref.rx_phys = BTM_BLE_PREFER_2M_PHY;
        phy_pref.tx_phys = BTM_BLE_PREFER_2M_PHY;
        printf("Requesting 2M PHY...\r\n");
    }
    else
    {
        phy_pref.rx_phys = BTM_BLE_PREFER_1M_PHY;
        phy_pref.tx_phys = BTM_BLE_PREFER_1M_PHY;
        printf("Requesting 1M PHY...\r\n");
    }

    wiced_bt_ble_set_phy(&phy_pref);
}

/*******************************************************************************
 * Function Name: app_throughput_timer_callb
 *
 * Summary:
 *   Timer callback (ISR context). Notifies the throughput task to calculate
 *   and print throughput.
 ******************************************************************************/
static void app_throughput_timer_callb(void *callback_arg, cyhal_timer_event_t event)
{
    (void)callback_arg;
    (void)event;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (throughput_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(throughput_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/*******************************************************************************
 * Function Name: throughput_calc_task
 *
 * Summary:
 *   FreeRTOS task that blocks on notification from the 1-second timer.
 *   Calculates and prints throughput, reads RSSI, and updates OLED display.
 ******************************************************************************/
void throughput_calc_task(void *pvParam)
{
    (void)pvParam;

    while (1)
    {
        /* Wait for 1-second timer notification */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (conn_state.connected)
        {
            /* Calculate throughput */
            uint32_t kbps = throughput_measure_get_kbps();

            if (kbps > 0)
            {
                printf("Throughput: %lu kbps\r\n", (unsigned long)kbps);
            }

            /* Read current RSSI */
            wiced_bt_dev_read_rssi(conn_state.remote_addr,
                                    BT_TRANSPORT_LE,
                                    (wiced_bt_dev_cmpl_cback_t *)NULL);

#ifdef USE_OLED_DISP
            oled_display_set_throughput(kbps);
            oled_display_update();
#endif
        }
    }
}
