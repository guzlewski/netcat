#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXBUF 65536
#define BACKLOG 1

typedef struct args
{
    int src;
    int dest;
} args;

void *Thread(void *arg);
int CreateClientSocket(char *address, char *port, int *type, int *family);
void Client(char *address, char *port, int *type, int *family);
int CreateServerSocket(char *address, char *port, int *type, int *family);
void Server(char *address, char *port, int *type, int *family);
void PrintError(char *function);
void Help(char *name);

int main(int argc, char **argv)
{
    int c, protocol = SOCK_STREAM, listen = 0, family = AF_UNSPEC;

    opterr = 0;

    while ((c = getopt(argc, argv, "lu46")) != -1)
    {
        switch (c)
        {
        case 'l':
            listen = 1;
            break;
        case 'u':
            protocol = SOCK_DGRAM;
            break;
        case '4':
            family = AF_INET;
            break;
        case '6':
            family = AF_INET6;
            break;
        }
    }

    if (listen == 0)
    {
        if (optind + 2 != argc)
        {
            Help(argv[0]);
        }

        Client(argv[optind], argv[optind + 1], &protocol, &family);
    }
    else
    {
        char *hostname = NULL, *port;

        if (optind + 1 == argc)
        {
            port = argv[optind];
        }
        else if (optind + 2 == argc)
        {
            hostname = argv[optind];
            port = argv[optind + 1];
        }
        else
        {
            Help(argv[0]);
        }

        Server(hostname, port, &protocol, &family);
    }

    return 0;
}

void *Thread(void *arg)
{
    args a = *(args *)arg;
    int n;
    char buffer[MAXBUF];

    while ((n = read(a.src, buffer, MAXBUF - 1)) > 0)
    {
        write(a.dest, buffer, n);
    }

    if (n == -1)
    {
        PrintError("Thread read()");
    }

    return NULL;
}

int CreateClientSocket(char *address, char *port, int *type, int *family)
{
    int sockfd, gai_error;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = *family;
    hints.ai_socktype = *type;

    if ((gai_error = getaddrinfo(address, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "netcat: error in getaddrinfo(): %s\n", gai_strerror(gai_error));
        exit(0);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1)
        {
            perror("netcat: error in socket()");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("netcat: error in connect()");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        printf("netcat: error could not connect\n");
        exit(0);
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

void Client(char *address, char *port, int *type, int *family)
{
    int sockfd, n;
    char buffer[MAXBUF];
    pthread_t printer;

    sockfd = CreateClientSocket(address, port, type, family);

    args a;
    a.src = STDIN_FILENO;
    a.dest = sockfd;

    if (pthread_create(&printer, NULL, Thread, (void *)&a) != 0)
    {
        PrintError("pthread_create()");
    }

    while ((n = read(sockfd, buffer, MAXBUF)) > 0)
    {
        if (write(STDOUT_FILENO, buffer, n) == -1)
        {
            PrintError("write()");
        }
    }

    if (n == -1)
    {
        PrintError("read()");
    }

    pthread_cancel(printer);
    pthread_join(printer, NULL);

    close(sockfd);
}

int CreateServerSocket(char *address, char *port, int *type, int *family)
{
    int sockfd, gai_error;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = *family;
    hints.ai_socktype = *type;
    hints.ai_flags = AI_PASSIVE;

    if ((gai_error = getaddrinfo(address, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "netcat: error in getaddrinfo(): %s\n", gai_strerror(gai_error));
        exit(0);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1)
        {
            perror("netcat: error in socket()");
            continue;
        }

        if (*type == SOCK_STREAM)
        {
            int sockopt = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

            if (sockopt == -1)
            {
                PrintError("setsockopt()");
            }
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("netcat: error in bind()");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        printf("netcat: error could not bind\n");
        exit(0);
    }

    if (*type == SOCK_STREAM && listen(sockfd, BACKLOG) == -1)
    {
        PrintError("listen()");
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

void Server(char *address, char *port, int *type, int *family)
{
    int serverfd, clientfd, n;
    char buffer[MAXBUF];
    pthread_t printer;
    socklen_t len;
    struct sockaddr_storage cli;

    serverfd = CreateServerSocket(address, port, type, family);
    len = sizeof cli;

    if (*type == SOCK_STREAM && (clientfd = accept(serverfd, (struct sockaddr *)&cli, &len)) == -1)
    {
        PrintError("accept()");
    }
    else if (*type == SOCK_DGRAM)
    {
        clientfd = serverfd;

        if ((n = recvfrom(serverfd, buffer, MAXBUF - 1, 0, (struct sockaddr *)&cli, &len)) == -1)
        {
            PrintError("recvfrom()");
        }

        if (write(STDOUT_FILENO, buffer, n) == -1)
        {
            PrintError("write()");
        }

        if (connect(serverfd, (struct sockaddr *)&cli, len) == -1)
        {
            PrintError("connect()");
        }
    }

    args a;
    a.src = STDIN_FILENO;
    a.dest = clientfd;

    if (pthread_create(&printer, NULL, Thread, (void *)&a) != 0)
    {
        PrintError("pthread_create()");
    }

    while ((n = read(clientfd, buffer, MAXBUF)) > 0)
    {
        if (write(STDOUT_FILENO, buffer, n) == -1)
        {
            PrintError("write()");
        }
    }

    if (n == -1)
    {
        PrintError("read()");
    }

    pthread_cancel(printer);
    pthread_join(printer, NULL);

    close(clientfd);
    close(serverfd);
}

void PrintError(char *function)
{
    char buffer[256];

    sprintf(buffer, "netcat: error in %s", function);
    perror(buffer);

    exit(0);
}

void Help(char *name)
{
    printf("Usage:\n");
    printf("\t-u - use udp instead of tcp\n");
    printf("\t-l - server mode, ADDRESS can be blank, default ip will be used\n");
    printf("\t-4 - force ip4\n");
    printf("\t-6 - force ip6\n\n");
    printf("\t%s [-u -4 -6] ADDRESS PORT\n", name);
    printf("\t%s -l [-u -4 -6] [ADDRESS] PORT\n", name);
    exit(0);
}
