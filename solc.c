#include <stdint.h>
#include <stdio.h>
#include <solc.h>
#include "blf.h"
#include <stdlib.h>
#include "hd44780.h"
#include "bsp.h"
#include "mcal.h"
#include "button.h"
#include "menu.h"
#include "rotmotor.h"


#ifdef TEST
  #define S_START() 0
#else
  #define S_START()           (GPIO_read(BSP_HOME_POS) == 0) // Start position
#endif

#define NV_MAGIC_NR 0x4ce7

#define REFRESH_TIMER_PERIOD 100
// Signals
enum
{
  REFRESH_IND = BLF_FIRST_USER_SIG,
};


// FF/Dag = Förflyttningar per dag
// Tot.FFT = Total förflyttningstid


enum {BUTTON_NAV1_ID, BUTTON_NAV2_ID};
enum {MM_STARTT, MM_STOPT, MM_NATT, MM_FFT};


typedef struct
{
  RTC_time_t times[4];
  uint32_t totKwh;
  uint16_t ff_dag;
  uint16_t magic;
} nvdata_t;


#define ct_assert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]

// Check size of NV struct
ct_assert(sizeof(nvdata_t) <= 40);


static nvdata_t nv;
static RTC_time_t now;
static int ff_dag;


struct solc_t
{
  BLFTimer *timer;
  menu_t menu;
  int buttonPressed;
  int buttonPressedTime;
};


static int sensor;

static int time_to_sec(RTC_time_t *t)
{
  int s = (int)t->h*60;
  s = (s * 60) + t->m *60 + t->s;
  return s;
}


static uint8_t time_test_equal(RTC_time_t *t1, RTC_time_t *t2) {
	if(t1->h == t2->h && t1->m == t2->m && t1->s == t2->s) return 1;
	else return 0;
}

static uint8_t time_test_later(RTC_time_t *t1, RTC_time_t *t2) {

  int t1s = time_to_sec(t1);
  int t2s = time_to_sec(t2);
  if(t1s >= t2s)
    return 1;
	else
    return 0;
}

// T1 later for positive duration
static int duration_sec(RTC_time_t *t1, RTC_time_t *t2)
{
  int t1s = time_to_sec(t1);
  int t2s = time_to_sec(t2);
  return t1s - t2s;
}


//----------------------------------------------------------------------------
// Temperature item
static void temperature_item_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{

}

static void temperature_item_draw(menu_t *m, const menu_item_t *self)
{
  int v = *(int *)self->storage;
  int v_abs = (v < 0) ? -v : v;
  sprintf(m->is, "   %d.%0d", v/10, v_abs%10);
}

const menu_item_class_t menuItemTemperature = {0, temperature_item_event, temperature_item_draw, 4, 4, 1, 1};

//----------------------------------------------------------------------------
// Flow item
static void flow_item_event(menu_t *m, const menu_item_t *item, const menu_event_t *evt)
{

}

static void flow_item_draw(menu_t *m, const menu_item_t *self)
{
  int *v = (int *)self->storage;
  sprintf(m->is, "   %d.%0d", *v/10, *v%10);
}
const menu_item_class_t menuItemFlow = {0, flow_item_event, flow_item_draw, 4, 4, 1, 1};




static int motor_selection = 0;

static int calc_mperiod_sec(void)
{
  return (3600*(int)(nv.times[MM_STOPT].h-nv.times[MM_STARTT].h)+
            60*(nv.times[MM_STOPT].m-nv.times[MM_STARTT].m)+
		    	  (nv.times[MM_STOPT].s-nv.times[MM_STARTT].s)) / nv.ff_dag;
}


static int calc_mon_sec(void)
{
  return time_to_sec(&nv.times[MM_FFT]) / nv.ff_dag;
}

typedef enum
{
  MOVE_STATE_MANUAL,
  MOVE_STATE_FIND_START_POS,
  MOVE_STATE_WAIT_MORNING,
  MOVE_STATE_MOVING,
  MOVE_STATE_PAUSING,
  MOVE_STATE_WAIT_NIGHT,
} move_state_t;

static int amstate = MOVE_STATE_MANUAL;

static void auto_statemachine(solc_t *self)
{

  static int m_period_sec, m_on_sec;
  static uint32_t stimer = 0;
  static uint32_t presc = 0;
  if(++presc == 10)
  {
    presc = 0;
    stimer++;
  }


  switch(amstate)
  {
    case MOVE_STATE_MANUAL :
    break;

    case MOVE_STATE_FIND_START_POS :
      if(S_START())
      {
        // If time is somewhere between start time and stoptime
        if(time_test_later(&now, &nv.times[MM_STARTT]) &&
           time_test_later(&nv.times[MM_STOPT], &now))
        {
          m_period_sec = calc_mperiod_sec();
          int total_move_duration_sec = duration_sec(&nv.times[MM_STOPT], &nv.times[MM_STARTT]);
          int current_position_sec = duration_sec(&now, &nv.times[MM_STARTT]);
          // Calculate movement time to get to right position
          m_on_sec = (current_position_sec * time_to_sec(&nv.times[MM_FFT])) / total_move_duration_sec;
          stimer = 0;
          rotmotor_control(ROTMOTOR_FORWARD);
          amstate = MOVE_STATE_MOVING;
        }
        else
        {
          rotmotor_control(ROTMOTOR_STOP);
          amstate = MOVE_STATE_WAIT_MORNING;
        }
      }
    break;
    case MOVE_STATE_WAIT_MORNING :
      if(time_test_equal(&now, &nv.times[MM_STARTT]))
      {
        m_period_sec = calc_mperiod_sec();
        m_on_sec = calc_mon_sec();
        rotmotor_control(ROTMOTOR_FORWARD);
        amstate = MOVE_STATE_MOVING;
        stimer = 0;
      }
    break;
    case MOVE_STATE_MOVING :

      if(time_test_later(&now, &nv.times[MM_STOPT]))
      {
        rotmotor_control(ROTMOTOR_STOP);
        amstate = MOVE_STATE_WAIT_NIGHT;
      }
      else if(stimer > m_on_sec)
      {
    		rotmotor_control(ROTMOTOR_STOP);
        amstate = MOVE_STATE_PAUSING;
      }
    break;

    case MOVE_STATE_PAUSING :
      if(time_test_later(&now, &nv.times[MM_STOPT]))
      {
        amstate = MOVE_STATE_WAIT_NIGHT;
      }
      else if(stimer >= m_period_sec)
      {
        stimer = 0;
        m_on_sec = calc_mon_sec();  // Re-calculate motor On time
        rotmotor_control(ROTMOTOR_FORWARD);
        amstate = MOVE_STATE_MOVING;
      }
    break;

    case MOVE_STATE_WAIT_NIGHT :

       if(time_test_later(&now, &nv.times[MM_NATT]))
       {
         rotmotor_control(ROTMOTOR_BACK);
         amstate = MOVE_STATE_FIND_START_POS;
       }
    break;
  }

}

static void storeToNvmCb(menu_t *m, const menu_item_t *self)
{
  if(ff_dag == 0)
    ff_dag = 1;
  nv.ff_dag = ff_dag;
  NVRAM_write(0, &nv, sizeof(nv));
}


static void setTimeCb(menu_t *m, const menu_item_t *self)
{
  RTC_time_t *t = (RTC_time_t *)self->storage;
  RTC_setTime(t);
}

static void motorSetCb(menu_t *m, const menu_item_t *self)
{
    switch(motor_selection)
    {
      // Stop
      case 0 :
        amstate = MOVE_STATE_MANUAL;
        rotmotor_control(ROTMOTOR_STOP);
      break;
      // Auto
      case 1 :
        rotmotor_control(ROTMOTOR_BACK);
        amstate = MOVE_STATE_FIND_START_POS;
      break;
      // Forward
      case 2 :
        amstate = MOVE_STATE_MANUAL;
        rotmotor_control(ROTMOTOR_FORWARD);
      break;
      // Back
      case 3 :
        amstate = MOVE_STATE_MANUAL;
        rotmotor_control(ROTMOTOR_BACK);
      break;

    }
}



static const menu_item_t main_menu_items[];

static const char* const motor_options[] = {"Stop", "Auto", "Fram", "Bak", 0};
static const char* const motor_states[] = {"Manuell", "Tebaka", "Natt", "Flyttar", "Paus", "Kvall", 0};


static const menu_item_t sensor_items[] =
{
  {"Startpos", &menuItemInt,        0, 0, &sensor, 0},
  {"Back",     &menuItemSubMenu,    0, main_menu_items, 0, 0},
  {0}
};

extern const char FWREV_text[];
static const menu_item_t info_items[] =
{
  {"Ver",      &menuItemStr,        0, 0, (char *)FWREV_text, 0},
  {"Status" ,  &menuItemSelect,     0, motor_states, &amstate, 0},
  {"Back",     &menuItemSubMenu,    0, main_menu_items, 0, 0},
  {0}
};


static const menu_item_t main_menu_items[] =
{
  {"Tid",      &menuItemTime,       MENU_ITEM_EDITABLE, 0, &now, setTimeCb},
  {"StartT.",  &menuItemTime,       MENU_ITEM_EDITABLE, 0, &nv.times[0], storeToNvmCb},
  {"StopT.",   &menuItemTime,       MENU_ITEM_EDITABLE, 0, &nv.times[1], storeToNvmCb},
  {"NattT.",   &menuItemTime,       MENU_ITEM_EDITABLE, 0, &nv.times[2], storeToNvmCb},
  {"FF.T.",    &menuItemTime,       MENU_ITEM_EDITABLE, 0, &nv.times[3], storeToNvmCb},
  {"FF./Dag.", &menuItemInt,        MENU_ITEM_EDITABLE, 0, &ff_dag, storeToNvmCb},
  {"Sensorer", &menuItemSubMenu,    0, sensor_items, 0, 0},
  {"Info",     &menuItemSubMenu,    0, info_items, 0, 0},
  {"Motor",    &menuItemSelect,     MENU_ITEM_EDITABLE, motor_options, &motor_selection, motorSetCb},
  {0}
};

static void button_repeat(solc_t *self)
{
  static int repeatDelayTimer = 0;

  if(self->buttonPressed != -1)
  {
    if(repeatDelayTimer >= 20)
    {
      menu_event_t evt;
      evt.id = self->buttonPressed;
      evt.type = MENU_EVENT_CLICK;
      menu_event(&self->menu, &evt);
    }
    else
      repeatDelayTimer++;
  }
  else
  {
    repeatDelayTimer = 0;
  }
}


#define BACKLIGHT_ON_TIME (3*60*10ul) // Turn on for 3 minutes

static int backlight_on_timer = BACKLIGHT_ON_TIME;

static void lcd_backlight_on(void)
{
  backlight_on_timer = BACKLIGHT_ON_TIME;
}

static void lcd_backlight_timer(void)
{
  if(backlight_on_timer > 0) {
    GPIO_write(BSP_LCD_BKL, 1);
    backlight_on_timer--;
  } else {
    GPIO_write(BSP_LCD_BKL, 0);
  }
}

static void runningRefresh(solc_t *self, void *evt)
{
  static int blinkPresc = 0;
  if(self->menu.state == 0)
    RTC_getTime(&now);

  button_repeat(self);
  auto_statemachine(self);
  rotmotor_timer();
  lcd_backlight_timer();

  if(++blinkPresc == 5)
  {
    sensor = S_START();
    blinkPresc = 0;
    menu_blink(&self->menu);
    GPIO_write(BSP_STATUS_LED, !GPIO_read(BSP_STATUS_LED));
  }
  // Reset watchdog timer
  WDG_resetTimer();

  BLF_startTimer(self->timer, REFRESH_TIMER_PERIOD);
}

static BLFState running =
{
  [BLF_ENTRY_SIG] = BLF_ignore,
  [BLF_EXIT_SIG] = BLF_ignore,
  [REFRESH_IND] = (BLFEventHandler)runningRefresh,
};

// Initial transition
static void initial(solc_t *self, void *evt)
{
  // Start refresh timer
  BLF_startTimer(self->timer, REFRESH_TIMER_PERIOD);
  BLF_transit(self, running);
}

static const gpio_t button_pins[] =
{

 [BUTTON_NAV1_ID] = BSP_BTN_NAV1,
 [BUTTON_NAV2_ID] = BSP_BTN_NAV2,
};



static void button_cb(void *ctx, uint8_t event, int id)
{
  solc_t *self = (solc_t *)ctx;
  menu_event_t evt;
  evt.id = id;
  if(event == BUTTON_PRESSED_EVENT)
  {
    lcd_backlight_on();
    evt.type = MENU_EVENT_CLICK;
    menu_event(&self->menu, &evt);
    self->buttonPressed = id;
    self->buttonPressedTime = 0;
  }
  else if(event == BUTTON_RELEASED_EVENT)
  {
    // If same is released
    if(self->buttonPressed == id)
      self->buttonPressed = -1;
  }


}




void solc_bg(solc_t *self)
{
  menu_draw(&self->menu);
}


static void set_defaults(nvdata_t *nv)
{
  nv->times[MM_STARTT].h = 9;
  nv->times[MM_STARTT].m = 0;
  nv->times[MM_STARTT].s = 0;

  nv->times[MM_STOPT].h = 18;
  nv->times[MM_STOPT].m = 0;
  nv->times[MM_STOPT].s = 0;

  nv->times[MM_NATT].h = 20;
  nv->times[MM_NATT].m = 0;
  nv->times[MM_NATT].s = 0;

  nv->times[MM_FFT].h = 0;
  nv->times[MM_FFT].m = 5;
  nv->times[MM_FFT].s = 30;

  nv->totKwh = 0;
  nv->ff_dag = 30;
  nv->magic = NV_MAGIC_NR;
}


solc_t *solc_init(void)
{

  solc_t *self = BLF_createThread(sizeof(*self), 0, (BLFEventHandler)initial, 0);
  self->timer = BLF_createTimer(self, REFRESH_IND, 0);
  self->buttonPressed = -1;
  rotmotor_init();
  for(uint32_t n = 0; n < (sizeof(button_pins) / sizeof(button_pins[0])); n++)
  {
    button_t *button = BUTTON_create(button_pins[n], 0, n);
    BUTTON_enable(button, button_cb, self);
  }

  NVRAM_read(0, &nv, sizeof(nv));

  // Set default config values if magic number doesnt match
  if(nv.magic != NV_MAGIC_NR)
  {
    set_defaults(&nv);
    NVRAM_write(0, &nv, sizeof(nv));
  }
  ff_dag = nv.ff_dag;

  menu_init(&self->menu, main_menu_items);
  lcd_backlight_on();
  //BLF_attach(fs,
  return self;
}


