# Exploration Sécurité

## Introduction
Dans le cadre du projet PATO, nous développons un robot d’exploration automatique dont l’objectif est de cartographier une zone. Ce robot est contrôlé à distance depuis une application installée sur un smartphone.
Pour ce faire, nous utilisons une architecture client/serveur composé de :

- Un client Android installé sur un smartphone
- Un serveur C embarqué sur la carte Raspberry PI 0 du robot

Pour l’incrément 1 du projet, nous avons livré un produit avec une communication fonctionnelle, mais non sécurisée entre le client et le serveur. L’objectif de cette exploration est donc d’implémenter une communication sécurisée et de documenter notre travail pour permettre à un autre développeur de remettre en place rapidement une communication sécurisée.

## Technologies utilisées
Pour sécuriser la communication entre le client et le serveur, nous avons choisi le protocole SS. Cette technologie utilise des certificats numériques pour établir une connexion sécurisée entre le serveur et le client. Ces certificats contiennent des clés cryptographiques qui sont utilisées pour chiffrer et déchiffrer les données échangées, tout en garantissant l'authenticité des parties impliquées.
Par ailleurs, SSL offre plusieurs avantages qui ont également justifié notre choix :

- Un chiffrement robuste : SSL propose des mécanismes avancés de chiffrement asymétrique et symétrique qui garantissent la confidentialité des données échangées. Cette capacité à chiffrer les informations sensibles assure une protection solide contre toute tentative d'interception ou de lecture non autorisée
- Une compatibilité étendue : SSL est largement pris en charge par de nombreuses plateformes, bibliothèques et langages de programmation, offrant ainsi une grande flexibilité d'utilisation dans divers environnements logiciels. Ce qui dans notre cas nous assure une communication fiable entre le client Android et le serveur C.
- Une technologie éprouvée et documentée : SSL est une technologie éprouvée et solidement documentée, garantissant sa stabilité à long terme.

# Implémentation en C
Pour le serveur C, nous utilisons la librairie OpenSSL pour implémenter la sécurisation SSL. Cette bibliothèque offre une gamme complète de protocoles cryptographiques, assurant des connexions sécurisées. De plus, elle est largement éprouvée et offre une documentation étayée, ce qui assure sa stabilité dans le temps.
Initialisation de la connexion SSL

## Initialisation de la connexion
Tout d’abord, on utilise la fonction connexion_init qui permet d’initialiser les différents éléments nécessaires à la connexion SSL :

```C
void connexion_init()
{
    // Initialisation de la librairie SSL
    SSL_library_init();
    
    // Initialisation d'un context SSL
    ctx = init_ctx();
    
    // Chargement du certificat et de la clé
    load_certificates(ctx, "../certificates/server.pem", "../certificates/server_key.pem");
    
    // Ouverture des ports et sockets
    socket_server = open_listener(atoi(port));
    
    // Mise en attente de connexion
    wait_for_connection();
}
```

### Initialisation du contexte SSL

La fonction init_ctx permet de charger un contexte SSL en spécifiant la méthode de cryptage utilisée. Ce contexte est unique et doit soit être déclaré en variable globale du fichier, soit être passé en paramètre de chaque fonction qui l’utilise. Par souci de simplicité, nous avons décidié de le déclarer en globale.

```C
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
```

### Chargement des certificats
La fonction load_certificates() permet de charger et vérifier le certificat ainsi que la clé utilisés pour sécuriser la connexion :

```C
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
```

### Mise en attente de connexion
La fonction wait_for_connection définie ci-dessous permet de se mettre en attente de la connexion d’un client via les méthodes accept et SSL_accept. Elle permet également d’instancier l’objet ssl grâce à la fonction SSL_new à partir du contexte SSL créé précédement. Cet objet représente une structure de type SSL qui est utilisée pour gérer la couche de sécurité SSL/TLS lors des communications. Il est déclaré en variable globale du fichier connexion.c pour le rendre disponible aux fonctions de lecture et d’écriture.

```C
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

    // Configure l'objet SSL pour utiliser le socket client pour cette      connexion
    SSL_set_fd(ssl, client);

    // Vérifie et accepte la connexion sécurisée avec le client
    if(SSL_accept(ssl) == -1){
        ERR_print_errors_fp(stderr);
    }

    printf("- SSL_accept : passed\n");
}
```

## Réception d’un message
Pour recevoir les messages envoyés par le client, on utilise la fonction connexion_read() :

```C
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

    // Retourne le nombre d'octets lus depuis la connexion SSL
    return (ssize_t)bytes_read;
}
```

Cette dernière utilise la fonction SSL_read mise à disposition par OpenSSL. Cette fonction est bloquante puisqu’elle se met en attente d’un message entrant sur la socket définie. Lors de la réception d’un message, le contenu est stocké dans la variable buffer qui est passé par pointeur en entrée de la fonction.
Pour assurer un bon fonctionnement de l’application, cette fonction est utilisé dans un thread qui va en permanence :
Ecouter le socket en attente d’un message

En cas de message entrant, le fournir au dispatcher de l’application qui va l’envoyer au module concerné
Se remettre en attente d’un message


## Envoi d’un message
Pour envoyer des messages au client, on utilise la fonction connexion_write qui utilise elle-même la fonction SSL_write fournie par OpenSSL pour écrire les bytes à envoyer sur le socket :

```C
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
```

# Implémentation en Java
Pour le client Java, nous implémenterons la sécurisation SSL en utilisant les packages java.security pour la manipulation de certificats et de clés, et javax.net.ssl pour la sécurisation des sockets.
Initialisation de la connexion SSL
La première étape dans l’initialisation de la connexion va être de déclarer l’IP et le Port du serveur auquel nous souhaitons nous connecter. Il faut ensuite récupérer le certificat SSL du serveur tout en précisant le type de chiffrement (ici TLS), et le format (ici X.509) du certificat, le tout dans un thread dédié.

private static final String SERVER_IP = "192.168.16.36";
private static final int SERVER_PORT = 12344;


Resources resources = context.getResources();
InputStream certificateFile = resources.openRawResource(R.raw.server);
CertificateFactory certificateFactory = CertificateFactory.getInstance("X.509");
Certificate certificate = certificateFactory.generateCertificate(certificateFile);

KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
keyStore.load(null, null);
keyStore.setCertificateEntry("server", certificate);

TrustManagerFactory trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
trustManagerFactory.init(keyStore);

SSLContext sslContext = SSLContext.getInstance("TLS");
sslContext.init(null, trustManagerFactory.getTrustManagers(), null);




L’étape suivante est ensuite de déclarer un Socket SSL auquel nous associons un contexte contenant tous les éléments déclarés au-dessus, pour ensuite établir la connexion.
SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

sslSocket = (SSLSocket) sslSocketFactory.createSocket(SERVER_IP, SERVER_PORT);

sslSocket.startHandshake();


Réception d’un message
La méthode de réception d’un message est identique, que la communication utilise ou non le protocole SSL. Nous allons déclarer un InputStream reçu par le socket, sur lequel nous lirons les données provenant du serveur.
InputStream inputStream = sslSocket.getInputStream();


int bytesRead = inputStream.read(buffer);


Envoi d’un message
La méthode d’envoi d’un message est identique, que la communication utilise ou non le protocole SSL. Nous allons déclarer un OutputStream reçu par le socket, sur lequel nous écrirons les données à destination du serveur.

OutputStream outputStream = sslSocket.getOutputStream();


outputStream.write(msg);


