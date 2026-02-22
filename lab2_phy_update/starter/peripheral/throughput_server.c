/*******************************************************************************
 * File Name: throughput_server.c
 *
 * Description: Bluetooth LE Throughput Peripheral GATT server implementation.
 *              Adapted from Infineon's BLE Throughput Server example.
 *              - NO LED PWM (simplified for course)
 *              - NO bonding/pairing
 *              - PHY and connection parameters driven by app_config.h
 *
 *******************************************************************************
 * Copyright 2024, Novel Bits, LLC. Based on Infineon Technologies AG examples.
 ******************************************************************************/

/*******************************************************************************
 *        Header Files
 ******************************************************************************/
#include "throughput_server.h"
#include "cyhal.h"
#include <FreeRTOS.h>
#include <task.h>
#include "wiced_memory.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_l2c.h"

/*******************************************************************************
 *        Macros
 ******************************************************************************/
#define GET_THROUGHPUT_TIMER_PERIOD      (9999u)
#define APP_MILLISEC_TIMER_PERIOD       (9u)
#define NOTIFICATION_DATA_SIZE          (244)
#define STO_MULTIPLIER                  (10)
#define TASK_NOTIFY_1MS_TIMER           (1u)
#define TASK_NOTIFY_NO_GATT_CONGESTION  (2u)

/*******************************************************************************
 *        Variable Definitions
 ******************************************************************************/
/* Variables to hold GATT notification bytes sent and GATT Write bytes received */
static unsigned long gatt_notif_tx_bytes = 0u;
static unsigned long gatt_write_rx_bytes = 0u;

/* Variable to store connection parameter status */
wiced_bool_t conn_param_status = FALSE;

/* Data buffers sent as GATT notifications (alternating) */
uint8_t notification_data_seq1[NOTIFICATION_DATA_SIZE];
uint8_t notification_data_seq2[NOTIFICATION_DATA_SIZE];

/* Variable to store packet size decided based on MTU exchanged */
static uint16_t packet_size = 0u;
uint8_t data_flag = 0;
uint8_t value_initialize = 243;

/* Variable to store connection state information */
static conn_state_info_t conn_state_info;

/* Variable to store BLE advertising state */
static app_bt_adv_conn_mode_t app_bt_adv_conn_state = APP_BT_ADV_OFF_CONN_OFF;

/* For 1 sec and 1 millisecond timer */
static cyhal_timer_t get_throughput_timer_obj, app_millisec_timer_obj;

const cyhal_timer_cfg_t get_throughput_timer_cfg =
{
    .compare_value = 0,                     /* Timer compare value, not used */
    .period = GET_THROUGHPUT_TIMER_PERIOD,   /* Defines the timer period */
    .direction = CYHAL_TIMER_DIR_UP,         /* Timer counts up */
    .is_compare = false,                     /* Don't use compare mode */
    .is_continuous = true,                   /* Run timer indefinitely */
    .value = 0                               /* Initial value of counter */
};

const cyhal_timer_cfg_t app_millisec_timer_cfg =
{
    .compare_value = 0,                     /* Timer compare value, not used */
    .period = APP_MILLISEC_TIMER_PERIOD,     /* Defines the timer period */
    .direction = CYHAL_TIMER_DIR_UP,         /* Timer counts up */
    .is_compare = false,                     /* Don't use compare mode */
    .is_continuous = true,                   /* Run timer indefinitely */
    .value = 0                               /* Initial value of counter */
};

/*******************************************************************************
 *        Function Prototypes
 ******************************************************************************/
static void tput_ble_app_init(void);
static uint16_t tput_get_notification_packet_size(uint16_t att_mtu_size);
static void tput_send_notification(void);
void app_throughput_timer_callb(void *callback_arg, cyhal_timer_event_t event);
void app_millisec_timer_callb(void *callback_arg, cyhal_timer_event_t event);

/* GATT Event Callback Functions */
static wiced_bt_gatt_status_t ble_app_write_handler(uint16_t conn_id,
                                            wiced_bt_gatt_opcode_t opcode,
                                    wiced_bt_gatt_write_req_t *p_write_req);
static wiced_bt_gatt_status_t ble_app_connect_callback
                            (wiced_bt_gatt_connection_status_t *p_conn_status);
static wiced_bt_gatt_status_t ble_app_server_callback
                            (wiced_bt_gatt_attribute_request_t *p_attr_req, uint16_t *p_error_handle);
static wiced_bt_gatt_status_t ble_app_gatt_event_handler(wiced_bt_gatt_evt_t event,
                                    wiced_bt_gatt_event_data_t *p_event_data);
static wiced_bt_gatt_status_t ble_app_read_handler(uint16_t conn_id,
                                                wiced_bt_gatt_opcode_t opcode,
                                                wiced_bt_gatt_read_t *p_read_req,
                                                uint16_t len_requested);
static gatt_db_lookup_table_t *app_bt_find_by_handle(uint16_t handle);
static wiced_bt_gatt_status_t app_bt_gatt_req_read_by_type_handler(uint16_t conn_id,
                                                                   wiced_bt_gatt_opcode_t opcode,
                                                                   wiced_bt_gatt_read_by_type_t *p_read_req,
                                                                   uint16_t len_requested,
                                                                   uint16_t *p_error_handle);

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/

/*******************************************************************************
 * Function Name: app_bt_free_buffer()
 ******************************************************************************/
void app_bt_free_buffer(uint8_t *p_buf)
{
    vPortFree(p_buf);
}

/*******************************************************************************
 * Function Name: app_bt_alloc_buffer()
 ******************************************************************************/
void *app_bt_alloc_buffer(int len)
{
    return pvPortMalloc(len);
}

/*******************************************************************************
 * Function Name: app_bt_management_callback()
 *
 * Summary:
 *   Bluetooth stack event handler function to receive management events from
 *   the BT stack and process as per the application.
 ******************************************************************************/
wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
                            wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t status = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = {0};

    wiced_bt_ble_pref_conn_params_t conn_param_t = {
        .conn_interval_min        = APP_CONN_INTERVAL_MIN,
        .conn_interval_max        = APP_CONN_INTERVAL_MAX + 1,
        .conn_latency             = APP_CONN_LATENCY,
        .conn_supervision_timeout = APP_SUPERVISION_TIMEOUT,
        .min_ce_length            = 0,
        .max_ce_length            = 0,
    };

    switch (event)
    {
    case BTM_ENABLED_EVT:
        /* Bluetooth Controller and Host Stack Enabled */
        if (WICED_BT_SUCCESS == p_event_data->enabled.status)
        {
            wiced_bt_dev_read_local_addr(bda);
#if APP_DEBUG_ENABLED
            printf("Local Bluetooth Address: ");
            print_bd_address(bda);
#endif
            /* Perform application-specific initialization */
            tput_ble_app_init();
        }
        else
        {
#if APP_DEBUG_ENABLED
            printf("Bluetooth Disabled\n");
#endif
        }
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        {
            wiced_bt_ble_advert_mode_t p_adv_mode = p_event_data->ble_advert_state_changed;
#if APP_DEBUG_ENABLED
            printf("Advertisement State Change: %s\n",
                                            get_bt_advert_mode_name(p_adv_mode));
#endif
            if (BTM_BLE_ADVERT_OFF == p_adv_mode)
            {
                if (0 == conn_state_info.conn_id)
                {
                    app_bt_adv_conn_state = APP_BT_ADV_OFF_CONN_OFF;
                }
                else
                {
                    app_bt_adv_conn_state = APP_BT_ADV_OFF_CONN_ON;
                }
            }
            else
            {
                app_bt_adv_conn_state = APP_BT_ADV_ON_CONN_OFF;
            }
        }
        break;

    case BTM_BLE_PHY_UPDATE_EVT:
        conn_state_info.rx_phy = p_event_data->ble_phy_update_event.rx_phy;
        conn_state_info.tx_phy = p_event_data->ble_phy_update_event.tx_phy;
#if APP_DEBUG_ENABLED
        printf("Selected RX PHY - %dM\nSelected TX PHY - %dM\nPeer address = ",
                                conn_state_info.rx_phy, conn_state_info.tx_phy);
        print_bd_address(conn_state_info.remote_addr);
#endif
#if APP_ENABLE_CONN_PARAM_UPDATE
        /* Send connection interval update after PHY is updated */
        conn_param_status = wiced_bt_l2cap_update_ble_conn_params(
                                conn_state_info.remote_addr, &conn_param_t);
#if APP_DEBUG_ENABLED
        if (TRUE == conn_param_status)
        {
            printf("Connection Interval update started\n");
        }
        else
        {
            printf("Failed to send connection interval update\n");
        }
#endif
#endif /* APP_ENABLE_CONN_PARAM_UPDATE */
        break;

    case BTM_BLE_CONNECTION_PARAM_UPDATE:
        /* Connection parameters updated */
        if (WICED_SUCCESS == p_event_data->ble_connection_param_update.status)
        {
            conn_state_info.conn_interval = (double)((p_event_data->ble_connection_param_update.conn_interval)
                                                    * CONN_INTERVAL_MULTIPLIER);
#if APP_DEBUG_ENABLED
            printf("New connection interval: %f ms\n",
                                                conn_state_info.conn_interval);
            printf("STO = %d\n",
            (p_event_data->ble_connection_param_update.supervision_timeout
                                                            * STO_MULTIPLIER));
#endif
        }
        else
        {
#if APP_DEBUG_ENABLED
            printf("Connection parameters update failed: %d\n",
                            p_event_data->ble_connection_param_update.status);
#endif
        }
        break;

    default:
#if APP_DEBUG_ENABLED
        printf("Unhandled Bluetooth Management Event: 0x%x %s\n",
                                            event, get_bt_event_name(event));
#endif
        break;
    }
    return status;
}

/*******************************************************************************
 * Function Name: tput_ble_app_init()
 *
 * Summary:
 *   Application level initialization called from BTM_ENABLED_EVT.
 ******************************************************************************/
static void tput_ble_app_init(void)
{
    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    wiced_result_t result = WICED_BT_SUCCESS;

#if APP_DEBUG_ENABLED
    printf("\n***********************************************\n");
    printf("**Discover device with \"TPUT\" name*\n");
    printf("***********************************************\n\n");
#endif

    /* Initialize the data packets to be sent as GATT notifications */
    for (uint8_t iterator = 0; iterator < NOTIFICATION_DATA_SIZE; iterator++)
    {
        notification_data_seq1[iterator] = iterator;
    }
    for (uint8_t iterator = 0; iterator < NOTIFICATION_DATA_SIZE; iterator++)
    {
        notification_data_seq2[iterator] = value_initialize;
        value_initialize--;
    }

    /* Initialize the timer used to calculate throughput (1 second) */
    rslt = cyhal_timer_init(&get_throughput_timer_obj, NC, NULL);
    if (CY_RSLT_SUCCESS != rslt)
    {
#if APP_DEBUG_ENABLED
        printf("Get throughput timer init failed!\n");
#endif
        CY_ASSERT(0);
    }
    cyhal_timer_configure(&get_throughput_timer_obj, &get_throughput_timer_cfg);
    rslt = cyhal_timer_set_frequency(&get_throughput_timer_obj, FREQUENCY);
    if (CY_RSLT_SUCCESS != rslt)
    {
#if APP_DEBUG_ENABLED
        printf("Get throughput timer set freq failed!\n");
#endif
        CY_ASSERT(0);
    }
    cyhal_timer_register_callback(&get_throughput_timer_obj,
                                  app_throughput_timer_callb,
                                  NULL);
    cyhal_timer_enable_event(&get_throughput_timer_obj,
                             CYHAL_TIMER_IRQ_TERMINAL_COUNT,
                             TIMER_INTERRUPT_PRIORITY,
                             true);

    /* Initialize the timer used to trigger notification sending (1 ms) */
    rslt = cyhal_timer_init(&app_millisec_timer_obj, NC, NULL);
    if (CY_RSLT_SUCCESS != rslt)
    {
#if APP_DEBUG_ENABLED
        printf("Get notification timer init failed!\n");
#endif
        CY_ASSERT(0);
    }
    cyhal_timer_configure(&app_millisec_timer_obj, &app_millisec_timer_cfg);
    rslt = cyhal_timer_set_frequency(&app_millisec_timer_obj, FREQUENCY);
    if (CY_RSLT_SUCCESS != rslt)
    {
#if APP_DEBUG_ENABLED
        printf("Notification timer set freq failed!\n");
#endif
        CY_ASSERT(0);
    }
    cyhal_timer_register_callback(&app_millisec_timer_obj,
                                  app_millisec_timer_callb,
                                  NULL);
    cyhal_timer_enable_event(&app_millisec_timer_obj,
                             CYHAL_TIMER_IRQ_TERMINAL_COUNT,
                             TIMER_INTERRUPT_PRIORITY,
                             true);

    /* Disable pairing for this application */
    wiced_bt_set_pairable_mode(WICED_FALSE, 0);

    /* Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE,
                                            cy_bt_adv_packet_data);

    /* Register with BT stack to receive GATT callback */
    status = wiced_bt_gatt_register(ble_app_gatt_event_handler);
#if APP_DEBUG_ENABLED
    printf("GATT event Handler registration status: %s\n",
                                            get_bt_gatt_status_name(status));
#endif
    if (WICED_BT_GATT_SUCCESS != status)
    {
#if APP_DEBUG_ENABLED
        printf("GATT Registration failed because of error: %d\n", status);
#endif
        CY_ASSERT(0);
    }

    /* Initialize GATT Database */
    status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
#if APP_DEBUG_ENABLED
    printf("GATT database initialization status: %s\n",
                                            get_bt_gatt_status_name(status));
#endif
    if (WICED_BT_GATT_SUCCESS != status)
    {
#if APP_DEBUG_ENABLED
        printf("GATT initialization failed because of error: %d\n", status);
#endif
        CY_ASSERT(0);
    }

    /* Start Undirected LE Advertisements on device startup */
    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL);
    if (WICED_BT_SUCCESS != result)
    {
#if APP_DEBUG_ENABLED
        printf("Advertisement cannot start because of error: %d\n", result);
#endif
        CY_ASSERT(0);
    }
}

/*******************************************************************************
 * Function Name: ble_app_gatt_event_handler()
 *
 * Summary:
 *   This function handles GATT events from the BT stack.
 ******************************************************************************/
static wiced_bt_gatt_status_t ble_app_gatt_event_handler(wiced_bt_gatt_evt_t event,
                                    wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    uint16_t error_handle = 0;

    switch (event)
    {
    case GATT_CONNECTION_STATUS_EVT:
        status = ble_app_connect_callback(&p_event_data->connection_status);
        break;

    case GATT_ATTRIBUTE_REQUEST_EVT:
        status = ble_app_server_callback(&p_event_data->attribute_request, &error_handle);
        break;

    case GATT_CONGESTION_EVT:
        if (!p_event_data->congestion.congested)
        {
            xTaskNotifyGiveIndexed(notif_send_task_handle,
                                   TASK_NOTIFY_NO_GATT_CONGESTION);
        }
        status = WICED_BT_GATT_SUCCESS;
        break;

    case GATT_GET_RESPONSE_BUFFER_EVT:
        p_event_data->buffer_request.buffer.p_app_rsp_buffer =
            app_bt_alloc_buffer(p_event_data->buffer_request.len_requested);
        p_event_data->buffer_request.buffer.p_app_ctxt =
            (void *)app_bt_free_buffer;
        status = WICED_BT_GATT_SUCCESS;
        break;

    case GATT_APP_BUFFER_TRANSMITTED_EVT:
        status = WICED_BT_GATT_SUCCESS;
        break;

    default:
        status = WICED_BT_GATT_SUCCESS;
        break;
    }
    return status;
}

/*******************************************************************************
 * Function Name: ble_app_write_handler()
 *
 * Summary:
 *   Handles Write Requests received from the client device.
 ******************************************************************************/
static wiced_bt_gatt_status_t ble_app_write_handler(uint16_t conn_id,
                                                wiced_bt_gatt_opcode_t opcode,
                                        wiced_bt_gatt_write_req_t *p_write_req)
{
    wiced_bool_t isHandleInTable = WICED_FALSE;
    wiced_bool_t validLen = WICED_FALSE;
    wiced_bt_gatt_status_t res = WICED_BT_GATT_INVALID_HANDLE;
    wiced_bt_gatt_status_t write_status;

    /* Check for a matching handle entry */
    for (int i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if (app_gatt_db_ext_attr_tbl[i].handle == p_write_req->handle)
        {
            isHandleInTable = WICED_TRUE;

            /* Check if the buffer has space to store the data */
            validLen = (app_gatt_db_ext_attr_tbl[i].max_len >= p_write_req->val_len);

            if (validLen)
            {
                /* Value fits within the supplied buffer; copy over the value */
                app_gatt_db_ext_attr_tbl[i].cur_len = p_write_req->val_len;
                memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_write_req->p_val,
                                                        p_write_req->val_len);
                res = WICED_BT_GATT_SUCCESS;

                switch (p_write_req->handle)
                {
                case HDLD_THROUGHPUT_MEASUREMENT_NOTIFY_CLIENT_CHAR_CONFIG:
                    if (app_throughput_measurement_notify_client_char_config[0])
                    {
#if APP_DEBUG_ENABLED
                        printf("\rNotifications enabled\n");
#endif
                        gatt_write_rx_bytes = 0;
                        if (CY_RSLT_SUCCESS != cyhal_timer_start(&app_millisec_timer_obj))
                        {
#if APP_DEBUG_ENABLED
                            printf("Notification timer start failed!");
#endif
                            CY_ASSERT(0);
                        }
                    }
                    else
                    {
#if APP_DEBUG_ENABLED
                        printf("\rNotifications disabled\n");
#endif
                        if (CY_RSLT_SUCCESS != cyhal_timer_stop(&app_millisec_timer_obj))
                        {
#if APP_DEBUG_ENABLED
                            printf("Notification timer stop failed!");
#endif
                            CY_ASSERT(0);
                        }
                        gatt_notif_tx_bytes = 0;
                        res = WICED_BT_GATT_SUCCESS;
                    }
                    write_status = wiced_bt_gatt_server_send_write_rsp(conn_id,
                                                                    opcode,
                                                        p_write_req->handle);
                    if (WICED_BT_GATT_SUCCESS != write_status)
                    {
#if APP_DEBUG_ENABLED
                        printf("wiced_bt_gatt_server_send_write_rsp failed");
#endif
                        CY_ASSERT(0);
                    }
                    break;

                case HDLC_THROUGHPUT_MEASUREMENT_WRITEME_VALUE:
                    /* Receive GATT write commands from client and update
                     * the counter with number of bytes received */
                    gatt_write_rx_bytes += p_write_req->val_len;
                    res = WICED_BT_GATT_SUCCESS;
                    break;
                }
            }
            else
            {
                /* Value to write does not meet size constraints */
                res = WICED_BT_GATT_INVALID_ATTR_LEN;
            }
            break;
        }
    }

    if (!isHandleInTable)
    {
#if APP_DEBUG_ENABLED
        printf("Write Request to Invalid Handle: 0x%x\n", p_write_req->handle);
#endif
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_write_req->handle,
                                            WICED_BT_GATT_INVALID_HANDLE);
        res = WICED_BT_GATT_INVALID_HANDLE;
    }

    return res;
}

/*******************************************************************************
 * Function Name: app_bt_find_by_handle()
 *
 * Summary:
 *   Find attribute description by handle.
 ******************************************************************************/
static gatt_db_lookup_table_t *app_bt_find_by_handle(uint16_t handle)
{
    int i;
    for (i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if (handle == app_gatt_db_ext_attr_tbl[i].handle)
        {
            return (&app_gatt_db_ext_attr_tbl[i]);
        }
    }
    return NULL;
}

/*******************************************************************************
 * Function Name: ble_app_read_handler()
 *
 * Summary:
 *   Handles Read Requests received from the client device.
 ******************************************************************************/
static wiced_bt_gatt_status_t ble_app_read_handler(uint16_t conn_id,
                                    wiced_bt_gatt_opcode_t opcode,
                                    wiced_bt_gatt_read_t *p_read_req,
                                    uint16_t len_req)
{
    gatt_db_lookup_table_t *puAttribute;
    int attr_len_to_copy;
    uint8_t *from;
    int to_send;

    puAttribute = app_bt_find_by_handle(p_read_req->handle);
    if (NULL == puAttribute)
    {
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->handle,
                                            WICED_BT_GATT_INVALID_HANDLE);
        return WICED_BT_GATT_INVALID_HANDLE;
    }
    attr_len_to_copy = puAttribute->cur_len;

#if APP_DEBUG_ENABLED
    printf("read_handler: conn_id:%d Handle:%x offset:%d len:%d\n",
            conn_id, p_read_req->handle, p_read_req->offset, attr_len_to_copy);
#endif

    if (p_read_req->offset >= puAttribute->cur_len)
    {
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->handle,
                                            WICED_BT_GATT_INVALID_OFFSET);
        return WICED_BT_GATT_INVALID_OFFSET;
    }

    to_send = MIN(len_req, attr_len_to_copy - p_read_req->offset);
    from = ((uint8_t *)puAttribute->p_data) + p_read_req->offset;

    return wiced_bt_gatt_server_send_read_handle_rsp(conn_id,
                                                     opcode,
                                                     to_send,
                                                     from,
                                                     NULL);
}

/*******************************************************************************
 * Function Name: ble_app_connect_callback()
 *
 * Summary:
 *   Handles connection status changes.
 ******************************************************************************/
static wiced_bt_gatt_status_t ble_app_connect_callback(
                            wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    wiced_result_t result = WICED_BT_SUCCESS;

    if (NULL != p_conn_status)
    {
        if (p_conn_status->connected)
        {
            /* Device has connected */
#if APP_DEBUG_ENABLED
            printf("Connected : BDA ");
            print_bd_address(p_conn_status->bd_addr);
            printf("Connection ID '%d'\n", p_conn_status->conn_id);
#endif
            /* Store the connection ID */
            conn_state_info.conn_id = p_conn_status->conn_id;

            /* Save BD address of the connected device */
            memcpy(conn_state_info.remote_addr, p_conn_status->bd_addr,
                                                        BD_ADDR_LEN);

            /* Set default MTU and packet size */
            conn_state_info.mtu = 23;
            packet_size = tput_get_notification_packet_size(conn_state_info.mtu);

            /* Update the adv/conn state */
            app_bt_adv_conn_state = APP_BT_ADV_OFF_CONN_ON;

#if APP_ENABLE_PHY_UPDATE
            /* Request PHY update based on app_config.h setting */
            wiced_bt_ble_phy_preferences_t phy_preferences;
#if (APP_SELECTED_PHY == APP_PHY_2M)
            phy_preferences.rx_phys = BTM_BLE_PREFER_2M_PHY;
            phy_preferences.tx_phys = BTM_BLE_PREFER_2M_PHY;
#else
            phy_preferences.rx_phys = BTM_BLE_PREFER_1M_PHY;
            phy_preferences.tx_phys = BTM_BLE_PREFER_1M_PHY;
#endif
            memcpy(phy_preferences.remote_bd_addr, conn_state_info.remote_addr,
                                                                BD_ADDR_LEN);
            result = wiced_bt_ble_set_phy(&phy_preferences);
#if APP_DEBUG_ENABLED
            if (result == WICED_BT_SUCCESS)
            {
                printf("Request sent to switch PHY to %dM\n",
                                    phy_preferences.tx_phys);
            }
            else
            {
                printf("PHY switch request failed, result: %d\n", result);
            }
#endif
#endif /* APP_ENABLE_PHY_UPDATE */

            /* Start the throughput calculation timer */
            if (CY_RSLT_SUCCESS != cyhal_timer_start(&get_throughput_timer_obj))
            {
#if APP_DEBUG_ENABLED
                printf("Get throughput timer start failed!");
#endif
                CY_ASSERT(0);
            }
        }
        else
        {
            /* Device has disconnected */
#if APP_DEBUG_ENABLED
            printf("Disconnected : BDA ");
            print_bd_address(p_conn_status->bd_addr);
            printf("Connection ID '%d', Reason '%s'\n", p_conn_status->conn_id,
                        get_bt_gatt_disconn_reason_name(p_conn_status->reason));
#endif
            /* Reset the connection information */
            memset(&conn_state_info, 0u, sizeof(conn_state_info));

            /* Stop the timers */
            cyhal_timer_stop(&get_throughput_timer_obj);
            cyhal_timer_stop(&app_millisec_timer_obj);

            /* Restart the advertisements */
            result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                                    0, NULL);
            if (WICED_BT_SUCCESS != result)
            {
#if APP_DEBUG_ENABLED
                printf("Advertisement cannot start because of error: %d\n", result);
#endif
                CY_ASSERT(0);
            }

            /* Update the adv/conn state */
            app_bt_adv_conn_state = APP_BT_ADV_ON_CONN_OFF;
        }
        status = WICED_BT_GATT_SUCCESS;
    }
    return status;
}

/*******************************************************************************
 * Function Name: ble_app_server_callback()
 *
 * Summary:
 *   Handles GATT server events from the BT stack.
 ******************************************************************************/
static wiced_bt_gatt_status_t ble_app_server_callback(
                                wiced_bt_gatt_attribute_request_t *p_attr_req,
                                uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    switch (p_attr_req->opcode)
    {
    case GATT_REQ_READ:
    case GATT_REQ_READ_BLOB:
        status = ble_app_read_handler(p_attr_req->conn_id,
                                      p_attr_req->opcode,
                                      &p_attr_req->data.read_req,
                                      p_attr_req->len_requested);
        break;

    case GATT_REQ_READ_BY_TYPE:
        status = app_bt_gatt_req_read_by_type_handler(p_attr_req->conn_id,
                                                      p_attr_req->opcode,
                                                      &p_attr_req->data.read_by_type,
                                                      p_attr_req->len_requested,
                                                      p_error_handle);
        break;

    case GATT_REQ_WRITE:
    case GATT_CMD_WRITE:
        status = ble_app_write_handler(p_attr_req->conn_id,
                                       p_attr_req->opcode,
                                       &p_attr_req->data.write_req);
        break;

    case GATT_REQ_MTU:
#if APP_DEBUG_ENABLED
        printf("\rClient MTU: %d\n", p_attr_req->data.remote_mtu);
#endif
        conn_state_info.mtu = APP_MTU_SIZE <= (p_attr_req->data.remote_mtu) ?
                                APP_MTU_SIZE :
                                (p_attr_req->data.remote_mtu);
        packet_size = tput_get_notification_packet_size(conn_state_info.mtu);
        wiced_bt_gatt_server_send_mtu_rsp(p_attr_req->conn_id,
                                          p_attr_req->data.remote_mtu,
                                          APP_MTU_SIZE);
        status = WICED_BT_GATT_SUCCESS;
        break;
    }
    return status;
}

/*******************************************************************************
 * Function Name: app_throughput_timer_callb()
 *
 * Summary:
 *   One second timer callback. Notifies the throughput calculation task.
 ******************************************************************************/
void app_throughput_timer_callb(void *callback_arg, cyhal_timer_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(get_throughput_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
 * Function Name: get_throughput_task()
 *
 * Summary:
 *   Calculates and displays throughput values every second.
 ******************************************************************************/
void get_throughput_task(void *pvParam)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Display GATT TX throughput result */
        if ((conn_state_info.conn_id) &&
                (app_throughput_measurement_notify_client_char_config[0]) &&
                (gatt_notif_tx_bytes))
        {
            /* GATT Throughput = (bytes in 1 second * 8 bits) bps */
            gatt_notif_tx_bytes = (gatt_notif_tx_bytes * 8) / 1000;
#if APP_DEBUG_ENABLED
            printf("\rGATT NOTIFICATION : Server Throughput (TX)= %lu kbps\n",
                   gatt_notif_tx_bytes);
#endif
            gatt_notif_tx_bytes = 0;
        }

        /* Display GATT RX throughput result */
        if (conn_state_info.conn_id && gatt_write_rx_bytes)
        {
            gatt_write_rx_bytes = (gatt_write_rx_bytes * 8) / 1000;
#if APP_DEBUG_ENABLED
            printf("\rGATT WRITE        : Server Throughput (RX)= %lu kbps\n",
                                                        gatt_write_rx_bytes);
#endif
            gatt_write_rx_bytes = 0;
        }
    }
}

/*******************************************************************************
 * Function Name: app_millisec_timer_callb()
 *
 * Summary:
 *   One millisecond timer callback. Notifies the notification send task.
 ******************************************************************************/
void app_millisec_timer_callb(void *callback_arg, cyhal_timer_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveIndexedFromISR(notif_send_task_handle,
                                  TASK_NOTIFY_1MS_TIMER,
                                  &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
 * Function Name: send_notification_task()
 *
 * Summary:
 *   Sends GATT notifications when triggered by the 1ms timer.
 ******************************************************************************/
void send_notification_task(void *pvParam)
{
    while (true)
    {
        ulTaskNotifyTakeIndexed(TASK_NOTIFY_1MS_TIMER, pdTRUE, portMAX_DELAY);
        if (app_throughput_measurement_notify_client_char_config[0])
        {
            tput_send_notification();
        }
    }
}

/*******************************************************************************
 * Function Name: tput_send_notification()
 *
 * Summary:
 *   Sends a GATT notification using alternating data buffers.
 ******************************************************************************/
static void tput_send_notification(void)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    if (data_flag == 0)
    {
        status = wiced_bt_gatt_server_send_notification(conn_state_info.conn_id,
                                     HDLC_THROUGHPUT_MEASUREMENT_NOTIFY_VALUE,
                                     packet_size,
                                     notification_data_seq1,
                                     NULL);
    }
    else
    {
        status = wiced_bt_gatt_server_send_notification(conn_state_info.conn_id,
                                     HDLC_THROUGHPUT_MEASUREMENT_NOTIFY_VALUE,
                                     packet_size,
                                     notification_data_seq2,
                                     NULL);
    }

    if (WICED_BT_GATT_SUCCESS == status)
    {
        gatt_notif_tx_bytes += packet_size;
        data_flag = data_flag == 0 ? 1 : 0;
    }
    else if (WICED_BT_GATT_CONGESTED == status)
    {
        ulTaskNotifyTakeIndexed(TASK_NOTIFY_NO_GATT_CONGESTION,
                                pdTRUE,
                                portMAX_DELAY);
    }
}

/*******************************************************************************
 * Function Name: tput_get_notification_packet_size()
 *
 * Summary:
 *   Decides size of notification packet based on the ATT MTU exchanged to
 *   utilize the LL payload space effectively.
 ******************************************************************************/
static uint16_t tput_get_notification_packet_size(uint16_t att_mtu_size)
{
    if (att_mtu_size < DATA_PACKET_SIZE_1 + ATT_HEADER)
    {
        /* Packet Length = ATT_MTU_SIZE - ATT_HANDLE(2 bytes) - ATT_OPCODE(1 byte)
         * With DLE enabled, LL payload is 251 bytes. If MTU < 247,
         * data fits in a single LL packet */
        packet_size = att_mtu_size - ATT_HEADER;
    }
    else if ((att_mtu_size >= DATA_PACKET_SIZE_1 + ATT_HEADER) &&
                (att_mtu_size < DATA_PACKET_SIZE_2 + ATT_HEADER))
    {
        /* If MTU is between 247 and 498, use 244 bytes to utilize
         * LL payload space effectively */
        packet_size = DATA_PACKET_SIZE_1;
    }
    else
    {
        /* For MTU > 498, use 495 bytes (two LL packets: 244 + 251) */
        packet_size = DATA_PACKET_SIZE_2;
    }
    return packet_size;
}

/*******************************************************************************
 * Function Name: app_bt_gatt_req_read_by_type_handler()
 *
 * Summary:
 *   Process read-by-type request from peer device.
 ******************************************************************************/
static wiced_bt_gatt_status_t app_bt_gatt_req_read_by_type_handler(uint16_t conn_id,
                                                                   wiced_bt_gatt_opcode_t opcode,
                                                                   wiced_bt_gatt_read_by_type_t *p_read_req,
                                                                   uint16_t len_requested,
                                                                   uint16_t *p_error_handle)
{
    gatt_db_lookup_table_t *puAttribute;
    uint16_t last_handle = 0;
    uint16_t attr_handle = p_read_req->s_handle;
    uint8_t *p_rsp = app_bt_alloc_buffer(len_requested);
    uint8_t pair_len = 0;
    int used_len = 0;

    if (NULL == p_rsp)
    {
#if APP_DEBUG_ENABLED
        printf("No memory, len_requested: %d!!\r\n", len_requested);
#endif
        return WICED_BT_GATT_INSUF_RESOURCE;
    }

    /* Read by type returns all attributes of the specified type,
     * between the start and end handles */
    while (WICED_TRUE)
    {
        *p_error_handle = attr_handle;
        last_handle = attr_handle;
        attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle,
                                                        p_read_req->e_handle,
                                                        &p_read_req->uuid);
        if (0 == attr_handle)
            break;

        if (NULL == (puAttribute = app_bt_find_by_handle(attr_handle)))
        {
#if APP_DEBUG_ENABLED
            printf("found type but no attribute for %d\r\n", last_handle);
#endif
            app_bt_free_buffer(p_rsp);
            return WICED_BT_GATT_INVALID_HANDLE;
        }

        {
            int filled = wiced_bt_gatt_put_read_by_type_rsp_in_stream(
                            p_rsp + used_len,
                            len_requested - used_len,
                            &pair_len,
                            attr_handle,
                            puAttribute->cur_len,
                            puAttribute->p_data);
            if (0 == filled)
            {
                break;
            }
            used_len += filled;
        }

        /* Increment starting handle for next search to one past current */
        attr_handle++;
    }

    if (0 == used_len)
    {
#if APP_DEBUG_ENABLED
        printf("attr not found  start_handle: 0x%04x  end_handle: 0x%04x  Type: 0x%04x\r\n",
               p_read_req->s_handle, p_read_req->e_handle, p_read_req->uuid.uu.uuid16);
#endif
        app_bt_free_buffer(p_rsp);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    /* Send the response */
    return wiced_bt_gatt_server_send_read_by_type_rsp(conn_id, opcode,
                                                      pair_len, used_len,
                                                      p_rsp,
                                                      (void *)app_bt_free_buffer);
}

/* [] END OF FILE */
