/*
 * swtimer.c
 *
 *  Created on: May 3, 2023
 *      Author: Akash
 */



#ifndef _SWTIMER_H
#define _SWTIMER_H

#ifdef  __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"


/* Exported types ------------------------------------------------------------*/
typedef enum
{
    TIMER_STOPPED = 0,
    TIMER_RUNNING,
    TIMER_ELAPSED,
} swtimer_status_t;

typedef struct
{
    uint32_t timeStamp;
    uint32_t delay;
} swtimer_t;

/* Exported functions --------------------------------------------------------*/
void Timer_Start(swtimer_t *p_timer, uint32_t delay);
void Timer_Stop(swtimer_t *p_timer);
swtimer_status_t Timer_GetStatus(swtimer_t *p_timer);

#ifdef  __cplusplus
}
#endif

#endif  /* __SWTIMER_H */

/******************* (C) COPYRIGHT 2018 Akash Kapashia ************************END OF FILE****************/
