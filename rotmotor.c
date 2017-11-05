#include "mcal.h"
#include "bsp.h"
#include "rotmotor.h"


#define M_FORWARD()	GPIO_write(BSP_M_1, 0)
#define	M_REVERSE()	GPIO_write(BSP_M_2, 0)

static void M_STOP(void)
{
  GPIO_write(BSP_M_1, 1);
  GPIO_write(BSP_M_2, 1);
}


typedef enum
{
  MOTOR_STATE_STOPPED,
  MOTOR_STATE_STOPPING,
  MOTOR_STATE_RUNNING,
} motor_state_t;

#define STOP_DELAY       15
#define START_DELAY      15

static motor_state_t state;
static rotmotor_cmd_t targetState;
static uint16_t delayTimer;



void rotmotor_init(void)
{
  M_STOP();
  delayTimer = 0;
  state = MOTOR_STATE_STOPPED;
}

void rotmotor_timer(void)
{
  if(delayTimer == 0)
    return;

  if(--delayTimer == 0)
  {
    switch(state)
    {
      case MOTOR_STATE_STOPPING:

      if(targetState == ROTMOTOR_BACK)
      {
        M_REVERSE();
        state = MOTOR_STATE_RUNNING;
      }
      else if(targetState == ROTMOTOR_FORWARD)
      {
        M_FORWARD();
        state = MOTOR_STATE_RUNNING;
      }
      else
      {
        state = MOTOR_STATE_STOPPED;
      }
      break;


      case MOTOR_STATE_RUNNING :
      case MOTOR_STATE_STOPPED :
      break;
    }
  }
}

void rotmotor_control(rotmotor_cmd_t cmd)
{
  targetState = cmd;

  switch(state)
  {
    case MOTOR_STATE_STOPPED :
      switch(cmd)
      {
        case ROTMOTOR_STOP :
        break;
        case ROTMOTOR_FORWARD :
          M_FORWARD(); // Turn on forward contactor
          state = MOTOR_STATE_RUNNING;
        break;
        case ROTMOTOR_BACK :
          M_REVERSE(); // Turn on reverse contactor
          state = MOTOR_STATE_RUNNING;
        break;
      }
    break;

    case MOTOR_STATE_STOPPING :
    break;


    case MOTOR_STATE_RUNNING :
        delayTimer = STOP_DELAY;
        M_STOP(); // Turn off relays
        state = MOTOR_STATE_STOPPING;
    break;
  }
}

