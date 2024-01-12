/*
 * packet_encode_decode.h
 *
 *      Author: EW Dev
 */
/* -------------------------------------------------------------------*/

#ifndef __PACKET_ENCODE_DECODE_H
#define __PACKET_ENCODE_DECODE_H

/*************************************************************
 *                        INCLUDES                           *
 *************************************************************/

#include <stdio.h>
#include "stdbool.h"
#include "stdint.h"

/*************************************************************
 *                        MAROS                              *
 *************************************************************/

#define PACKET_SOF 0x98  // Start of Frame
#define PACKET_EOF 0x99  // End of Frame

/*************************************************************
 *                   GLOBAL VARIABLES                        *
 *************************************************************/
typedef enum {
	SOF_IDX = 0,
	OPCODE_IDX = 1,
	DATA_LENGTH_IDX = 2,
	DATA_IDX = 3,
	EOF_IDX = 4,
	MIN_FRAME_SIZE = 5,
}PACKET_FRAME_IDX;

typedef struct {
	uint8_t opcode;
	uint8_t dataLength;
	uint8_t * data;
}AppsPacket;

/*************************************************************
 *                   FUNCTION PROTOTYPES                     *
 *************************************************************/

AppsPacket Decode_Packet(uint8_t *inputStream, uint8_t length);
uint8_t Calculate_CRC(uint8_t *data, uint8_t length);
void Encode_Packet(uint8_t * packet, AppsPacket appPacket);
uint8_t Get_Packet_Length(uint8_t dataLength);

#endif
