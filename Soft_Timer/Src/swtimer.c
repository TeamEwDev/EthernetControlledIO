/*
 * swtimer.c
 *
 *  Created on: May 3, 2023
 *      Author: EW Dev
 */


/* Includes ------------------------------------------------------------------*/
#include "swtimer.h"

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


void Timer_Start(swtimer_t *p_swTimer, uint32_t delay, uint32_t io)
{
    p_swTimer->delay = delay;
    p_swTimer->timeStamp = HAL_GetTick();
    p_swTimer->io = io;
}

void Timer_Stop(swtimer_t *p_swTimer)
{
    p_swTimer->delay = 0;
    p_swTimer->timeStamp = 0;
}

swtimer_status_t Timer_GetStatus(swtimer_t *p_swTimer)
{
    swtimer_status_t status;
    uint32_t tick;

    tick = HAL_GetTick();

    if (p_swTimer->delay)
    {
        if (tick < p_swTimer->timeStamp)
        {
            if (((0xFFFFFFFF - p_swTimer->timeStamp) + tick) > p_swTimer->delay)
            {
                status = TIMER_ELAPSED;
            }
            else
            {
                status = TIMER_RUNNING;
            }
        }
        else
        {
            if ((tick - p_swTimer->timeStamp) > p_swTimer->delay)
            {
                status = TIMER_ELAPSED;
            }
            else
            {
                status = TIMER_RUNNING;
            }
        }
    }
    else
    {
        status = TIMER_STOPPED;
    }

    return status;
}
