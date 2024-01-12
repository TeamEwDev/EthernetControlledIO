/*
 * app.c
 *
 *      Author: EW Dev
 */
/* Includes -------------------------------------------------------------------*/

#include "app.h"

wiz_NetInfo netInfo =
{
    .mac = { 0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef },
    .ip = { 192, 168, 1, 180 },
    .sn = { 255, 255, 255, 0 },
    .gw = { 192, 168, 1, 1 }
};

wiz_NetTimeout timeout =
{
    .retry_cnt = 3,         //RCR = 3
    .time_100us = 5000
};    //500ms

uint8_t buffer[8];      //client sends 8 bytes

void W5500_Select(void)
{
    HAL_GPIO_WritePin(SPI1_NSS_W5500_GPIO_Port, SPI1_NSS_W5500_Pin, GPIO_PIN_RESET);
}

void W5500_Unselect(void)
{
    HAL_GPIO_WritePin(SPI1_NSS_W5500_GPIO_Port, SPI1_NSS_W5500_Pin, GPIO_PIN_SET);
}

void W5500_ReadBuff(uint8_t *buff, uint16_t len)
{
    HAL_SPI_Receive(&hspi1, buff, len, HAL_MAX_DELAY);
}

void W5500_WriteBuff(uint8_t *buff, uint16_t len)
{
    HAL_SPI_Transmit(&hspi1, buff, len, HAL_MAX_DELAY);
}

void App_Rejector_Write(uint8_t rejectorIdx, bool state)
{
    switch (rejectorIdx)
    {
        case 0:
            {
                printf("Rejector 1 Write : Status - %d\n", state);
                HAL_GPIO_WritePin(REJECTOR_OUTPUT_24V_GPIO_Port, REJECTOR_OUTPUT_24V_Pin, state);
            }
        default:
            {
                /* Reserved DO NOTHING */
            }
    }
}

bool App_Rejector_Read(uint8_t rejectorIdx)
{
    switch (rejectorIdx)
    {
        case 0:
            {
                printf("Rejector 1 Read\n");
                return HAL_GPIO_ReadPin(REJECTOR_INPUT_24V_GPIO_Port, REJECTOR_INPUT_24V_Pin);
            }
        default:
            {
                /* Reserved DO NOTHING */
            }
    }

    return 0;
}

uint8_t W5500_ReadByte(void)
{
    uint8_t byte;
    W5500_ReadBuff(&byte, sizeof(byte));
    return byte;
}

void W5500_WriteByte(uint8_t byte)
{
    W5500_WriteBuff(&byte, sizeof(byte));
}

bool NetworkInit_W5500(void)
{
    wiz_NetInfo tmpInfo;
    wiz_NetTimeout tmpTimeout;
    wizchip_setnetinfo(&netInfo);

    //get network information
    wizchip_getnetinfo(&tmpInfo);
    printf("IP: %03d.%03d.%03d.%03d\nGW: %03d.%03d.%03d.%03d\nNet: %03d.%03d.%03d.%03d\nPort : %d\n",
           tmpInfo.ip[0], tmpInfo.ip[1], tmpInfo.ip[2], tmpInfo.ip[3],
           tmpInfo.gw[0], tmpInfo.gw[1], tmpInfo.gw[2], tmpInfo.gw[3],
           tmpInfo.sn[0], tmpInfo.sn[1], tmpInfo.sn[2], tmpInfo.sn[3],
           LISTEN_PORT);

    if (tmpInfo.mac[0] != netInfo.mac[0] ||
        tmpInfo.mac[1] != netInfo.mac[1] ||
        tmpInfo.mac[2] != netInfo.mac[2] ||
        tmpInfo.mac[3] != netInfo.mac[3])
    {
        printf("wizchip_getnetinfo failed.\n");
        return false;
    }

    //set timeout
    ctlnetwork(CN_SET_TIMEOUT, (void *)&timeout);
    ctlnetwork(CN_GET_TIMEOUT, (void *)&tmpTimeout);

    if (tmpTimeout.retry_cnt != timeout.retry_cnt || tmpTimeout.time_100us != timeout.time_100us)
    {
        printf("ctlnetwork(CN_SET_TIMEOUT) failed.\n");
        return false;
    }

    return true;
}

bool Init_W5500(void)
{
    printf("\r\ninit() called!\r\n");

    HAL_GPIO_WritePin(RESET_W5500_GPIO_Port, RESET_W5500_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(RESET_W5500_GPIO_Port, RESET_W5500_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
    printf("Registering W5500 callbacks...\r\n");
    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

    printf("Calling wizchip_init()...\r\n");
    uint8_t version = getVERSIONR();
    if (version != 0x04)
    {
        printf("getVERSIONR returns wrong version!\n");
        return false;
    }
    wiz_PhyConf phyConf;
    wizphy_getphystat(&phyConf);
    printf("PHY conf.by = {%d}, conf.mode={%d}, conf.speed={%d}, conf.duplex={%d}\n",
           phyConf.by, phyConf.mode, phyConf.speed, phyConf.duplex);

    return true;
}

void Start_Listening_To_TCP_Client(void)
{
    uint8_t ret;
    uint8_t remoteIP[4];
    uint16_t remotePort;
    AppsPacket appPacket;

    while (1)
    {
        ret = socket(CLIENT_SOCKET, Sn_MR_TCP, LISTEN_PORT, SF_TCP_NODELAY);

        if (ret < 0)
        {
            printf("socket failed{%d}.\n", ret);
            close(CLIENT_SOCKET);
            HAL_Delay(100);
            continue;
        }

        //check initialization
        while (getSn_SR(CLIENT_SOCKET) != SOCK_INIT)
        {
            HAL_Delay(10);
        }

        printf("listening....\n");
        ret = listen(CLIENT_SOCKET);
        if (ret < 0)
        {
            printf("listen failed{%d}.\n", ret);
            close(CLIENT_SOCKET);
            HAL_Delay(100);
            continue;
        }

        //check listening status
        while (getSn_SR(CLIENT_SOCKET) == SOCK_LISTEN)
        {
            HAL_Delay(10);
        }

        if (getSn_SR(CLIENT_SOCKET) == SOCK_ESTABLISHED)
        {
            //client accepted
            printf("accepted....\n");

            //get remote information
            getsockopt(CLIENT_SOCKET, SO_DESTIP, remoteIP);
            getsockopt(CLIENT_SOCKET, SO_DESTPORT, (uint8_t *)&remotePort);
            printf("remote IP[PORT]:%03d.%03d.%03d.%03d[%05d]\n",
                   remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);

            //receive data
            ret = recv(CLIENT_SOCKET, buffer, sizeof(buffer));
            if (ret < SOCKERR_SOCKSTATUS)
            {
                printf("recv failed.{%d}\n", ret);
                close(CLIENT_SOCKET); //unexpected close
                continue;
            }

            printf("received...\n %s", buffer);

            uint8_t errCode = Decode_Packet(buffer, ret, &appPacket);

            errCode = Validate_Packet(errCode, appPacket, ret);

            if (errCode == PACKET_OK)
            {
                printf("decoded packet...\n opcode - %d \n dataLength - %d \n Data - ",
                       appPacket.opcode,
                       appPacket.dataLength);

                for (uint8_t byteIdx = 0; byteIdx < appPacket.dataLength; byteIdx++)
                {
                    printf("0x%x ", appPacket.data[byteIdx]);
                }

                printf("\n");
                errCode = Process_Payload(appPacket);
            }

            Send_Confirmation(errCode);
        }
        else
        {
            printf("getSn_SR() != SOCKET_ESTABLISHED.\n");
        }
    }
    //close socket
    printf("closed...\n");
    close(CLIENT_SOCKET);
}

uint8_t Validate_Packet(uint8_t status, AppsPacket appPacket, uint8_t recvLength)
{
    uint8_t payloadLength;
    uint8_t expectedPacketLength;

    if (status == PACKET_OK)
    {
        payloadLength = Get_Payload_Length(appPacket.opcode);
        expectedPacketLength = Get_Packet_Length(payloadLength);

        if (recvLength != expectedPacketLength)
        {
            printf("Invalid packet length received : Expected - %d, Received - %d", expectedPacketLength, recvLength);
            return PACKET_ERROR_INVALID_PAYLOAD_LENGTH;
        }

        if (appPacket.opcode < APP_REJECTOR_WRITE_CMD ||
            appPacket.opcode > APP_REJECTOR_CNF ||
            appPacket.opcode == APP_REJECTOR_CNF ||
            appPacket.opcode == APP_SEND_REJECTOR_STATUS_IND)
        {
            printf("Invalid opcode received");
            return APP_ERROR_INVALID_OPCODE;
        }
    }

    return status;
}

void Send_Confirmation(uint8_t errCode)
{
    AppsPacket appPacketCnf;
    uint8_t appPacketCnfLength;

    appPacketCnf.opcode = APP_REJECTOR_CNF;
    appPacketCnf.data = &errCode;
    appPacketCnf.dataLength = 0x1;

    appPacketCnfLength = Get_Packet_Length(appPacketCnf.dataLength);
    uint8_t appPacketCnfRaw[appPacketCnfLength];

    Encode_Packet(appPacketCnfRaw, appPacketCnf);

    Send_Data_To_TCP_Client(appPacketCnfRaw, sizeof(appPacketCnfRaw));
}

uint8_t Get_Payload_Length(uint8_t opcode)
{
    switch (opcode)
    {
        case APP_REJECTOR_WRITE_CMD:
        case APP_REJECTOR_READ_CMD:
            {
                return (NUM_REJECTORS / 8) + 1;
            }
        case APP_REJECTOR_WRITE_PULSE_CMD:
            {
                return NUM_REJECTORS;
            }
        default:
            {
                printf("Invalid opcode received");
                return 0;
            }
    }
}

uint8_t Process_Payload(AppsPacket appPacket)
{
    bool state = false;
    uint8_t rejector = 0;
    uint8_t dataByte = 0;
    uint8_t statusByte = 0;
    uint8_t packetLength;
    uint8_t packetPayload[appPacket.dataLength];
    uint8_t delay_ms;
    AppsPacket rejectorStatus;

    switch (appPacket.opcode)
    {
        case APP_REJECTOR_WRITE_CMD:
            {
                printf("Opcode - APP_REJECTOR_WRITE\n");
                for (uint8_t byteIdx = 0; byteIdx < appPacket.dataLength; byteIdx++)
                {
                    dataByte = appPacket.data[byteIdx];

                    for (uint8_t bitIdx = 0; bitIdx < 8; bitIdx++)
                    {
                        state = ((dataByte & (0x80 >> bitIdx)) != 0);
                        rejector = (byteIdx * 8) + bitIdx;

                        if (state == App_Rejector_Read(rejector))
                        {
                            printf("Invalid payload - Rejector already in same state\n");
                            return APP_ERROR_INVALID_PAYLOAD;
                        }

                        App_Rejector_Write(rejector, state);
                    }
                }
                break;
            }
        case APP_REJECTOR_READ_CMD:
            {
                printf("Opcode - APP_REJECTOR_READ\n");
                packetLength = Get_Packet_Length(appPacket.dataLength);
                uint8_t rejectorStatusPkt[packetLength];

                for (uint8_t byteIdx = 0; byteIdx < appPacket.dataLength; byteIdx++)
                {
                    dataByte = appPacket.data[byteIdx];
                    statusByte = 0x0;

                    for (uint8_t bitIdx = 0; bitIdx < 8; bitIdx++)
                    {
                        state = ((dataByte & (0x1 << bitIdx)) != 0);
                        rejector = (byteIdx * 8) + bitIdx;

                        if (state)
                        {
                            statusByte = statusByte | (App_Rejector_Read(rejector) << (8 - bitIdx));
                        }
                    }
                    packetPayload[byteIdx] = statusByte;
                }

                rejectorStatus.data = (uint8_t *)packetPayload;
                rejectorStatus.dataLength = appPacket.dataLength;
                rejectorStatus.opcode = APP_SEND_REJECTOR_STATUS_IND;

                Encode_Packet(rejectorStatusPkt, rejectorStatus);

                Send_Data_To_TCP_Client(rejectorStatusPkt, sizeof(rejectorStatusPkt));
                break;
            }
        case APP_REJECTOR_WRITE_PULSE_CMD:
            {
                printf("Opcode - APP_REJECTOR_WRITE_PULSE_CMD\n");
                for (uint8_t byteIdx = 0; byteIdx < appPacket.dataLength; byteIdx++)
                {
                    delay_ms = appPacket.data[byteIdx];

                    if (delay_ms > 0)
                    {
                        App_Rejector_Write(byteIdx, REJECTOR_ON);
                        HAL_Delay(delay_ms);
                        App_Rejector_Write(byteIdx, REJECTOR_OFF);
                    }
                }
                break;
            }
        default:
            {
                /* DO NOTHING */
            }
    }

    return PACKET_OK;
}

RET_StatusTypeDef Send_Data_To_TCP_Client(uint8_t *data, uint32_t dataLen)
{
    if (getSn_SR(CLIENT_SOCKET) == SOCK_ESTABLISHED)
    {
        send(CLIENT_SOCKET, data, dataLen);
        HAL_Delay(100);
        return RET_OK;
    }
    return RET_ERROR;
}
