/*
 * lfs_test.c - test littleFS with Flash
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
#include "little_fs_file_system_test.h"

/* Global variables -----------------------------------------------------------*/
uint8_t LFSWriteData[4] = {0xDE, 0x34, 0xAC, 0x89};
uint8_t LFSReadData[4];

/* Function Definition -----------------------------------------------------------*/
void File_Open_Write_Read_Test()
{
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Open("test.txt", LFS_O_WRONLY | LFS_O_CREAT), 0);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Write((uint8_t *)LFSWriteData,
                                           sizeof(LFSWriteData)), 4);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Close(), 0);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Open("test.txt", LFS_O_RDONLY), 0);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Read(LFSReadData,
                                          sizeof(LFSReadData)), 4);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Close(), 0);

    if (memcmp(LFSReadData, LFSWriteData, sizeof(LFSReadData)) != 0)
    {
        while (1);
    }
}

void File_Open_Write_Read_Random_Test()
{
    uint16_t dataLen = rand() % MAX_RANDOM_VAL;
    uint16_t dataLenBackup = dataLen;
    uint16_t dummbuff[dataLen];
    uint8_t readData[dataLen];

    uint8_t *writeData = (uint8_t *)Platform_Allocate_Memory(dataLen);
    if (writeData != NULL)
    {
        for (int i = 0; i < dataLenBackup ; i++)
        {
            writeData[i] = (uint8_t)(rand() % 255);
            dummbuff[i] = writeData[i];
        }
    }

    /*write into file*/
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Open("test2.txt", LFS_O_WRONLY | LFS_O_CREAT), 0);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Write((uint8_t *)writeData,
                                           dataLen), dataLen);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Close(), 0);

    /*read from file*/
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Open("test2.txt", LFS_O_RDONLY), 0);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Read(readData,
                                          dataLen), dataLen);
    CU_ASSERT_EQUAL(FS_Interface_Lfs_Close(), 0);

    /*compare written and read data*/
    if (memcmp(writeData, readData, dataLen) != 0)
    {
        while (1);
    }

    if (writeData != NULL)
    {
        Platform_FreeMemory(writeData);
        memset(dummbuff, 0, sizeof(dummbuff));
    }
}

static CU_TestInfo FileSystemTests[] =
{
    { "File Open Write Read Constant 4 Bytes Test", File_Open_Write_Read_Test },
    { "File Open Write Read random Test", File_Open_Write_Read_Random_Test },

    CU_TEST_INFO_NULL,
};


static CU_SuiteInfo suites[] =
{
    {"suite_success_both", NULL, NULL, NULL, NULL, FileSystemTests}, CU_SUITE_INFO_NULL
};

void LFS_Test_Add(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    LOG_TRACE(LOG_NFC_APP, "LittleFS Test");
    /* Register suites. */
    if (CU_register_suites(suites) != CUE_SUCCESS)
    {
        fprintf(stderr, "suite registration failed - %s\r\n",
                CU_get_error_msg());
        exit(EXIT_FAILURE);
    }
}
