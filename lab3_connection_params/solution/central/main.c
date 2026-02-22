/*******************************************************************************
 * File: main.c
 *
 * Description: Entry point for the Bluetooth LE Throughput Central application.
 *              Initializes BSP, Bluetooth stack, and FreeRTOS scheduler.
 ******************************************************************************/

#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_memory.h"
#include "cycfg_bt_settings.h"
#include "cybsp_bt_config.h"
#include "cybt_platform_trace.h"
#include "throughput_central.h"
#include <FreeRTOS.h>
#include <task.h>

#ifdef USE_OLED_DISP
#include "user_oled.h"
#include "oled_display.h"
#endif

/* Throughput calculation task stack size and priority */
#define THROUGHPUT_TASK_STACK_SIZE   (4096u)
#define THROUGHPUT_TASK_PRIORITY     (5u)

/*******************************************************************************
 * Function Name: main
 *
 * Summary:
 *   Entry point. Initializes BSP, retarget-io, Bluetooth stack, OLED display
 *   (if enabled), creates the throughput measurement task, and starts FreeRTOS.
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the board support package */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io for UART debug output */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                  CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Print application banner */
    printf("\r\n");
    printf("==============================================\r\n");
    printf(" Bluetooth LE Throughput - Central\r\n");
    printf("==============================================\r\n");
    printf("\r\n");

    /* Enable BT stack trace for debugging */
    cybt_platform_set_trace_level(CYBT_TRACE_ID_STACK, CYBT_TRACE_ID_MAX);

    /* Configure the Bluetooth platform */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Initialize Bluetooth stack */
    wiced_bt_stack_init(app_bt_management_callback, &wiced_bt_cfg_settings);

    /* Create a buffer heap, make it the default heap */
    wiced_bt_create_heap("app", NULL, 0x1000, NULL, WICED_TRUE);

#ifdef USE_OLED_DISP
    /* Initialize OLED display */
    oled_init();
    oled_clear_screen();
    oled_display_init();
#endif

    /* Create throughput calculation task */
    xTaskCreate(throughput_calc_task, "Throughput Task",
                THROUGHPUT_TASK_STACK_SIZE, NULL,
                THROUGHPUT_TASK_PRIORITY, &throughput_task_handle);

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    CY_ASSERT(0);
    return 0;
}
