/*
 * lfs_test.c - test littleFS with Flash
 *
 *  Created on: May 27, 2023
 *      Author: EW Dev
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Basic.h"
#include "stdint.h"
#include "stm32f4xx_hal.h"

int unit_test_case(void);

#ifdef UNIT_TEST_EN
void unitTestCases(void *argument)
{
    /* USER CODE BEGIN unitTestCases */
    /* Infinite loop */
    for (;;)
    {
        unit_test_case();
        HAL_Delay(1);

    }
    /* USER CODE END unitTestCases */
}
#endif

uint32_t TestCnt;

int unit_test_case(void)
{
    CU_BasicRunMode mode = CU_BRM_VERBOSE;

    setvbuf(stdout, NULL, _IONBF, 0);

    if (CU_initialize_registry())
    {
        printf("\nInitialization of Test Registry failed.\r\n");
    }
    else
    {
        CU_basic_set_mode(mode);
        printf("\nTests completed with return value %d. Test cnt:%ld\r\n",
               CU_basic_run_tests(), TestCnt++);
        CU_cleanup_registry();
    }

    return 0;
}
