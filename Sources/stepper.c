
/**
	Stepper Motor Module 
	
	@author Natesh Narain
*/

#include "stepper.h"
#include "timer.h"
#include "util.h"
#include "derivative.h"
#include "adc.h"
#include "delay.h"

#define STEPPER_PORT      PTT

#define STEPPER_DDR       DDRT
#define STEPPER_DDR_INIT  0xF0

#define STEP_MASK         0x07

#define STEPPER_CHNL      4

/* limit switches */

#define LIMIT_PORT        PTAD
#define LIMIT_R           6
#define LIMIT_L           7

#define LIMIT_PRESSED(sw) IS_BIT_CLR(LIMIT_PORT, sw)

/**/

#define LEFT   1
#define RIGHT -1

// stepper coil pattern table
static char stepTable[8] = 
{
    0x80,
    0xA0,
    0x20,
    0x60,
    0x40,
    0x50,
    0x10,
    0x90
};
// table index
static unsigned char idx = 0;
static signed   char direction = RIGHT;

static unsigned int maxSteps;
static unsigned int stepSpeed = 5000;

static unsigned int currentPosition;
static unsigned int targetPosition;

static StepMode stepMode;

static volatile unsigned char busy;

/* Private Prototypes */

static void stepper_home(void);

static void step(void);

/*************************************************************************/

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
    
    // initialize the limit switch ports
    
    // set as digital GPIO
    ADC_SET_DIGITAL(LIMIT_L);
    ADC_SET_DIGITAL(LIMIT_R);
    
    // set as input
    CLR(DDRAD, BV(LIMIT_L) | BV(LIMIT_R));
    
    stepMode = STEP_HALF;
    direction = -1 * stepMode;
    
    // home the stepper 
    stepper_home();
    
    //
    TIMER_CHNL_ENABLE_INT(STEPPER_CHNL);
}

void stepper_setAngle(unsigned char angle)
{
    int diff;
    
    angle = angle % 180;
    
    // calculate the target position
    targetPosition = ( (float)angle / 180.0f ) * maxSteps;
    
    // calculate the direction to move through the table
    diff = targetPosition - currentPosition;
    direction = (diff > 0) ? stepMode : -1 * stepMode;
    
    // start the timer channel cycle
    TCHNL(STEPPER_CHNL) = TCNT + stepSpeed;
    TIMER_CHNL_ENABLE_INT(STEPPER_CHNL);
    
    busy = 1;
}

void stepper_setStepMode(StepMode mode)
{
    stepMode = mode;
}

unsigned char stepper_isBusy(void)
{
    return busy;
}

/**
	Count the steps between the 2 limit switches and center the stepper
*/
static void stepper_home(void)
{
	// find the first limit switch
	while(!LIMIT_PRESSED(LIMIT_R))
	{
	    step();
	    delay_ms(5);
	}
	
	currentPosition = 0;
	
	// step in the opposite direction to find the next switch
	direction *= -1;
	while(!LIMIT_PRESSED(LIMIT_L))
	{
        step();
        delay_ms(5);
	}

	// record number of steps between the limits
	maxSteps = currentPosition;
	
	//
}

static void step(void)
{
    unsigned char pattern;

    pattern = stepTable[idx];
    
    FORCE(STEPPER_PORT, 0xF0, pattern);
    
    idx = (idx + direction) & STEP_MASK;
    currentPosition += direction;

}

interrupt VectorNumber_Vtimch4 void stepper_handler(void)
{

    unsigned char pattern;
    
    if(currentPosition != targetPosition)
    {
        step();
        TCHNL(STEPPER_CHNL) += stepSpeed;
    }
    else
    {
        (void)TCHNL(STEPPER_CHNL);
        TIMER_CHNL_DISABLE_INT(STEPPER_CHNL);
        busy = 0;
    }

    return;
}