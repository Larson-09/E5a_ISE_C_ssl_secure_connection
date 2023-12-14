//
// Created by buryhugo on 02/06/23.
//

#include "protocole.h"

void protocole_decode(uint8_t *input){
}

void protocole_code(message_t * message, uint8_t *buffer)
{
    buffer[0] = message->dlc;
    buffer[1] = message->id;

    for (int i = 0; i < sizeof(message_t) - 2; ++i) {
        buffer[i + 2] = message->payload[i];
    }

}
