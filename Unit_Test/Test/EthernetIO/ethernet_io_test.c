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

void Process_Payload_Rejector_Write_Test(void)
{
    AppsPacket appPacket;
    uint8_t dataRaw[1] = {0x80};

    appPacket.opcode = APP_REJECTOR_WRITE_CMD;
    appPacket.dataLength = 0x1;
    appPacket.data = dataRaw;

    UTX_Rejector_Write_Status_Init();
    UTX_Rejector_Status_Init();

    UTX_Set_Rejector_Status(0x0, 0x0);

    CU_ASSERT_EQUAL(Process_Payload(appPacket), PACKET_OK);
    CU_ASSERT_EQUAL(UTX_Get_Rejector_Write_Status(0x0), 0x1);
}

void Process_Payload_Rejector_Write_2_Test(void)
{
    AppsPacket appPacket;
    uint8_t dataRaw[1] = {0x80};

    appPacket.opcode = APP_REJECTOR_WRITE_CMD;
    appPacket.dataLength = 0x1;
    appPacket.data = dataRaw;

    UTX_Rejector_Write_Status_Init();
    UTX_Rejector_Status_Init();

    UTX_Set_Rejector_Status(0x0, 0x1);

    CU_ASSERT_EQUAL(Process_Payload(appPacket), APP_ERROR_INVALID_PAYLOAD);
    CU_ASSERT_EQUAL(UTX_Get_Rejector_Write_Status(0x0), 0x0);
}

void Process_Payload_Rejector_Pulse_Test(void)
{
    AppsPacket appPacket;
    uint8_t dataRaw[1] = {0x80};
    App_Save_Rejector_Delay_Ms(100, 0);

    uint32_t rejectorDelay = App_Get_Rejector_Delay_Ms(0x0);

    appPacket.opcode = APP_REJECTOR_WRITE_PULSE_CMD;
    appPacket.dataLength = 0x1;
    appPacket.data = dataRaw;

    UTX_Rejector_Write_Status_Init();
    UTX_Rejector_Status_Init();

    UTX_Set_Rejector_Status(0x0, 0x0);

    CU_ASSERT_EQUAL(Process_Payload(appPacket), PACKET_OK);
    HAL_Delay(rejectorDelay);
    App_Rejector_Timer_Process();
    CU_ASSERT_EQUAL(UTX_Get_Rejector_Write_Status(0x0), 0x3);
}

void Validate_Packet_Test(void)
{
    uint8_t status = PACKET_OK;
    uint8_t dataRaw = {0x80};
    uint8_t recvLength = 6;

    AppsPacket appPacket;
    appPacket.opcode = APP_REJECTOR_WRITE_CMD;
    appPacket.dataLength = 0x1;
    appPacket.data = &dataRaw;

    CU_ASSERT_EQUAL(Validate_Packet(status, appPacket, recvLength), PACKET_OK);

    status = PACKET_ERROR_INVALID_SOF;

    CU_ASSERT_EQUAL(Validate_Packet(status, appPacket, recvLength), PACKET_ERROR_INVALID_SOF);

    status = PACKET_OK;
    recvLength = 2;

    CU_ASSERT_EQUAL(Validate_Packet(status, appPacket, recvLength), PACKET_ERROR_INVALID_PAYLOAD_LENGTH);

    recvLength = 5;
    appPacket.opcode = APP_REJECTOR_CNF;

    CU_ASSERT_EQUAL(Validate_Packet(status, appPacket, recvLength), APP_ERROR_INVALID_OPCODE);
}

void Decode_Packet_Test(void)
{
    uint8_t buffer[8] = {0x98, 0x0, 0x1, 0x80, 0x80, 0x99, 0x0, 0x0};
    uint8_t ret = 6;
    AppsPacket appPacket;

    CU_ASSERT_EQUAL(Decode_Packet(buffer, ret, &appPacket), PACKET_OK);
    CU_ASSERT_EQUAL(appPacket.opcode, APP_REJECTOR_WRITE_CMD);
    CU_ASSERT_EQUAL(appPacket.dataLength, 0x1);
    CU_ASSERT_EQUAL(appPacket.data, &buffer[3]);

    ret = 5;

    CU_ASSERT_EQUAL(Decode_Packet(buffer, ret, &appPacket), PACKET_ERROR_INVALID_PAYLOAD_LENGTH);

    ret = 6;
    buffer[0] = 0x99;

    CU_ASSERT_EQUAL(Decode_Packet(buffer, ret, &appPacket), PACKET_ERROR_INVALID_SOF);

    buffer[0] = 0x98;
    buffer[5] = 0x98;

    CU_ASSERT_EQUAL(Decode_Packet(buffer, ret, &appPacket), PACKET_ERROR_INVALID_EOF);

    buffer[5] = 0x99;
    buffer[4] = 0x81;

    CU_ASSERT_EQUAL(Decode_Packet(buffer, ret, &appPacket), PACKET_ERROR_INVALID_CRC);
}

void Encode_Packet_Test(void)
{
    uint8_t packet[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t expectedPacket[8] =  {0x98, 0x00, 0x01, 0x80, 0x80, 0x99, 0x00, 0x00};
    AppsPacket appPacket;
    uint8_t dataRaw[1] = {0x80};

    appPacket.opcode = APP_REJECTOR_WRITE_CMD;
    appPacket.dataLength = 0x1;
    appPacket.data = dataRaw;

    Encode_Packet(packet, appPacket);

    for (uint8_t byteIdx = 0; byteIdx < 8; byteIdx++)
    {
        CU_ASSERT_EQUAL(packet[byteIdx], expectedPacket[byteIdx]);
    }
}

void Rejector_Status_Test(void)
{
    uint8_t expectedPacket[6] = {0x98, 0x02, 0x1, 0x80, 0x80, 0x99};
    uint8_t *rejectorStatusPacket;

    UTX_Rejector_Write_Status_Init();
    UTX_Rejector_Status_Init();

    UTX_Set_Rejector_Status(0x0, 0x1);

    Send_Rejector_Status();

    rejectorStatusPacket = UTX_Get_Rejector_Status_Packet();

    for (uint8_t byteIdx = 0; byteIdx < 6; byteIdx++)
    {
        printf("Test - %d %d\n", byteIdx, rejectorStatusPacket[byteIdx]);
        CU_ASSERT_EQUAL(rejectorStatusPacket[byteIdx], expectedPacket[byteIdx]);
    }
}

static CU_TestInfo Ethernet_Io_Tests[] =
{
    { "Rejector Delay Flash Read Write Test", Rejector_Delay_Flash_Read_Write_Test },
    { "Process Payload Rejector Write Test ", Process_Payload_Rejector_Write_Test},
    { "Process Payload Rejector Write 2 Test ", Process_Payload_Rejector_Write_2_Test},
    { "Process Payload Rejector Pulse Test ", Process_Payload_Rejector_Pulse_Test},
    { "Validate Packet Test ", Validate_Packet_Test},
    { "Decode Packet Test ", Decode_Packet_Test},
    { "Encode Packet Test ", Encode_Packet_Test},
    { "Rejector Status Test ", Rejector_Status_Test},
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
