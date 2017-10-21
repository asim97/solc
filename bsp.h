#ifndef BSP_H
#define BSP_H
#include <stdint.h>
#include "mcal_gpio.h"
#ifdef __cplusplus
extern "C" {
#endif

// HW Timers

#define BSP_SM_TIMER        2

#define BSP_ADC_REF_VOLTAGE 3300

// ADC channels
enum
{
  BSP_ADC_CH_NTC_BOARD,
  BSP_ADC_CH_SUPPLY_VOLTAGE,
  BSP_ADC_CH_DIM2,
  BSP_ADC_CH_CURR1,
  BSP_ADC_CH_CURR2,
  BSP_ADC_CH_CURR3,
  BSP_ADC_CH_DIM3,
  BSP_ADC_CH_DIM1,
};



// PA
#define BSP_USER_BTN              GPIO_MAKEPIN(0,0)
#define BSP_DBG_UART_TX           GPIO_MAKEPIN(0,2)
#define BSP_DBG_UART_RX           GPIO_MAKEPIN(0,3)

#define BSP_LCD_RS                GPIO_MAKEPIN(0,4)
#define BSP_LCD_RW                GPIO_MAKEPIN(0,5)
#define BSP_LCD_E                 GPIO_MAKEPIN(0,6)
#define BSP_LCD_CP                GPIO_MAKEPIN(0,7) // Charge pump for LCD bias, TIM3_CH2

#define BSP_LCD_BKL               GPIO_MAKEPIN(0,8)
//#define BSP_WS2812B               GPIO_MAKEPIN(0,8)
//#define BSP_SLIP_UART_TX          GPIO_MAKEPIN(0,9)  // On=Low
//#define BSP_SLIP_UART_RX          GPIO_MAKEPIN(0,10) // On=Low


// PB
#define BSP_DONT_USE_PB0          GPIO_MAKEPIN(1,0) // Shorted to PA7 which is used as PWM
#define BSP_HOME_POS              GPIO_MAKEPIN(1,5)  // Active low
//#define BSP_I2C1_SCL              GPIO_MAKEPIN(1,6)  // Active low
//#define BSP_I2C1_SDA              GPIO_MAKEPIN(1,7)  // Active low
#define BSP_BTN_NAV1              GPIO_MAKEPIN(1,6)
#define BSP_BTN_NAV2              GPIO_MAKEPIN(1,7)

#define BSP_M_1                   GPIO_MAKEPIN(1,8)
#define BSP_M_2                   GPIO_MAKEPIN(1,9)

/*
#define BSP_SM_A1                 GPIO_MAKEPIN(1,12) // On=Low
#define BSP_SM_A2                 GPIO_MAKEPIN(1,13) // TIM1_CH1N, AF: 2
#define BSP_SM_B1                 GPIO_MAKEPIN(1,14) // TIM1_CH2N, AF: 2
//#define BSP_SM_B2                 GPIO_MAKEPIN(1,15) // TIM1_CH3N, AF: 2
#define BSP_WS2812B               GPIO_MAKEPIN(1,15)
*/
#define BSP_LCD_D0               GPIO_MAKEPIN(1,12)
#define BSP_LCD_D1               GPIO_MAKEPIN(1,13)
#define BSP_LCD_D2               GPIO_MAKEPIN(1,14)
#define BSP_LCD_D3               GPIO_MAKEPIN(1,15)



// PC



#define BSP_STATUS_LED              GPIO_MAKEPIN(2,13) // Active High




void BSP_init(void);


// Interfaces
enum
{
  BSP_LED_GREEN    = 0x01,
  BSP_LED_BLUE     = 0x02,
  BSP_LED_ORANGE   = 0x04,
  BSP_LED_RED      = 0x08,
 // BSP_LED_GREEN   = 0x10,
};


#ifdef __cplusplus
}
#endif

#endif
