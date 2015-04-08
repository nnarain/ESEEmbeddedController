
#include "stepper.h"
#include "timer.h"
#include "util.h"
#include "derivative.h"

#define STEPPER_PORT     PTT

#define STEPPER_DDR      DDRT
#define STEPPER_DDR_INIT 0xF0

#define NUM_STEPS        8

// stepper coil pattern
static char stepTable[NUM_STEPS] = 
{
    0x80,
    0xA0,
    0x20,
    0x60,
    0x40,
    0x50,
    0x10,
    0x80
};

static char idx = 0;

static unsigned int stepSpeed = 5000;

void stepper_init(void)
{
    // set pins 4 - 7 on the port to outcapture and set no output action
    // to configure as GPIO
    TIMER_CHNL_MAKE_OC(4);
    TIMER_CHNL_MAKE_OC(5);
    TIMER_CHNL_MAKE_OC(6);
    TIMER_CHNL_MAKE_OC(7);
    
    TIMER_SET_OC_ACTION(4, TIMER_OC_ACTION_NO_ACTION);
    TIMER_SET_OC_ACTION(5, TIMER_OC_ACTION_NO_ACTION);
    TIMER_SET_OC_ACTION(6, TIMER_OC_ACTION_NO_ACTION);
    TIMER_SET_OC_ACTION(7, TIMER_OC_ACTION_NO_ACTION);
    
    SET(STEPPER_DDR, STEPPER_DDR_INIT);
    
    //
    
    TC4 = TCNT + stepSpeed;
    
    TIMER_CHNL_ENABLE_INT(4);
    
}

void stepper_setPeriod(unsigned int speed)
{
    stepSpeed = speed;
}

interrupt VectorNumber_Vtimch4 void stepper_handler(void)
{

    unsigned char pattern = stepTable[idx];
    
    FORCE(STEPPER_PORT, 0xF0, pattern);
    
    idx = (idx + 1) % NUM_STEPS;
    
    TC4 += stepSpeed;

    return;
}