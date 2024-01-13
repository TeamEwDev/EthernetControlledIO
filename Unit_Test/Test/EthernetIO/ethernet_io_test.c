/*
 * ethernet_io_test.c - Ethernet IO tests
 *
 *  Created on: May 27, 2023
 *      Author: EW Dev
 */
/* Includes -------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "CUnit.h"
#include "app.h"

/* Global variables -----------------------------------------------------------*/


/* Function Definition -----------------------------------------------------------*/
void Rejector_Delay_Flash_Read_Write_Test(void)
{
    uint32_t rejectorDelayMs = 0x23232323;
    App_Save_Rejector_Delay_Ms(rejectorDelayMs, 0);
    uint32_t readRejectorDelayMs = App_Get_Rejector_Delay_Ms(0);

    CU_ASSERT_EQUAL(readRejectorDelayMs, rejectorDelayMs);
}

static CU_TestInfo Ethernet_Io_Tests[] =
{
    { "Rejector Delay Flash Read Write Test", Rejector_Delay_Flash_Read_Write_Test },
    CU_TEST_INFO_NULL,
};


static CU_SuiteInfo Ethernet_Io_Suites[] =
{
    {"Ethernet IO Suites", NULL, NULL, NULL, NULL, Ethernet_Io_Tests}, CU_SUITE_INFO_NULL
};

void Ethernet_Io_Test_Add(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    /* Register suites. */
    if (CU_register_suites(Ethernet_Io_Suites) != CUE_SUCCESS)
    {
        fprintf(stderr, "suite registration failed - %s\r\n",
                CU_get_error_msg());
        exit(EXIT_FAILURE);
    }
}
