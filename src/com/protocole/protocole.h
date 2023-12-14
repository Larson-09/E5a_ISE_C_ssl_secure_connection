//
// Created by buryhugo on 02/06/23.
//

#ifndef C_PROTOCOLE_H
#define C_PROTOCOLE_H

#define MAX_MSG_SIZE (27)
#define MAX_MESSAGES 200
#define SERVER_PORT (12344)
#define MAX_CONNEXIONS_EN_ATTENTE (5)

#include <sys/types.h>
#include <inttypes.h>

/**
 * Structure d'un message d'arret d'urgence de M_CTRL vers M_EMB
 */
typedef struct
{
    uint8_t dlc;
    uint8_t id;
    uint8_t payload[MAX_MSG_SIZE];
} message_t;

/**
 * Code a message to send it on the network as a frame
 * @param buffer    the buffer that will contain the encoded frame
 */
void protocole_code(message_t * message, uint8_t *buffer);

/**
 * Decode an incoming network frame according to the protocole
 * @param buffer    the network fram received */
void protocole_decode(uint8_t *input);

#endif //C_PROTOCOLE_H
