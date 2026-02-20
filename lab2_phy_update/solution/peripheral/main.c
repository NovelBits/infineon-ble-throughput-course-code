/*******************************************************************************
 * File Name: main.c
 *
 * Description: Entry point for the Bluetooth LE Throughput Peripheral firmware.
 *              Initializes BSP, BT stack, and FreeRTOS tasks.
 *
 *******************************************************************************
 * Copyright 2024, Novel Bits, LLC. Based on Infineon Technologies AG examples.
 ******************************************************************************/

/*******************************************************************************
 *        Header Files
 ******************************************************************************/
#include "throughput_server.h"
#include "wiced_bt_stack.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cycfg_bt_settings.h"
#include "cybsp_bt_config.h"

/*******************************************************************************
 *        Macros
 ******************************************************************************/
#define TASK_PRIORITY               (configMAX_PRIORITIES - 4)
#define TASK_STACK_SIZE             (configMINIMAL_STACK_SIZE * 4)
#define THROUGHPUT_TASK_STRING      "Throughput Task"
#define NOTIFICATION_TASK_STRING    "Notification Task"

/*******************************************************************************
 *        Global Variables
 ******************************************************************************/
TaskHandle_t notif_send_task_handle;
TaskHandle_t get_throughput_task_handle;

/*******************************************************************************
 *        Function Definitions
 ******************************************************************************/
int main()
{
    cy_rslt_t bsp_result = CY_RSLT_SUCCESS;
    wiced_result_t result = WICED_SUCCESS;
    BaseType_t rtos_result;

    /* Initialize the board support package */
    bsp_result = cybsp_init();
    if (CY_RSLT_SUCCESS != bsp_result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX,
                        CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    printf("\n");
    printf("==============================================\n");
    printf("  BLE Throughput: Peripheral\n");
    printf("==============================================\n\n");

    /* Initialize Bluetooth porting layer with HCI configuration */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Register callback and configuration with stack */
    result = wiced_bt_stack_init(app_bt_management_callback,
                                &wiced_bt_cfg_settings);
    if (WICED_BT_SUCCESS == result)
    {
        printf("Bluetooth Stack Initialization Successful\n");
    }
    else
    {
        printf("Bluetooth Stack Initialization failed!!\n");
        CY_ASSERT(0);
    }

    /* Create a task to calculate throughput */
    rtos_result = xTaskCreate(get_throughput_task, THROUGHPUT_TASK_STRING,
                              TASK_STACK_SIZE, NULL, TASK_PRIORITY,
                              &get_throughput_task_handle);
    if (pdPASS != rtos_result)
    {
        printf("Throughput Task creation failed\n");
        CY_ASSERT(0);
    }

    /* Create a task to send notifications */
    rtos_result = xTaskCreate(send_notification_task, NOTIFICATION_TASK_STRING,
                              TASK_STACK_SIZE, NULL, TASK_PRIORITY,
                              &notif_send_task_handle);
    if (pdPASS != rtos_result)
    {
        printf("Notification Task creation failed\n");
        CY_ASSERT(0);
    }

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* The application should never reach here */
    CY_ASSERT(0);
}

/* [] END OF FILE */
