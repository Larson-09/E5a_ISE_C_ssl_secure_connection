//
// Created by jordan on 02/06/23.
//

#ifndef C_CONNEXION_H
#define C_CONNEXION_H

#include <sys/types.h>
#include <inttypes.h>
#include <stdint.h>

/**
 * Initialize SSL connection elements
 */
void connexion_init();

/**
 * Wait for an incoming message on the used socket
 * @param buffer        the variable used to store the received message
 * @param length        the size of the buffer
 * @return              the buffer filled with the message
 */
ssize_t connexion_read(uint8_t *buffer, size_t length) ;

/**
 * Write a message on the used socket
 * @param data          the data to send
 * @param length        the size of the data
 * @return              the number of written bytes
 */
ssize_t connexion_write(const uint8_t* data, size_t length);

/**
 * Close the SSL connection and free SSL elements
 */
void connexion_close();


#endif //C_CONNEXION_H
