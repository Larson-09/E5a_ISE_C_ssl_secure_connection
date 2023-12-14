//
// Created by jordan on 22/11/23.
//
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

#include "connexion.h"
#include "../conf.c"

static int socket_server;
static SSL_CTX *ctx;
static SSL *ssl;
int running = 1;


/**
 * Initialize the SSL context
 * @return
 */
SSL_CTX* init_ctx(void);

/**
 * Load server certificate and private key
 * @param context           The SSL context used by the connection
 * @param cert_filepath     The path of the certificate file
 * @param key_filepath      The path of the private key file
 */
void load_certificates(SSL_CTX* context, char* cert_filepath, char* key_filepath);

/***
 * Show certificate information
 * @param ssl
 */
void show_certificates();
/***
 * Open a listener on a specific port
 * @param port  The port to listen on
 * @return      The ID of the used socket
 */

int open_listener(int port);

/**
 * Wait for client connection on server
 */
void wait_for_connection();


void connexion_init()
{
    char port[16];
    snprintf(port, sizeof(port), "%d", SERVER_PORT);

    // Initialize SSL library
    SSL_library_init();

    // Initialize a SSL context
    ctx = init_ctx();

    // Load and check certificate and key
    load_certificates(ctx, "../certificates/server.pem", "../certificates/server_key.pem");

    // Open a listener on socket and port
    socket_server = open_listener(atoi(port));

    // Client connection waiting
    wait_for_connection();
}

ssize_t connexion_read(uint8_t *buffer, size_t length) {

    // Waiting for a incoming message
    int bytes_read = SSL_read(ssl, buffer, (int)length);

    // If an error occurs
    if (bytes_read == -1) {

        ERR_print_errors_fp(stderr);
        fflush(stderr);
        return -1;
    }
    // If the connection is closed by the client
    else if (bytes_read == 0) {
        printf("\nConnection closed by client\n");
        wait_for_connection();
    }

    // Return the number of read bytes
    return (ssize_t)bytes_read;
}

ssize_t connexion_write(const uint8_t *data, size_t length) {

    // Write a message on the socket
    ssize_t num_written = SSL_write(ssl, data, length);
    if (num_written < 0) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Return the number of read bytes
    return num_written;
}

void connexion_close(){
    close(socket_server);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}

int open_listener(int port)
{
    printf("Opening listener on port %i\n", port);

    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("Impossible to bind port");
        abort();
    }
    if (listen(sd, 10) != 0)
    {
        perror("Impossible to configure listening port");
        abort();
    }
    return sd;
}

SSL_CTX* init_ctx(void)
{
    // Load available cryptographic algorithms
    OpenSSL_add_all_algorithms();

    // Load all error strings for OpenSSL
    SSL_load_error_strings();

    // Select the protocol method for the server
    SSL_METHOD *method;
    method = TLSv1_2_server_method();

    // Init a new SSL context
    SSL_CTX *ctx;
    ctx = SSL_CTX_new(method);

    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    return ctx;
}


void load_certificates(SSL_CTX* context, char* cert_filepath, char* key_filepath)
{
    // Load server certificate
    printf("Loading certificates %s\n", cert_filepath);
    if (SSL_CTX_use_certificate_file(context, cert_filepath, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    // Load server private key
    printf("Loading key %s\n", key_filepath);
    if (SSL_CTX_use_PrivateKey_file(context, key_filepath, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    // Check the private key
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }

    printf("Certificates successfully loaded\n");
}

void show_certificates()
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);

    if (cert != NULL)
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);

        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);

        X509_free(cert);
    }
    else
        printf("No certificates\n");
}


void wait_for_connection() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    // Wait for a connection
    printf("Waiting for connection\n");
    int client = accept(socket_server, (struct sockaddr*)&addr, &len);

    // Display connection detail
    printf("\nNew connection :\n"
           "- Source : %s:%d\n"
           "- Certificate : ",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    show_certificates(ssl);

    // Instantiate the SSL object
    ssl = SSL_new(ctx);

    // Configures the SSL object to use the client socket for this connection
    SSL_set_fd(ssl, client);

    // Verifies and accepts the secure connection with the client
    if(SSL_accept(ssl) == -1){
        ERR_print_errors_fp(stderr);
    }
}