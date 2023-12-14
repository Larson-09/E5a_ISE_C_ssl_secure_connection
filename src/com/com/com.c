#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>

#include "com.h"
#include "../connexion/connexion.h"

#define MQ_WRITE_NAME "/mq_write"

static pthread_t thread_read;
static pthread_t thread_write;

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

void test_message();

static int running = 1;

mqd_t mq_write;

void com_init() {
    // Open mq for socket writing
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(message_t);

    mq_unlink(MQ_WRITE_NAME);
    mq_write = mq_open(MQ_WRITE_NAME, O_CREAT | O_RDWR | O_EXCL, 0644, &attr);
    if (mq_write == (mqd_t) -1) {
        perror("Erreur création mq\n");
        mq_unlink(MQ_WRITE_NAME);
        mq_write = mq_open(MQ_WRITE_NAME, O_CREAT | O_RDWR | O_EXCL, 0644, &attr);
    }

    // Opening server on port SERVEUR_PORT
    connexion_init(SERVER_PORT);
    test_message();


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

void com_send_message(message_t *message) {
    if (mq_send(mq_write, message, sizeof(message_t), 0) == -1) {
        perror("mq_send");
        exit(EXIT_FAILURE);
    }
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
            printf("Message received :\n");
            printf("- Bytes read : %d\n", bytes_read);
            printf("- Content : %s\n", buffer);

            // Send a response message to the client
            test_message();

        }
    }
}

void *thread_write_fct(void *arg) {

    while (running) {

        // Memory allocation for the message
        message_t *msg = malloc(sizeof(message_t));
        if (msg == NULL) {
            perror("Erreur d'allocation de mémoire\n");
            exit(EXIT_FAILURE);
        }

        printf("message_t : %d\n", sizeof(message_t));
        printf("message : %d\n", sizeof(msg));

        // Waiting for a message on the message queue
        ssize_t bytes_read = mq_receive(mq_write, msg, sizeof(message_t), NULL);
        if (bytes_read == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);

        } else if (bytes_read > 0) {

            // Writing the message content in a buffer and send it on the socket
            uint8_t buffer[MAX_MSG_SIZE];
            protocole_code(msg, buffer);
            ssize_t bytes_sent = connexion_write( buffer, MAX_MSG_SIZE);

            // Display sending information
            printf("\nMessage sent :\n");
            printf("- Bytes_read : %d\n", bytes_sent);
            printf("- Message : ");

            for (int i = 0; i < MAX_MSG_SIZE; ++i) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
        }

        free(msg);
        usleep(200);
    }
    return NULL;
}

void test_message(){
    sleep(1);
    message_t *msg = malloc(sizeof(message_t));
    if (msg == NULL) {
        perror("Erreur d'allocation de mémoire\n");
        exit(EXIT_FAILURE);
    }

    msg->id = 1;
    msg->dlc = 27;

    for (int i = 0; i < MAX_MSG_SIZE; ++i) {
        msg->payload[i] = 0xFF;
    }
    com_send_message(msg);
}
