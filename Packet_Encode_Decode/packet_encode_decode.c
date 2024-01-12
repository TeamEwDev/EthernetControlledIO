/*
 * packet_encode_decode.c
 *
 *      Author: EW Dev
 */
/* Includes -------------------------------------------------------------------*/

#include "packet_encode_decode.h"

// Function to calculate CRC (XOR of all data bits)
uint8_t Calculate_CRC(uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        crc ^= data[i];
    }
    return crc;
}

// Function to decode the packet
PACKET_ERROR_CODES Decode_Packet(uint8_t *inputStream, uint8_t length, AppsPacket *appPacket)
{

    // Extract data length from the second byte
    uint8_t dataLengthInput = inputStream[DATA_LENGTH_IDX];
    uint8_t *data = &inputStream[DATA_IDX];
    uint8_t opcode = inputStream[OPCODE_IDX];

    uint8_t receivedCRC;
    uint8_t calculatedCRC;

    // Check if the packet starts with the correct SOF byte
    if (length < MIN_FRAME_SIZE)
    {
        printf("Invalid packet format. Unable to decode.\n");
        return PACKET_ERROR_INVALID_PAYLOAD_LENGTH;
    }

    if (inputStream[SOF_IDX] != PACKET_SOF)
    {
        printf("Invalid SOF received");
        return PACKET_ERROR_INVALID_SOF;
    }

    // Check if the packet length is valid
    if (length < dataLengthInput + MIN_FRAME_SIZE)
    {
        printf("Invalid packet length. Unable to decode.\n");
        return PACKET_ERROR_INVALID_PAYLOAD_LENGTH;
    }

    // Extract CRC from the packet
    receivedCRC = inputStream[dataLengthInput + DATA_IDX];

    // Calculate CRC for the received data
    calculatedCRC = Calculate_CRC(data, dataLengthInput);

    // Check if received CRC matches calculated CRC
    if (receivedCRC != calculatedCRC)
    {
        printf("CRC check failed. Unable to decode.\n");
        return PACKET_ERROR_INVALID_CRC;
    }

    // Check if the packet ends with the correct EOF byte
    if (inputStream[dataLengthInput + EOF_IDX] != PACKET_EOF)
    {
        printf("Invalid packet format. Unable to decode.\n");
        return PACKET_ERROR_INVALID_EOF;
    }

    // Display the decoded data
    printf("Decoded data: ");
    for (int i = 0; i < dataLengthInput; i++)
    {
        printf("%c", data[i]);
    }
    printf("\n");

    appPacket->opcode     = opcode;
    appPacket->dataLength = dataLengthInput;
    appPacket->data       = data;

    return PACKET_OK;
}

void Encode_Packet(uint8_t *packet, AppsPacket appPacket)
{
    packet[SOF_IDX] = PACKET_SOF;
    packet[OPCODE_IDX] = appPacket.opcode;
    packet[DATA_LENGTH_IDX] = appPacket.dataLength;

    for (uint8_t i = 0; i < appPacket.dataLength; i++)
    {
        packet[DATA_IDX + i] = appPacket.data[i];
    }

    packet[DATA_IDX + appPacket.dataLength] = Calculate_CRC(appPacket.data, appPacket.dataLength);
    packet[appPacket.dataLength + EOF_IDX] = PACKET_EOF;
}

uint8_t Get_Packet_Length(uint8_t dataLength)
{
    return dataLength + MIN_FRAME_SIZE;
}
