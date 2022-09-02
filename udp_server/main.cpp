#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define BUFLEN 128
#define SERVER_PORT 5555

void SetSockaddr (struct sockaddr_in *addr)
{
    bzero((char *) addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(SERVER_PORT);
}

void RunServer()
{
    int sockfd;
    char buffer[BUFLEN];
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    //Filling Server information
    SetSockaddr(&servaddr);

    //Bind the socket with the server address
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Bind failed");
        exit(1);
    }


    while(1)
    {
        // receiving data
        unsigned int len = sizeof(cliaddr);
        int n = recvfrom(sockfd, (char *)buffer, BUFLEN, 0, ( struct sockaddr *) &cliaddr, &len);

        if (n < 0)
        {
            perror("Receive failed");
            exit(2);
        }

        buffer[n] = '\0';
        std::cout << "Client with port " << ntohs(cliaddr.sin_port) << " sent me this: " << buffer << std::endl;

        //sending data back
        int s = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&cliaddr, len);

        if (s < 0)
        {
            perror("Send failed");
            exit(3);
        }
        std::cout << "I sent client this: " << buffer << std::endl;
    }

}

int main()
{
    RunServer();
}
