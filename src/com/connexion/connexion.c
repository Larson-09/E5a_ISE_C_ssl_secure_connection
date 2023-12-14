//
// Created by jordan on 22/11/23.
//
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "openssl/ssl.h"
#include "openssl/err.h"

#include "../protocole/protocole.h"

static int socket_server;
static SSL_CTX *ctx;
static SSL *ssl;
int running = 1;

int open_listener(int port);
SSL_CTX* init_ctx(void);
void load_certificates(SSL_CTX* context, char* cert_filepath, char* key_filepath);
void wait_for_connection();
void show_certificates();
int is_root();

void connexion_init()
{
    char port[16];
    snprintf(port, sizeof(port), "%d", SERVER_PORT);

    // Initialisation de la librairie SSL
    SSL_library_init();

    // Initialisation d'un context SSL
    ctx = init_ctx();

    // Chargement du certificat et de la clé
    load_certificates(ctx, "../certificates/server.pem", "../certificates/server_key.pem");

    // Ouvertur des ports et sockets
    socket_server = open_listener(atoi(port));

    // Mise en attente de connexion
    wait_for_connection();
}

ssize_t connexion_read(uint8_t *buffer, size_t length) {

    // Lecture des données depuis la connexion SSL
    int bytes_read = SSL_read(ssl, buffer, (int)length);

    // Si une erreur survient
    if (bytes_read == -1) {

        ERR_print_errors_fp(stderr);
        fflush(stderr);
        return -1;
    }
    // Si la connexion est fermée par le client
    else if (bytes_read == 0) {
        printf("\nConnection closed by client\n");
        wait_for_connection();
    }

    return (ssize_t)bytes_read; // Retourne le nombre d'octets lus depuis la connexion SSL
}

ssize_t connexion_write(const uint8_t *data, size_t length) {

    // Ecriture du message sur le socket
    ssize_t num_written = SSL_write(ssl, data, length);
    if (num_written < 0) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Retour du nombre de bytes écris
    return num_written;
}

void connexion_close(){
    close(socket_server);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}

/***
 * Open a listener on a specific port
 * @param port  The port to listen
 * @return      The ID of the used socket
 */
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
    // Charge tous les algorithmes cryptographiques disponibles
    OpenSSL_add_all_algorithms();

    // Charge toutes les chaînes d'erreur pour OpenSSL
    SSL_load_error_strings();

    // Sélectionne la méthode du protocole TLS v1.2 pour le serveur
    SSL_METHOD *method;
    method = TLSv1_2_server_method();

    // Crée un nouvel objet de contexte SSL
    SSL_CTX *ctx;
    ctx = SSL_CTX_new(method);

    // Vérifie si la création du contexte a réussi
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    // Retourne le contexte SSL nouvellement créé
    return ctx;
}


void load_certificates(SSL_CTX* context, char* cert_filepath, char* key_filepath)
{
    // Chargement du certificat
    printf("Loading certificates %s\n", cert_filepath);
    if (SSL_CTX_use_certificate_file(context, cert_filepath, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    // Chargement de la clé privée
    printf("Loading key %s\n", key_filepath);
    if (SSL_CTX_use_PrivateKey_file(context, key_filepath, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    // Vérification de la clé privée
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }

    printf("Certificates successfully loaded\n");
}

/***
 * Show certificate information
 * @param ssl
 */
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

/**
 * Wait for client connection on server
 */
void wait_for_connection() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    // Attend une nouvelle connexion
    printf("Waiting for connection\n");
    int client = accept(socket_server, (struct sockaddr*)&addr, &len);

    // Affiche les détails de la nouvelle connexion
    printf("\nNew connection :\n"
           "- Source : %s:%d\n"
           "- Certificate : ",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    // Crée un nouvel objet SSL pour cette connexion
    ssl = SSL_new(ctx);

    // Affiche les détails du certificat du serveur
    show_certificates(ssl);

    // Configure l'objet SSL pour utiliser le socket client pour cette connexion
    SSL_set_fd(ssl, client);

    // Vérifie et accepte la connexion sécurisée avec le client
    if(SSL_accept(ssl) == -1){
        ERR_print_errors_fp(stderr);
    }

    printf("- SSL_accept : passed\n");
}


int is_root()
{
    if (getuid() != 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}