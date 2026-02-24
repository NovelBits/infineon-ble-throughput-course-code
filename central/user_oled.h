/*******************************************************************************
* File Name:   user_oled.h
*
* Description: Header file for the SSD1306 OLED display driver.
*
* Related Document: See README.md
*
********************************************************************************
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

/*******************************************************************************
 * Include guard
 ******************************************************************************/
#ifndef _USER_OLED_H_
#define _USER_OLED_H_

/*******************************************************************************
 * Header file includes
 ******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define SSD1309_I2C_ADDRESS                             0x3C

#define SSD1309_LCDWIDTH                                128u
#define SSD1309_LCDHEIGHT                               64u

#define SSD1309_SETCONTRAST                             0x81
#define SSD1309_DISPLAY_RAM                             0xA4
#define SSD1309_DISPLAYALLON                            0xA5
#define SSD1309_DISPLAY_NORMAL                          0xA6
#define SSD1309_DISPLAY_INVERTED                        0xA7
#define SSD1309_DISPLAYOFF                              0xAE
#define SSD1309_DISPLAYON                               0xAF

#define SSD1309_SETSTARTLINE                            0x40
#define SSD1309_SETDISPLAYOFFSET                        0xD3
#define SSD1309_SETCOMPINS                              0xDA
#define SSD1309_SETVCOMDETECT                           0xDB
#define SSD1309_SETDISPLAYCLOCKDIV                      0xD5
#define SSD1309_SETPRECHARGE                            0xD9
#define SSD1309_SETMULTIPLEX                            0xA8
#define SSD1309_SETLOWCOLUMN                            0x00
#define SSD1309_SETHIGHCOLUMN                           0x10

#define SSD1309_MEMORYMODE                              0x20
#define SSD1309_COLUMNADDR                              0x21
#define SSD1309_PAGEADDR                                0x22
#define SSD1309_COMSCANINC                              0xC0
#define SSD1309_COMSCANDEC                              0xC8
#define SSD1309_SEGREMAP                                0xA0
#define SSD1309_CHARGEPUMP                              0x8D

#define SSD1309_EXTERNALVCC                             0x01
#define SSD1309_SWITCHCAPVCC                            0x02

#define SSD1309_ACTIVATE_SCROLL                         0x2F
#define SSD1309_DEACTIVATE_SCROLL                       0x2E
#define SSD1309_LEFT_HORIZONTAL_SCROLL                  0x27
#define SSD1309_RIGHT_HORIZONTAL_SCROLL                 0x26
#define SSD1309_SET_VERTICAL_SCROLL_AREA                0xA3
#define SSD1309_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define SSD1309_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
cy_rslt_t oled_init(void);
void oled_clear_screen(void);
void oled_clearLine(uint8_t line);
void oled_printText(uint8_t line, uint8_t x, char data[]);
void oled_printLargeText(uint8_t line, uint8_t x, char data[]);

#endif /* _USER_OLED_H_ */

/* [] END OF FILE */
