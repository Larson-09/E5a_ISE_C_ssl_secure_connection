#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>

#include "example_code.h"
#include "../connexion/connexion.h"
#include "../conf.c"
#include "../trace/trace.h"


#define MQ_WRITE_NAME "/mq_write"

static pthread_t thread_read;
static pthread_t thread_write;

/**
 * Send a message on the socket
 * @param message   The message to send
 * @param size      The size of the message
 */
void send_message(u_int8_t *message, ssize_t size);

/**
 * Build a test message and send it on the socket
 */
void test_message();

/**
 * Thread function used to regularly read the socket
 * @param arg
 * @return
 */
void *thread_read_fct(void *arg);

/**
 * Thread function used to regularly check the message queue and write messages on the socket
 * @param arg
 * @return
 */
void *thread_write_fct(void *arg);


static int running = 1;
mqd_t mq_write;

void launch() {
    // Open mq for socket writing
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;

    mq_unlink(MQ_WRITE_NAME);
    mq_write = mq_open(MQ_WRITE_NAME, O_CREAT | O_RDWR | O_EXCL, 0644, &attr);
    if (mq_write == (mqd_t) -1) {
        perror("Erreur création mq\n");
        mq_unlink(MQ_WRITE_NAME);
        mq_write = mq_open(MQ_WRITE_NAME, O_CREAT | O_RDWR | O_EXCL, 0644, &attr);
    }

    // Opening server on port SERVEUR_PORT
    connexion_init(SERVER_PORT);

    // Launching reading thread
    if (pthread_create(&thread_read, NULL, thread_read_fct, NULL) != 0) {
        fprintf(stderr, "erreur pthread_create thread_read\n");
        exit(-1);
    }

    // Launch writing thread
    if (pthread_create(&thread_write, NULL, thread_write_fct, NULL) != 0) {
        fprintf(stderr, "erreur pthread_create thread_read\n");
        exit(-1);
    }
}

void send_message(u_int8_t *message, ssize_t size) {
    if (mq_send(mq_write, message, size, 0) == -1) {
        perror("mq_send");
        exit(EXIT_FAILURE);
    }
}

void test_message(){
    sleep(1);
    uint8_t buffer[MAX_MSG_SIZE];

    u_int8_t filler = 0x00;
    for (int i = 0; i < MAX_MSG_SIZE; ++i) {
        buffer[i] = filler;
        filler++;
    }

    send_message(buffer, MAX_MSG_SIZE);
}

void *thread_read_fct(void *arg) {
    while (running) {

        // Read message on socket
        uint8_t buffer[MAX_MSG_SIZE];
        ssize_t bytes_read = connexion_read(buffer, MAX_MSG_SIZE);

        if (bytes_read == -1) {
            break;
        } else {
            // Display received message information
            TRACE("Message received :\n");
            TRACE("- Bytes read : %d\n", bytes_read);
            TRACE("- Content : %s\n", buffer);

            // Send a response message to the client
            test_message();

        }
    }
}

void *thread_write_fct(void *arg) {

    while (running) {

        // Memory allocation for the message
        uint8_t buffer[MAX_MSG_SIZE];

        // Waiting for a message on the message queue
        ssize_t bytes_read = mq_receive(mq_write, buffer, MAX_MSG_SIZE, NULL);
        if (bytes_read == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);

        } else if (bytes_read > 0) {
            ssize_t bytes_sent = connexion_write( buffer, MAX_MSG_SIZE);

            // Display sending information
            TRACE("\nMessage sent :\n");
            TRACE("- Bytes_read : %d\n", bytes_sent);
            TRACE("- Message : ");

            for (int i = 0; i < MAX_MSG_SIZE; ++i) {
                TRACE("%02X ", buffer[i]);
            }
            TRACE("\n");
        }

        usleep(200);
    }
    return NULL;
}
