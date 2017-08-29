/*----------------------------------------------------------------------------
 * Name:    LCD_4bit.c
 * Purpose: Functions for 2 line 16 character Text LCD (4-bit interface)
 *                connected on MCBSTM32 evaluation board
 * Version: V1.10
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * Copyright (c) 2005-2007 Keil Software. All rights reserved.
 *---------------------------------------------------------------------------*/

//#include <stm32f10x_lib.h>              /* STM32F10x Library Definitions      */
#include "mcal.h"
#include "stm32f10x.h"
#include "bsp.h"
#include "stdarg.h"
#include "stdio.h"


/*********************** Hardware specific configuration **********************/

/*------------------------- Speed dependant settings -------------------------*/

/* If processor works on high frequency delay has to be increased, it can be
   increased by factor 2^N by this constant                                   */
#define DELAY_2N     8

/*------------------------- Text LCD size definitions ------------------------*/

#define LineLen     8                  /* Width (in characters)              */
#define NumLines     2                 /* Hight (in lines)                   */




#define PINS_DATA             (0x0F <<  12)
//#define PINS_ALL              (PINS_CTRL | PINS_DATA)

//const unsigned int SWAP_DATA[16] = { 0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
//                                     0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

/* Enable Clock for peripheral driving LCD pins                               */
#define LCD_CLOCK_EN         (RCC->APB2ENR |= (1 << 4)); // enable clock for GPIOC

/* pin E  setting to 0 or 1                                                   */
#define LCD_E(x)              GPIO_write(BSP_LCD_E, x);

/* pin RW setting to 0 or 1                                                   */
#define LCD_RW(x)              GPIO_write(BSP_LCD_RW, x);

/* pin RS setting to 0 or 1                                                   */
#define LCD_RS(x)             GPIO_write(BSP_LCD_RS, x);

/* Reading DATA pins                                                          */
#define LCD_DATA_IN           (((GPIOB->IDR & PINS_DATA) >> 12) & 0x0F)

/* Writing value to DATA pins                                                 */
#define LCD_DATA_OUT(x)       GPIOB->ODR = (GPIOB->ODR & ~PINS_DATA) | ((uint16_t)(x) << 12);

/* Setting all pins to output mode                                            */
//#define LCD_ALL_DIR_OUT       GPIOC->CRL = (GPIOC->CRL & 0xFFFF0000) | 0x00003333;
//                              GPIOC->CRH = (GPIOC->CRH & 0xFFF000FF) | 0x00033300;

/* Setting DATA pins to input mode                                            */
static void LCD_DATA_DIR_IN(void )
{
  GPIO_configPin(BSP_LCD_D0, GPIO_MODE_INPUT, 0);
  GPIO_configPin(BSP_LCD_D1, GPIO_MODE_INPUT, 0);
  GPIO_configPin(BSP_LCD_D2, GPIO_MODE_INPUT, 0);
  GPIO_configPin(BSP_LCD_D3, GPIO_MODE_INPUT, 0);
}

/* Setting DATA pins to output mode                                           */
static void LCD_DATA_DIR_OUT(void)
{
  GPIO_configPin(BSP_LCD_D0, GPIO_MODE_OUTPUT, 0);
  GPIO_configPin(BSP_LCD_D1, GPIO_MODE_OUTPUT, 0);
  GPIO_configPin(BSP_LCD_D2, GPIO_MODE_OUTPUT, 0);
  GPIO_configPin(BSP_LCD_D3, GPIO_MODE_OUTPUT, 0);
}

/******************************************************************************/


/* 8 user defined characters to be loaded into CGRAM (used for bargraph)      */
const char UserFont[8][8] = {
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
  { 0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10 },
  { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 },
  { 0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C },
  { 0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E },
  { 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F },
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }
};


/************************ Global function definitions *************************/


/*******************************************************************************
* Delay in while loop cycles                                                   *
*   Parameter:    cnt:    number of while cycles to delay                      *
*   Return:                                                                    *
*******************************************************************************/

static void delay (int cnt)
{
  cnt <<= DELAY_2N;

  while (cnt--);
}


/*******************************************************************************
* Read status of LCD controller                                                *
*   Parameter:    none                                                         *
*   Return:       Status byte contains busy flag and address pointer           *
*******************************************************************************/

static unsigned char lcd_read_status (void)
{
  unsigned char status;

  LCD_DATA_DIR_IN();
  LCD_RS(0)
  LCD_RW(1)
  delay(10);
  LCD_E(1)
  delay(10);
  status  = LCD_DATA_IN << 4;
  LCD_E(0)
  delay(10);
  LCD_E(1)
  delay(10);
  status |= LCD_DATA_IN;
  LCD_E(0)
  LCD_DATA_DIR_OUT();
  return (status);
}


/*******************************************************************************
* Wait until LCD controller busy flag is 0                                     *
*   Parameter:                                                                 *
*   Return:       Status byte of LCD controller (busy + address)               *
*******************************************************************************/

static unsigned char wait_while_busy (void)
{
  unsigned char status;

  do  {
    status = lcd_read_status();
  }  while (status & 0x80);             /* Wait for busy flag                 */

  return (status);
}


/*******************************************************************************
* Write 4-bits to LCD controller                                               *
*   Parameter:    c:      command to be written                                *
*   Return:                                                                    *
*******************************************************************************/

static void lcd_write_4bit (unsigned char c)
{
  LCD_RW(0)
  LCD_E(1)
  LCD_DATA_OUT(c&0x0F)
  delay(10);
  LCD_E(0)
  delay(10);
}


/*******************************************************************************
* Write command to LCD controller                                              *
*   Parameter:    c:      command to be written                                *
*   Return:                                                                    *
*******************************************************************************/

void lcd_write_cmd (unsigned char c)
{
  wait_while_busy();

  LCD_RS(0)
  lcd_write_4bit (c>>4);
  lcd_write_4bit (c);
}


/*******************************************************************************
* Write data to LCD controller                                                 *
*   Parameter:    c:      data to be written                                   *
*   Return:                                                                    *
*******************************************************************************/

static void lcd_write_data (unsigned char c)
{
  wait_while_busy();

  LCD_RS(1)
  lcd_write_4bit (c>>4);
  lcd_write_4bit (c);
}


/*******************************************************************************
* Print Character to current cursor position                                   *
*   Parameter:    c:      character to be printed                              *
*   Return:                                                                    *
*******************************************************************************/

void hd44780_putchar (char c)
{
  lcd_write_data (c);
}


/*******************************************************************************
* Initialize the LCD controller                                                *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/
static void charge_pump_init(void)
{
  tim_t *t = TIM_create(2);
  TIM_configure(t, 8000000, 1024);

  pwm_t *pwmb = PWM_create(t, 1);
  PWM_configure(pwmb, TIM_MODE_PWM, 0, 0);

  PWM_setDuty(pwmb, PWM_MAX_DUTY * 0.05);
  PWM_outputEnable(pwmb, 1);
  TIM_start(t);
}

void hd44780_init(void)
{
  int i;
  char const *p;
  charge_pump_init();
  LCD_CLOCK_EN                          /* Enable clock for peripheral        */
  GPIO_configPin(BSP_LCD_E, GPIO_MODE_OUTPUT, 0);
  GPIO_configPin(BSP_LCD_RW, GPIO_MODE_OUTPUT, 0);
  GPIO_configPin(BSP_LCD_RS, GPIO_MODE_OUTPUT, 0);
  LCD_DATA_DIR_OUT();

  /* Set all pins for LCD as outputs                                          */
  //LCD_ALL_DIR_OUT

  delay (15000);
  LCD_RS(0)
  lcd_write_4bit (0x3);                 /* Select 4-bit interface             */
  delay (4100);
  lcd_write_4bit (0x3);
  delay (100);
  lcd_write_4bit (0x3);
  lcd_write_4bit (0x2);

  lcd_write_cmd (0x28);                 /* 2 lines, 5x8 character matrix      */
  lcd_write_cmd (0x0C);                 /* Display ctrl:Disp=ON,Curs/Blnk=OFF */
  lcd_write_cmd (0x06);                 /* Entry mode: Move right, no shift   */

  /* Load user-specific characters into CGRAM                                 */
  lcd_write_cmd(0x40);                  /* Set CGRAM address counter to 0     */
  p = &UserFont[0][0];
  for (i = 0; i < sizeof(UserFont); i++, p++)
    lcd_write_data (*p);

  lcd_write_cmd(0x80);                  /* Set DDRAM address counter to 0     */
}



/*******************************************************************************
* Set cursor position on LCD display                                           *
*   Parameter:    column: column position                                      *
*                 line:   line position                                        *
*   Return:                                                                    *
*******************************************************************************/

void hd44780_set_cursor (int column, int line)
{
  unsigned char address;

  address = (line * 40) + column;
  address = 0x80 + (address & 0x7F);
  lcd_write_cmd(address);               /* Set DDRAM address counter to 0     */
}

/*******************************************************************************
* Clear the LCD display                                                        *
*   Parameter:                                                                 *
*   Return:                                                                    *
*******************************************************************************/

void hd44780_clear (void)
{
  lcd_write_cmd(0x01);                  /* Display clear                      */
  hd44780_set_cursor (0, 0);
}


/*******************************************************************************
* Print sting to LCD display                                                   *
*   Parameter:    string: pointer to output string                             *
*   Return:                                                                    *
*******************************************************************************/

void hd44780_print (const char *string)
{
  while (*string)  {
    lcd_write_data (*string++);
  }
}


/*******************************************************************************
* Print a bargraph to LCD display                                              *
*   Parameter:     val:  value 0..100 %                                        *
*                  size: size of bargraph 1..16                                *
*   Return:                                                                    *
*******************************************************************************/
void hd44780_bargraph (int value, int size) {
   int i;

   value = value * size / 20;            /* Display matrix 5 x 8 pixels       */
   for (i = 0; i < size; i++) {
      if (value > 5) {
         lcd_write_data (0x05);
         value -= 5;
      }
      else {
         lcd_write_data (value);
         break;
      }
   }
}


/*******************************************************************************
* Display bargraph on LCD display                                              *
*   Parameter:     pos_x: horizontal position of bargraph start                *
*                  pos_y: vertical position of bargraph                        *
*                  value: size of bargraph active field (in pixels)            *
*   Return:                                                                    *
*******************************************************************************/

void hd44780_bargraphXY (int pos_x, int pos_y, int value) {
  int i;

  hd44780_set_cursor (pos_x, pos_y);
  for (i = 0; i < 16; i++)  {
    if (value > 5) {
      lcd_write_data (0x05);
      value -= 5;
    } else {
      lcd_write_data (value);
      while (i++ < 16) lcd_write_data (0);
    }
  }
}

/******************************************************************************/
