/*******************************************************************************
 * (c) 2021-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 *******************************************************************************/

#include <stdint.h>

#include "user_btn.h"

#include <FreeRTOS.h>
#include <task.h>

#include "cybsp_types.h"
#include "cycfg_pins.h"
#include "cyhal_gpio.h"

/* Stack size for BTN task */
#define BTN_TASK_STACK_SIZE (512u)

#define GPIO_INTERRUPT_PRIORITY (4u)

/* Minimum time between button presses (milliseconds) */
#define BTN_DEBOUNCE_MS 300

/* Handle of the btn task */
static TaskHandle_t btn_handle;
static gpio_interrupt_handler_t g_gpio_interrupt_handler_ptr = NULL;

static void gpio_interrupt_handler(void *callback_arg, cyhal_gpio_event_t event);

static cyhal_gpio_callback_data_t cb_data = {.callback = gpio_interrupt_handler, .callback_arg = NULL};

static void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static TickType_t previous_press_tick = 0;

    if (event != CYHAL_GPIO_IRQ_FALL) return;

    TickType_t current_tick = xTaskGetTickCountFromISR();

    /* Debounce: require at least BTN_DEBOUNCE_MS between presses */
    if ((current_tick - previous_press_tick) > pdMS_TO_TICKS(BTN_DEBOUNCE_MS))
    {
        previous_press_tick = current_tick;

        vTaskNotifyGiveIndexedFromISR(btn_handle, 0, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void user_btn_thread(void *arg)
{
    for (;;) {
        ulTaskNotifyTakeIndexed(0,              /* Use the 0th notification */
                                pdTRUE,         /* Clear the notification value before exiting. */
                                portMAX_DELAY); /* Block indefinitely. */

        if (g_gpio_interrupt_handler_ptr) g_gpio_interrupt_handler_ptr();
    }
}

void configure_user_btn(gpio_interrupt_handler_t gpio_interrupt_handler)
{
    BaseType_t xReturned;

    xReturned = xTaskCreate(user_btn_thread, "User btn task", BTN_TASK_STACK_SIZE, NULL, 4, &btn_handle);
    if (xReturned != pdPASS) return;

    /* Initialize the user button with internal pull-up (active-low button) */
    cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);

    /* Configure GPIO interrupt on falling edge (button press) */
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &cb_data);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, GPIO_INTERRUPT_PRIORITY, true);

    g_gpio_interrupt_handler_ptr = gpio_interrupt_handler;
}
