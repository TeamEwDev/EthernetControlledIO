/*
 * app.h
 *
 *      Author: EW Dev
 */
/* Includes -------------------------------------------------------------------*/

#ifndef __APP_H
#define __APP_H

/*************************************************************
 *                        INCLUDES                           *
 *************************************************************/

#include "main.h"
#include "stdbool.h"
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include "socket.h"
#include "dns.h"
#include "packet_encode_decode.h"

/*************************************************************
 *                        MAROS                              *
 *************************************************************/

#define DHCP_SOCKET     0
#define DNS_SOCKET      1
#define HTTP_SOCKET     2
#define CLIENT_SOCKET   2   //tcp client socket 1
#define LISTEN_PORT     7   //server port


#define NUM_REJECTORS   1  // This parameter is configurable
#define REJECTOR_ON     1
#define REJECTOR_OFF    0
#define REJECTOR_DELAY_ADDR(rejIdx)    (FLASH_END - (3 * (rejIdx + 1))) // This is temporary . To use FS for more rejectors
#define REJECTOR_DELAY_MS_DEFAULT      10
#define APP_REJECTOR_DELAY_PAYLOAD_LENGTH   5

#ifdef DEBUG
#define APP_PRINT(args, ...) printf(args, ##__VA_ARGS__)
#else
#define APP_PRINT(args, ...)
#endif
/*************************************************************
 *                   GLOBAL VARIABLES                        *
 *************************************************************/
extern SPI_HandleTypeDef hspi1;

/*************************************************************
 *                        ENUM                               *
 *************************************************************/

typedef enum
{
    APP_REJECTOR_WRITE_CMD,
    APP_REJECTOR_READ_CMD,
    APP_SEND_REJECTOR_STATUS_IND,
    APP_REJECTOR_WRITE_PULSE_CMD,
    APP_REJECTOR_DELAY_WRITE_CMD,
    APP_REJECTOR_CNF
} APP_OPCODE;

typedef enum
{
    APP_ERROR_INVALID_OPCODE = 5,
    APP_ERROR_INVALID_PAYLOAD
} APP_ERROR_CODE;

typedef enum
{
    RET_OK       = 0x00,
    RET_ERROR    = 0x01,
    RET_BUSY     = 0x02,
    RET_TIMEOUT  = 0x03
} RET_StatusTypeDef;

/*************************************************************
 *                   FUNCTION PROTOTYPES                     *
 *************************************************************/

void W5500_Select(void);
void W5500_Unselect(void);
void W5500_ReadBuff(uint8_t *buff, uint16_t len);
void W5500_WriteBuff(uint8_t *buff, uint16_t len);
uint8_t W5500_ReadByte(void);
void W5500_WriteByte(uint8_t byte);
bool NetworkInit_W5500(void);
bool Init_W5500(void);
void Start_Listening_To_TCP_Client(void);
void App_Rejector_Write(uint8_t rejectorIdx, bool state);
bool App_Rejector_Read(uint8_t rejectorIdx);
uint8_t Process_Payload(AppsPacket appPacket);
uint8_t Get_Payload_Length(uint8_t opcode);
uint8_t Validate_Packet(uint8_t status, AppsPacket appPacket, uint8_t recvLength);
void Send_Confirmation(uint8_t errCode);
RET_StatusTypeDef Send_Data_To_TCP_Client(uint8_t *data, uint32_t dataLen);
void App_Save_Rejector_Delay_Ms(uint32_t delayMs, uint8_t rejectorIdx);
uint32_t App_Get_Rejector_Delay_Ms(uint8_t rejectorIdx);
void Send_Rejector_Status(void);
void App_Rejector_Timer_Process(void);
void App_Rejector_Delay_Timer_Config(uint32_t delay_ms);

#ifdef UNIT_TEST_EN
extern uint8_t UTXrejectorStatusPacket[(NUM_REJECTORS / 8) + 6];

void UTX_Set_Rejector_Status(uint8_t rejector, uint8_t status);
uint8_t UTX_Get_Rejector_Status(uint8_t rejector);
void UTX_Set_Rejector_Write_Status(uint8_t rejector, uint8_t On);
uint8_t UTX_Get_Rejector_Write_Status(uint8_t rejector);
void UTX_Rejector_Write_Status_Init(void);
void UTX_Rejector_Status_Init(void);
uint8_t *UTX_Get_Rejector_Status_Packet(void);
#endif
#endif
